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
	bind("SampleSizeMult_", &SampleSizeMult_);
	bind("MinNumLoss_", &MinNumLoss_);
	bind("InitHistorySize_", &InitHistorySize_);
	bind("bval_", &bval_);
	bind("NumFeedback_", &NumFeedback_);

	rate_ = 0 ; 
	rtt_ =  0 ; 
	tzero_ = 0 ;
	flost_ = 0 ;
	last_timestamp_ = 0;
	last_arrival_ = 0 ;
	last_nack_ = 0 ; 
	pvec_ = NULL ; 
	tsvec_ = NULL ; 
	rtvec_ = NULL ;
	RTTvec_ = NULL ;
	pveclen_ = 0 ; 
	pvecfirst_ = 0 ; 
	pveclast_ = 0 ; 
	prevpkt_ = -1 ; 
	total_received_ = 0 ;
	loss_seen_yet = 0 ;
	last_report_sent=0 ;
	prevpkt_=-1;
	left_edge = 0 ;
}

void TfrcSinkAgent::recv(Packet *pkt, Handler *)
{
	double prevrtt ;
  hdr_tfrc *tfrch = hdr_tfrc::access(pkt); 
	double now = Scheduler::instance().clock();
	int UrgentFlag ; 

	total_received_++;

	UrgentFlag = tfrch->UrgentFlag ;
	/*
	printf ("r %f %d\n", now, tfrch->seqno); 
	fflush(stdout); 
	*/

	if (pvec_==NULL) {

		/* create history list if not yet created */

		pvec_=(int *)malloc(sizeof(int)*InitHistorySize_);
		tsvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		rtvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		RTTvec_=(double *)malloc(sizeof(double)*InitHistorySize_);

		if ((pvec_ == NULL) || (tsvec_ == NULL) || (rtvec_ == NULL)) {
			printf ("error allocating memory\n");
			exit(1);
		}
		pveclen_=InitHistorySize_;
		pvecfirst_=0; 
		pveclast_=0;
		pvec_[0]=tfrch->seqno;
		tsvec_[0]=tfrch->timestamp;
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
		pvec_[pveclast_] = tfrch->seqno;
		tsvec_[pveclast_] = tfrch->timestamp;
		rtvec_[pveclast_] = now;
		RTTvec_[pveclast_] = tfrch->rtt;
	}

	prevrtt=rtt_;
	rtt_=tfrch->rtt;
	prevpkt_=tfrch->seqno;
	tzero_=tfrch->tzero;
	psize_=tfrch->psize;
	last_arrival_=Scheduler::instance().clock();
	last_timestamp_=tfrch->timestamp;
	rate_=tfrch->rate;

	/*if we are in slow start (i.e. (loss_seen_yet ==0)), */
	/*and if we saw a loss, report it immediately */

	if ( (rate_ < SMALLFLOAT) || 
			 (prevrtt < SMALLFLOAT)||
			 (UrgentFlag) ||
			 ((rtt_ > SMALLFLOAT) && 
				(now - last_report_sent >= rtt_/(float)NumFeedback_)) ||
			 ((pveclast_>1) && 
				(loss_seen_yet ==0) && 
			  (tfrch->seqno-pvec_[pveclast_-1] > 1))
		 ) {
		nextpkt();
		if((pveclast_>1) && 
			 (loss_seen_yet ==0) && 
			 (tfrch->seqno-pvec_[pveclast_-1] > 1)) {
			loss_seen_yet = 1;
		}
	}
	Packet::free(pkt);
}

void TfrcSinkAgent::nextpkt() {

	/*double now = Scheduler::instance().clock();*/

	/* send the report */
	sendpkt();

	/* schedule next report rtt/NumFeedback_ later */

	if (rtt_ > 0.0 && NumFeedback_ > 0) 
		nack_timer_.resched(1.5*rtt_/(float)NumFeedback_);
		/*nack_timer_.resched(rtt_/(float)NumFeedback_);*/
}

void TfrcSinkAgent::sendpkt()
{
	int sample ;	
  double p;
	int rcvd_since_last_report  = 0 ;
	double time_for_rcv_rate = -1; 
	int sent=0;
	int lost=0;
	int rcvd=0;
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
	tfrc_ackh->seqno=pvec_[pveclast_];
	tfrc_ackh->timestamp_echo=last_timestamp_;
	tfrc_ackh->timestamp_offset=now-last_arrival_;
	tfrc_ackh->timestamp=now;
	tfrc_ackh->SampleSizeMult_ = SampleSizeMult_;
	tfrc_ackh->bval_ = bval_ ;
	tfrc_ackh->NumFeedback_ = NumFeedback_ ;

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

	/* now, see what loss we actually observed */
	/* assume no redordering! */

	tfrc_ackh->signal = NORMAL ;
	
	int prev=pvec_[pveclast_]+1;
	int ix=pveclast_;
	double last_loss=LARGE_DOUBLE;

	while((sent<sample)||(lost<MinNumLoss_)) {      
		sent+=prev-pvec_[ix];
		rcvd++;
		if ((prev-pvec_[ix])!=1) {
			/*all losses within one RTT count as one*/
			if ((last_loss - tsvec_[ix]) > RTTvec_[ix]) {
	 			lost++;
	 			last_loss = tsvec_[ix];
			}
		}
		prev=pvec_[ix];
		/*have we any more history? or have we hit the left edge*/
		if (ix==pvecfirst_ || ix==left_edge) { 
			break;
		}
		ix--;
		if (ix < 0) 
			ix = pveclen_-1;
	}
	left_edge = ix ; 

	/* pick the larger of the two loss rates */

	if (sent > 0 && lost > 0 ) {
		flost_=(float)lost/(float)sent;
	}
	else {
		flost_ = 0 ;
	}

	tfrc_ackh->flost = flost_ ; 

	if ((rtt_ > 0) && ((now - last_report_sent) >= rtt_)) {
		time_for_rcv_rate = (now - last_report_sent) ; 
	}
	else if (rtt_ > 0){
		time_for_rcv_rate = rtt_; 
	}

	ix = pveclast_;
	double last = now ; 
	while((ix>=0) && ((last - rtvec_[ix]) < time_for_rcv_rate)) {
		rcvd_since_last_report ++ ;
		if (ix==pvecfirst_) 
			break;
		ix--;
		if (ix < 0) 
			ix = pveclen_-1;
	}
	if (rcvd_since_last_report > 0 && time_for_rcv_rate > SMALLFLOAT) {
		tfrc_ackh->rate_since_last_report = 
			rcvd_since_last_report/time_for_rcv_rate; 
	}
	else {
		tfrc_ackh->rate_since_last_report = 1 ; 
	}

	/* note time */
	last_report_sent = now ; 
	send(pkt, 0);
}

void TfrcNackTimer::expire(Event *) {
	a_->nextpkt();
}
