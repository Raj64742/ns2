#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "tfrc-sink.h"
#include "formula-with-inverse.h"

static class TfrcSinkClass : public TclClass {
public:
  	TfrcSinkClass() : TclClass("Agent/TFRCSink") {}
  	TclObject* create(int, const char*const*) {
     		return (new TfrcSinkAgent());
  	}
} class_tfrcSink; 


TfrcSinkAgent::TfrcSinkAgent() : Agent(PT_TFRC_ACK), nack_timer_(this)
{
	bind("packetSize_", &size_);	
	bind("InitHistorySize_", &InitHistorySize_);
	bind("NumFeedback_", &NumFeedback_);
	bind ("AdjustHistoryAfterSS_", &adjust_history_after_ss);

	rate_ = 0; 
	rtt_ =  0; 
	tzero_ = 0;
	flost_ = 0;
	last_timestamp_ = 0;
	last_arrival_ = 0;
	last_nack_ = 0; 
	pvec_ = NULL; 
	tsvec_ = NULL; 
	rtvec_ = NULL;
	RTTvec_ = NULL;
	lossvec_ = NULL;
	total_received_ = 0;
	loss_seen_yet = 0;
	last_report_sent=0;
	maxseq = -1;
	rcvd_since_last_report  = 0;
	lastloss = 0;
	false_sample = 0;
}

/*
 * Receive new data packet.  If appropriate, generate a new report.
 */
void TfrcSinkAgent::recv(Packet *pkt, Handler *)
{
	double prevrtt;
  	hdr_tfrc *tfrch = hdr_tfrc::access(pkt); 
	double now = Scheduler::instance().clock();
	int UrgentFlag; 
	int prevmaxseq = maxseq;
	total_received_++;
	rcvd_since_last_report ++;
	double p = -1;

	UrgentFlag = tfrch->UrgentFlag;
	
	add_packet_to_history (pkt);

	prevrtt=rtt_;
	rtt_=tfrch->rtt;
	tzero_=tfrch->tzero;
	psize_=tfrch->psize;
	last_arrival_=Scheduler::instance().clock();
	last_timestamp_=tfrch->timestamp;
	rate_=tfrch->rate;
	/*if we are in slow start (i.e. (loss_seen_yet ==0)), */
	/*and if we saw a loss, report it immediately */

	if ( (rate_ < SMALLFLOAT) || (prevrtt < SMALLFLOAT) || (UrgentFlag) ||
		 ((rtt_ > SMALLFLOAT) && 
		  (now - last_report_sent >= rtt_/(float)NumFeedback_)) ||
		((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq > 1)) ) {
		/*
		 * time to generate a new report
		 */
		if((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq> 1)) {
			loss_seen_yet = 1;
			if (adjust_history_after_ss) {
				p = adjust_history(); 
			}
		}
		nextpkt(p);
	}
	Packet::free(pkt);
}

void TfrcSinkAgent::add_packet_to_history (Packet *pkt) 
{
	hdr_tfrc *tfrch = hdr_tfrc::access(pkt);
	double now = Scheduler::instance().clock();
	register int i; 
	register int seqno = tfrch->seqno;

	if (seqno >= InitHistorySize_) {
		printf ("history buffer too small\n");
		abort ();
	}
	if (pvec_ == NULL) {
		pvec_=(char *)malloc(sizeof(int)*InitHistorySize_);
		tsvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		rtvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		RTTvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		lossvec_=(char *)malloc(sizeof(double)*InitHistorySize_);
		if (pvec_ && tsvec_ && rtvec_ && RTTvec_ && lossvec_) {
			for (i = 0; i < InitHistorySize_ ; i ++) {
				pvec_[i] = UNKNOWN;
				tsvec_[i] = rtvec_[i] = RTTvec_[i] = -1; 
				lossvec_[i] = UNKNOWN;
			}
		}
		else {
			printf ("error allocating memory for packet buffers\n");
			abort (); 
		}
	}

	/* for the time being, we will ignore out of order and duplicate 
	   packets etc. */
	if (seqno > maxseq) {
		pvec_[seqno] = RCVD; 
		tsvec_[seqno] = tfrch->timestamp;
		rtvec_[seqno]=now;	
		RTTvec_[seqno]=tfrch->rtt;
		lossvec_[seqno] = NOLOSS;
		for (i = maxseq+1; i < seqno ; i ++) {
			pvec_[i] = LOST;
			tsvec_[i] = tfrch->timestamp;
			rtvec_[i]=now;	
			RTTvec_[i]=tfrch->rtt;
			if (tsvec_[i] - lastloss > RTTvec_[i]) {
/*
printf ("lost: %d %f %f %f %f\n", i, lastloss, tsvec_[i], RTTvec_[i], rate_);
*/
				lossvec_[i] = LOST;
				lastloss = tsvec_[i] + 0.5*rtt_;
			}
			else {
				lossvec_[i] = NOLOSS; 
			}
		}
		maxseq = seqno;
	}
}

/*
 * Estimate the loss rate.  This function calculates two loss rates,
 * and returns the smaller of the two.
 */
