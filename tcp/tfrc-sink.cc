/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
	bind("InitHistorySize_", &hsz);
	bind("NumFeedback_", &NumFeedback_);
	bind ("AdjustHistoryAfterSS_", &adjust_history_after_ss);

	rate_ = 0; 
	rtt_ =  0; 
	tzero_ = 0;
	flost_ = 0;
	last_timestamp_ = 0;
	last_arrival_ = 0;
	rtvec_ = NULL;
	lossvec_ = NULL;
	total_received_ = 0;
	loss_seen_yet = 0;
	last_report_sent=0;
	maxseq = -1;
	rcvd_since_last_report  = 0;
	lastloss = 0;
	false_sample = 0;
	lastloss_round_id = -1 ;
	sample_count = 0 ;
	last_sample = 0;
	for (int i = 0 ; i < MAXSAMPLES+1 ; i ++) {
		sample[i] = 0 ; 
	}
	weights[0] = 1; 
	weights[1] = 1; 
	weights[2] = 1; 
	weights[3] = 0.8; 
	weights[4] = 0.6; 
	weights[5] = 0.4; 
	weights[6] = 0.2; 
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
	round_id = tfrch->round_id ;

	if (tfrch->seqno - last_sample > hsz) {
		printf ("time=%f, pkt=%d, last=%d history to small\n",
		         now, tfrch->seqno, last_sample);
		abort();
	}

	prevrtt=rtt_;
	rtt_=tfrch->rtt;
	tzero_=tfrch->tzero;
	psize_=tfrch->psize;
	last_arrival_=now;
	last_timestamp_=tfrch->timestamp;
	rate_=tfrch->rate;
	
	add_packet_to_history (pkt);

	/*
	 * if we are in slow start (i.e. (loss_seen_yet ==0)), 
	 * and if we saw a loss, report it immediately
	 */

	if ((rate_ < SMALLFLOAT) || (prevrtt < SMALLFLOAT) || (UrgentFlag) ||
		  ((rtt_ > SMALLFLOAT) && 
			 (now - last_report_sent >= rtt_/(float)NumFeedback_)) ||
		  ((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq > 1)) ) {
		/*
		 * time to generate a new report
		 */
		if((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq> 1)) {
			loss_seen_yet = 1;
			if (adjust_history_after_ss) {
				p = adjust_history(tfrch->timestamp); 
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

	if (lossvec_ == NULL) {
		rtvec_=(double *)malloc(sizeof(double)*hsz);
		lossvec_=(char *)malloc(sizeof(double)*hsz);
		if (rtvec_ && lossvec_) {
			for (i = 0; i < hsz ; i ++) {
				lossvec_[i] = UNKNOWN;
				rtvec_[i] = -1; 
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
		rtvec_[seqno%hsz]=now;	
		lossvec_[seqno%hsz] = RCVD;
		i = maxseq+1 ;
		while(i < seqno) {
			rtvec_[i%hsz]=now;	
			if ((last_timestamp_-lastloss > rtt_) && 
			    (round_id > lastloss_round_id)) {
				lossvec_[i%hsz] = LOST;
/*printf ("===>%d %d\n", round_id, lastloss_round_id); */
				lastloss = tfrch->timestamp;
				lastloss_round_id = round_id ;
			}
			else {
				lossvec_[i%hsz] = NOLOSS; 
			}
			i++;
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
	int i;
	double p1, p2; 

	// sample[i] counts the number of packets since the i-th loss event
	// sample[0] contains the most recent sample.
	for (i = last_sample; i <= maxseq ; i ++) {
		sample[0]++; 
		if (lossvec_[i%hsz] == LOST) {
			sample_count ++;
			if (sample_count >= MAXSAMPLES+1) 
				sample_count = MAXSAMPLES;
			shift_array (sample, MAXSAMPLES+1); 
		}
	}
	last_sample = maxseq+1 ; 

	if (sample_count == 0 && false_sample == 0) 
		return 0; 

	if (sample_count < MAXSAMPLES && false_sample > 0) {
		/* sample_count++; sample[sample_count] = false_sample;*/
		sample[sample_count] += false_sample;
		false_sample = 0 ; 
	}

	p1 = 0; p2 = 0;
	double wsum1 = 0; 
	double wsum2 = 0; 
	for (i = 0; i < sample_count; i ++) 
		wsum1 += weights[i]; 
	wsum2 = wsum1; 
	if (sample_count < MAXSAMPLES)  
		wsum1 += weights[i]; 
	for (i = 0; i <= sample_count; i ++) {
		if (i != MAXSAMPLES)
			p1 += weights[i]*sample[i]/wsum1;
		if (i > 0) 
			p2 += weights[i-1]*sample[i]/wsum2;
	}
	if (p2 > p1)
		p1 = p2;
/*
double now = Scheduler::instance().clock();
printf ("%f %d: ", now, sample_count +1);
for (i = 0 ; i <= sample_count ; i++) 
        printf ("%d ", sample[i]);
if (p1 > 0) 
	printf (" p=%.6f %.2f %.2f %d\n", 1/p1, wsum1, wsum2, maxseq);
*/
	if (p1 > 0) 
		return 1/p1; 
	else return 999;     
}
void TfrcSinkAgent::shift_array(int *a, int sz) 
{
	int i ;
	for (i = sz-2 ; i >= 0 ; i--) {
		a[i+1] = a[i] ;
	}
	a[0] = 0;
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
			double last = rtvec_[maxseq%hsz]; 
			int rcvd = 0;
			int i = maxseq;
			while (((rtvec_[i%hsz] + rtt_) > last) && (i >= 0)) {
				if (lossvec_[i%hsz] == RCVD) 
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
double TfrcSinkAgent::adjust_history (double ts)
{
	int i;
	double p;
	for (i = maxseq; i >= 0 ; i --) {
		if (lossvec_[i%hsz] == LOST) {
			lossvec_[i%hsz] = NOLOSS; 
		}
	}
	lastloss = ts; 
	lastloss_round_id = round_id ;
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

	if (last_arrival_ < last_report_sent)
		return;

	Packet* pkt = allocpkt();
	if (pkt == NULL) {
		printf ("error allocating packet\n");
		abort(); 
	}

	hdr_tfrc_ack *tfrc_ackh = hdr_tfrc_ack::access(pkt);

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
