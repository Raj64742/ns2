#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "tfrm-sink.h"
#include "formula-with-inverse.h"

static class TfrmSinkClass : public TclClass {
public:
  TfrmSinkClass() : TclClass("Agent/TFRMSink") {}
  TclObject* create(int, const char*const*) {
     return (new TfrmSinkAgent());
  }
} class_tfrmSink; 


TfrmSinkAgent::TfrmSinkAgent() : Agent(PT_TFRMC), 
	nack_timer_(this),
	rate_(0.0), 
	last_arrival_(0.0),
	last_nack_(0.0), 
	pvec_(NULL), tsvec_(NULL), total_received_(0) 
{
	bind("packetSize_", &size_);	
	bind("SampleSizeMult_", &SampleSizeMult_);
	bind("MinNumLoss_", &MinNumLoss_);
	bind("InitHistorySize_", &InitHistorySize_);
	bind("HysterisisLower_", &HysterisisLower_);
	bind("HysterisisUpper_", &HysterisisUpper_);
	bind("bval_", &bval_);
	bind("NumFeedback_", &NumFeedback_);
	prevpkt_=-1;
	loss_seen_yet = 0 ;
	last_report_sent=0 ;
	rtt_ = 0 ;

}

void TfrmSinkAgent::recv(Packet *pkt, Handler *)
{
	double prevrtt ;
  hdr_tfrm *tfrmh = hdr_tfrm::access(pkt); 
	double now = Scheduler::instance().clock();

	total_received_++;

	/*
	printf ("r %f %d\n", now, tfrmh->seqno); 
	fflush(stdout); 
	*/

	if (pvec_==NULL) {

		/* create history list if not yet created */

		pvec_=(int *)malloc(sizeof(int)*InitHistorySize_);
		tsvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		rtvec_=(double *)malloc(sizeof(double)*InitHistorySize_);

		if ((pvec_ == NULL) || (tsvec_ == NULL) || (rtvec_ == NULL)) {
			printf ("error allocating memory\n");
			exit(1);
		}
		pveclen_=InitHistorySize_;
		pvecfirst_=0; 
		pveclast_=0;
		pvec_[0]=tfrmh->seqno;
		tsvec_[0]=tfrmh->timestamp;
		rtvec_[0]=now;
	} 
	else {

		/* add packet to existing history */
		/* history is maintained a queue in */
		/* circular buffer between pvecfirst and pveclast */

		pveclast_++;
		
		if (pveclast_==pveclen_)
			pveclast_=0;
		if (pvecfirst_==pveclast_) 
			pvecfirst_++;
		if (pvecfirst_==pveclen_) 
			pvecfirst_=0;
		pvec_[pveclast_] = tfrmh->seqno;
		tsvec_[pveclast_] = tfrmh->timestamp;
		rtvec_[pveclast_] = now;
	}

	prevrtt=rtt_;
	rtt_=tfrmh->rtt;
	prevpkt_=tfrmh->seqno;
	tzero_=tfrmh->tzero;
	psize_=tfrmh->psize;
	version_=tfrmh->version;
	last_arrival_=Scheduler::instance().clock();
	last_timestamp_=tfrmh->timestamp;
	rate_=tfrmh->rate;

	/*if we are in slow start (i.e. (loss_seen_yet ==0)), */
	/*and if we saw a loss, report it immediately */

	if ((rate_ < SMALLFLOAT) || (prevrtt < SMALLFLOAT)||
			( (pveclast_>1) && 
				(loss_seen_yet ==0) && 
			  (tfrmh->seqno-pvec_[pveclast_-1] > 1))) {
		nextpkt();
		if((pveclast_>1) && 
			 (loss_seen_yet ==0) && 
			 (tfrmh->seqno-pvec_[pveclast_-1] > 1)) {
			loss_seen_yet = 1;
		}
	}
	Packet::free(pkt);
}

void TfrmSinkAgent::nextpkt() {

	double now = Scheduler::instance().clock();

	/* send the report */
	sendpkt();

	/* note time */
	last_report_sent = now ; 

	/* schedule next report one rtt later */
	if (rtt_ > 0.0 && NumFeedback_ > 0) 
		nack_timer_.resched(rtt_/(float)NumFeedback_);
}