double TfrcSinkAgent::est_loss () 
{
	int sample[MAXSAMPLES+1] = {0, 0, 0, 0, 0, 0, 0, 0}; 
	double weights[MAXSAMPLES] = {1, 1, 1, 0.8, 0.6, 0.4, 0.2}; 
	int i, count;
	double p1, p2; 

	count = 0;
	// sample[i] counts the number of packets since the i-th loss event
	// sample[0] contains the most recent sample.
	for (i = maxseq; i >= 0 ; i --) {
		if (lossvec_[i] == LOST) {
			count ++;
			if (count >= MAXSAMPLES+1) {
				count = MAXSAMPLES;
				break;
			}
			else sample[count] = 1;
		}
		else sample[count]++; 
	}

	if (count == 0) 
		return 0; 

	if (count < MAXSAMPLES && false_sample > 0) 
		sample[count] += false_sample;
	p1 = 0; p2 = 0;
	double wsum1 = 0; 
	double wsum2 = 0; 
	for (i = 0; i < count; i ++) 
		wsum1 += weights[i]; 
	wsum2 = wsum1; 
	if (count < MAXSAMPLES)  
		wsum1 += weights[i]; 
	for (i = 0; i <= count; i ++) {
		// p1 is the weighted sum of all sample intervals
		p1 += weights[i]*sample[i]/wsum1;
		// p2 is the weighted sum of all sample intervals
		//  except for the first
		if (i > 0) 
			p2 += weights[i-1]*sample[i]/wsum2;
	}
	if (p2 > p1)
		p1 = p2;
/*
double now = Scheduler::instance().clock();
printf ("%f %d: ", now, count+1);
for (i = 0 ; i <= count ; i++) 
        printf ("%d ", sample[i]);
printf ("\n");
if (p1 > 0) 
	printf ("%f %f %f %f\n", now, 1/p1, wsum1, wsum2);
*/
	if (p1 > 0) 
		return 1/p1; 
	else return 999;     
}

/*
 * compute estimated throughput for report.
 */
double TfrcSinkAgent::est_thput () 
{
	double time_for_rcv_rate;
	double now = Scheduler::instance().clock();
	if ((rtt_ > 0) && ((now - last_report_sent) >= rtt_)) {
		// more than an RTT since the last report
		time_for_rcv_rate = (now - last_report_sent);
		if (time_for_rcv_rate > 0 && rcvd_since_last_report > 0) 
			return rcvd_since_last_report/time_for_rcv_rate;
		else return 1;
	}
	else {
		// count number of packets received in the last RTT
		if (rtt_ > 0){
			double last = rtvec_[maxseq]; 
			int rcvd = 0;
			int i = maxseq;
			while (((rtvec_[i] + rtt_) > last) && (i >= 0)) {
				if (pvec_[i] == RCVD) 
					rcvd++; 
				i--; 
			}
			if (rcvd > 0)
				return rcvd/rtt_; 
			else return 1; 
		}
		else return 1;
	}
}

/*
 * We just received our first loss, and need to adjust our history.
 */
double TfrcSinkAgent::adjust_history ()
{
	int i;
	double p;
	for (i = maxseq; i >= 0 ; i --) {
		if (lossvec_[i] == LOST) {
			lossvec_[i] = NOLOSS; 
		}
	}
	lastloss = tsvec_[maxseq]+0.5*rtt_; 
	p=b_to_p(est_thput()*psize_, rtt_, tzero_, psize_, 1);
	false_sample = (int)(1/p);
	return p;
}

/*
 * Schedule sending this report, and set timer for the next one.
 */
void TfrcSinkAgent::nextpkt(double p) {

	/*double now = Scheduler::instance().clock();*/

	/* send the report */
	sendpkt(p);

	/* schedule next report rtt/NumFeedback_ later */
	if (rtt_ > 0.0 && NumFeedback_ > 0) 
		nack_timer_.resched(1.5*rtt_/(float)NumFeedback_);
		/*nack_timer_.resched(rtt_/(float)NumFeedback_);*/
}

/*
 * Create report message, and send it.
 */
void TfrcSinkAgent::sendpkt(double p)
{
	double now = Scheduler::instance().clock();

	/*don't send an ACK unless we've received new data*/
	/*if we're sending slower than one packet per RTT, don't need*/
	/*multiple responses per data packet.*/

	if (last_arrival_ < last_nack_)
		return;

	Packet* pkt = allocpkt();
	if (pkt == NULL) {
		printf ("error allocating packet\n");
		abort(); 
	}

	hdr_tfrc_ack *tfrc_ackh = hdr_tfrc_ack::access(pkt);

	last_nack_=now;
	tfrc_ackh->seqno=maxseq;
	tfrc_ackh->timestamp_echo=last_timestamp_;
	tfrc_ackh->timestamp_offset=now-last_arrival_;
	tfrc_ackh->timestamp=now;
	tfrc_ackh->NumFeedback_ = NumFeedback_;
	if (p < 0) 
		tfrc_ackh->flost = est_loss (); 
	else
		tfrc_ackh->flost = p;
	tfrc_ackh->rate_since_last_report = est_thput ();
	last_report_sent = now; 
	rcvd_since_last_report = 0;
	send(pkt, 0);
}

void TfrcNackTimer::expire(Event *) {
	a_->nextpkt(-1);
}
