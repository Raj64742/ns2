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


TfrmSinkAgent::TfrmSinkAgent() : Agent(PT_TFRMC), rate_(0.0), 
	last_arrival_(0.0),
	last_nack_(0.0), 
	pvec_(NULL), tsvec_(NULL), total_received_(0), nack_timer_(this)
{
	bind("packetSize_", &size_);	
	bind("SampleSizeMult_", &k_);
	bind("MinNumLoss_", &MinNumLoss_);
	bind("InitHistorySize_", &InitHistorySize_);
	prevpkt_=-1;
}

void TfrmSinkAgent::recv(Packet *pkt, Handler *)
{
	double prevrtt ;
  	hdr_tfrm *tfrmh = hdr_tfrm::access(pkt); 

	total_received_++;

	if (pvec_==NULL) {
		pvec_=(int *)malloc(sizeof(int)*InitHistorySize_);
		tsvec_=(double *)malloc(sizeof(double)*InitHistorySize_);
		pveclen_=InitHistorySize_;
		pvecfirst_=0; pveclast_=0;
		pvec_[0]=tfrmh->seqno;
		tsvec_[0]=tfrmh->timestamp;
	} else {
		pveclast_++;
		if (pveclast_==pveclen_) pveclast_=0;
		if (pvecfirst_==pveclast_) pvecfirst_++;
		if (pvecfirst_==pveclen_) pvecfirst_=0;
		pvec_[pveclast_] = tfrmh->seqno;
		tsvec_[pveclast_] = tfrmh->timestamp;
	}

	prevrtt=rtt_;
	rtt_=tfrmh->rtt;
	prevpkt_=tfrmh->seqno;
	tzero_=tfrmh->tzero;
	psize_=tfrmh->psize;
	version_=tfrmh->version;
	last_arrival_=Scheduler::instance().clock();
	last_timestamp_=tfrmh->timestamp;
	if ((rate_==0.0)||(prevrtt==0.0))
		nextpkt();
	rate_=tfrmh->rate;
	Packet::free(pkt);
}

void TfrmSinkAgent::nextpkt() {
	sendpkt();
	if (rtt_!=0.0) 
		nack_timer_.resched(rtt_);
}

void TfrmSinkAgent::sendpkt()
{
	int sample ;	
        double p;
	Packet* pkt = allocpkt();
	hdr_tfrmc *tfrmch = hdr_tfrmc::access(pkt);

	double now = Scheduler::instance().clock();

	//don't send an ACK unless we've received new data
	//if we're sending slower than one packet per RTT, don't need
	//multiple responses per data packet.
	if (last_arrival_ < last_nack_)
		return;
	last_nack_=now;
	
	tfrmch->seqno=pvec_[pveclast_];
	tfrmch->timestamp_echo=last_timestamp_;
	tfrmch->timestamp_offset=now-last_arrival_;
	tfrmch->timestamp=now;

        if (rate_!=0) {
                p = b_to_p(rate_, rtt_, tzero_, psize_);
        } else {
                p=1;
	}

	sample = (int)(k_/p); 
	if (sample>total_received_)
		sample=total_received_;

	tfrmch->signal = NORMAL;

	if (pveclen_<sample) {
		//we don't have a long enough pvec for this low loss rate
		//we'd like this never to happen!
		increase_pvec(sample);
	} else {
		int sent=0;
		int lost=0;
		int rcvd=0;
		int ix=pveclast_;
		int prev=pvec_[pveclast_]+1;
		double last_loss=LARGE_DOUBLE;
		while((sent<sample)||(lost<MinNumLoss_)) {      
			sent+=prev-pvec_[ix];
			rcvd++;
			if ((prev-pvec_[ix])!=1) {
				//there was loss
				if ((last_loss - tsvec_[ix]) > rtt_) {
	  			//only count losses if they're 
				//separate congestion events
	  				lost++;
	  				last_loss = tsvec_[ix];
				}
			}
			prev=pvec_[ix];

			//have we any more history?
			if (ix==pvecfirst_) break;

			ix--;
			if (ix < 0) ix=pveclen_-1;
		}
		flost_=((float)lost)/((float)sent);
		if (version_==0) {
			if (flost_>p*1.5) {
				//we're nonconformant
				tfrmch->signal = DECREASE;
			} else if (flost_<(p/2.0)) {
				//we're overconformant
				tfrmch->signal = INCREASE;
			} else {
				/* do nothing */
			}
		}
	}
	tfrmch->flost=flost_;
	send(pkt, 0);
}

void TfrmSinkAgent::increase_pvec(int size)
{
	if (size>MAX_HISTORY_SIZE) {
		abort();
	}
	int *pvec2=(int *)malloc(sizeof(int)*2*size);
	double *tsvec2=(double *)malloc(sizeof(double)*2*size);
	int i,j;
	for(i=pvecfirst_,j=0;;i++,j++) {
		if (i==pveclen_) i=0;
		pvec2[j]=pvec_[i];
		tsvec2[j]=tsvec_[i];
		if (i==pveclast_) break;
	}
	free(pvec_);
	free(tsvec_);
	pvec_=pvec2;
	tsvec_=tsvec2;
	pveclen_=2*size;
	pvecfirst_=0;
	pveclast_=i;
}

void TfrmNackTimer::expire(Event *e) {
	a_->nextpkt();
}