void TfrmSinkAgent::sendpkt()
{
	int sample ;	
  double p;
	int rcvd_since_last_report  = 0 ;
	double time_for_rcv_rate = -1; 
	int sent=0;
	int lost=0, slost=0;
	int rcvd=0;
	int ix=pveclast_;
	int prev=pvec_[pveclast_]+1;
	double last_loss=LARGE_DOUBLE;
	double now = Scheduler::instance().clock();

	/*don't send an ACK unless we've received new data*/
	/*if we're sending slower than one packet per RTT, don't need*/
	/*multiple responses per data packet.*/

	if (last_arrival_ < last_nack_)
		return;

	Packet* pkt = allocpkt();
	if (pkt == NULL) {
		printf ("error allocating packet\n");
		exit(1); 
	}
	hdr_tfrmc *tfrmch = hdr_tfrmc::access(pkt);
	last_nack_=now;
	tfrmch->seqno=pvec_[pveclast_];
	tfrmch->timestamp_echo=last_timestamp_;
	tfrmch->timestamp_offset=now-last_arrival_;
	tfrmch->timestamp=now;

	/* estimate loss rate from formula */

  if (rate_!=0) {
    p = b_to_p(rate_, rtt_, tzero_, psize_, bval_);
  } 
	else {
    p=1;
	}

	/* estimate size of sample in which to look for loss rate */

	sample = (int)(SampleSizeMult_/p); 
	if (sample>total_received_)
		sample=total_received_;

	if (pveclen_<sample) {
		/*we don't have a long enough pvec for this low loss rate*/
		/*we'd like this never to happen!*/
		/*
		printf ("very low loss rate: %f %f\n", now, p);
		fflush(stdout);
		*/
	}


	/* now, see what loss we actually observed */
	/*we need to keep track of two loss rates, */
	/*one over ssm/p packets and other that incudes*/
	/*a minimum of mnl losses. then we use the higher.*/

	tfrmch->signal = NORMAL ;
	while((sent<sample)||(lost<MinNumLoss_)) {      
		sent+=prev-pvec_[ix];
		rcvd++;
		if ((prev-pvec_[ix])!=1) {

			/*all losses within one RTT count as one*/

			if ((last_loss - tsvec_[ix]) > rtt_) {
					if (sent<sample) {
						/*Keep track of packtes lost within sample*/
						slost ++ ; 
					}
	 				lost++;
	 				last_loss = tsvec_[ix];
			}
		}
		prev=pvec_[ix];

		/*have we any more history?*/

		if (ix==pvecfirst_) { 
			break;
		}
		ix--;
		if (ix < 0) 
			ix = pveclen_-1;
	}

	/* pick the larger of the two loss rates */

	double xlost = 0 ;
	double ylost = 0 ;
	if (sent > 0 && lost > 0 ) {
		xlost=(float)lost/(float)sent;
	}
	if (sample > 0  && slost > 0 ) {
		ylost = (float)slost/(float)sample ; 
	}
	if (xlost > ylost) {
		flost_ = xlost ; 
	}
	else {
		flost_ = ylost ; 
	}
	if (lost > 0 || slost > 0) {
		tfrmch->flost=flost_;
	}
	else {
		tfrmch->flost= -1 ;
	}
	
	if (version_==0) {
		if (flost_> (p*(1+HysterisisUpper_)) ) {
			tfrmch->signal = DECREASE;
		} 
		else if (flost_< (p*(1-HysterisisLower_)) ) {
			tfrmch->signal = INCREASE;
		} 
		else {
		}
	}

	if ((rtt_ > 0) && ((now - last_report_sent) >= rtt_)) {
		time_for_rcv_rate = (now - last_report_sent) ; 
	}
	else if (rtt_ > 0){
		time_for_rcv_rate = rtt_; 
	}
	ix = pveclast_;
	double last = rtvec_[ix] ; 
	while((ix>=0) && ((last - rtvec_[ix]) < time_for_rcv_rate)) {
		rcvd_since_last_report ++ ;
		if (ix==pvecfirst_) 
			break;
		ix--;
		if (ix < 0) 
			ix = pveclen_-1;
	}
	if (rcvd_since_last_report > 0 && time_for_rcv_rate > SMALLFLOAT) {
		tfrmch->rate_since_last_report = 
			rcvd_since_last_report/time_for_rcv_rate; 
	}
	else {
		tfrmch->rate_since_last_report = 1 ; 
	}
	
	send(pkt, 0);
}

void TfrmNackTimer::expire(Event *) {
	a_->nextpkt();
}
