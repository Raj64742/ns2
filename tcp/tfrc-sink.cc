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
	bind ("NumSamples_", &numsamples);
	bind ("discount_", &discount);
	bind ("printLoss_", &printLoss_);
	bind ("smooth_", &smooth_);

	rtt_ =  0; 
	tzero_ = 0;
	last_timestamp_ = 0;
	last_arrival_ = 0;
	last_report_sent=0;

	maxseq = -1;
	rcvd_since_last_report  = 0;
	loss_seen_yet = 0;
	lastloss = 0;
	false_sample = 0;
	lastloss_round_id = -1 ;
	sample_count = 1 ;
	last_sample = 0;
	mult_factor_ = 1.0;
	UrgentFlag = 0 ;

	rtvec_ = NULL;
	tsvec_ = NULL;
	lossvec_ = NULL;
	sample = NULL ; 
	weights = NULL ;
	mult = NULL ;

}

/*
 * Receive new data packet.  If appropriate, generate a new report.
 */
void TfrcSinkAgent::recv(Packet *pkt, Handler *)
{
	hdr_tfrc *tfrch = hdr_tfrc::access(pkt); 
	double now = Scheduler::instance().clock();
	int prevmaxseq = maxseq;
	double p = -1;

	rcvd_since_last_report ++;

	if (numsamples < 0) {
		// This is the first packet received.
		numsamples = DEFAULT_NUMSAMPLES ;	
		// forget about losses before this
		prevmaxseq = maxseq = tfrch->seqno-1 ; 
		if (smooth_ == 1) {
			numsamples = numsamples + 1;
		}
		sample = (int *)malloc((numsamples+1)*sizeof(int));
		weights = (double *)malloc((numsamples+1)*sizeof(double));
		mult = (double *)malloc((numsamples+1)*sizeof(double));
		for (int i = 0 ; i < numsamples+1 ; i ++) {
			sample[i] = 0 ; 
		}
		if (smooth_ == 1) {
			weights[0] = 1.0;
			weights[1] = 1.0;
			weights[2] = 1.0; 
			weights[3] = 1.0; 
			weights[4] = 1.0; 
			weights[5] = 0.8;
			weights[6] = 0.6;
			weights[7] = 0.4;
			weights[8] = 0.2;
			weights[9] = 0;
		} else {
			weights[0] = 1.0;
			weights[1] = 1.0;
			weights[2] = 1.0; 
			weights[3] = 1.0; 
			weights[4] = 0.8; 
			weights[5] = 0.6;
			weights[6] = 0.4;
			weights[7] = 0.2;
			weights[8] = 0;
		}
		for (int i = 0; i < numsamples+1; i ++) {
			mult[i] = 1.0 ; 
		}
	} 

	UrgentFlag = tfrch->UrgentFlag;
	round_id = tfrch->round_id ;
	rtt_=tfrch->rtt;
	tzero_=tfrch->tzero;
	psize_=tfrch->psize;
	last_arrival_=now;
	last_timestamp_=tfrch->timestamp;
	
	add_packet_to_history (pkt);

	/*
	 * if we are in slow start (i.e. (loss_seen_yet ==0)), 
	 * and if we saw a loss, report it immediately
	 */

	if ((UrgentFlag) || ((rtt_ > SMALLFLOAT) && (now - last_report_sent >= rtt_/(float)NumFeedback_)) ||
	    ((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq > 1))) {
		/*
		 * time to generate a new report
		 */
		if((loss_seen_yet ==0) && (tfrch->seqno-prevmaxseq> 1)) {
			loss_seen_yet = 1;
			if (adjust_history_after_ss) {
				p = adjust_history(tfrch->timestamp); 
			}
		}
		UrgentFlag = 0 ;
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
		// Initializing history.
		rtvec_=(double *)malloc(sizeof(double)*hsz);
		tsvec_=(double *)malloc(sizeof(double)*hsz);
		lossvec_=(char *)malloc(sizeof(double)*hsz);
		if (rtvec_ && lossvec_) {
			for (i = 0; i < hsz ; i ++) {
				lossvec_[i] = UNKNOWN;
				rtvec_[i] = -1; 
				tsvec_[i] = -1; 
			}
			for (i = 0; i <= maxseq ; i++) {
				lossvec_[i] = NOLOSS ; 
				rtvec_[i] = now ;
				tsvec_[i] = last_timestamp_ ;
			}
		}
		else {
			printf ("error allocating memory for packet buffers\n");
			abort (); 
		}
	}

	if (tfrch->seqno - last_sample > hsz) {
		printf ("time=%f, pkt=%d, last=%d history to small\n",
		         now, tfrch->seqno, last_sample);
		abort();
	}


	/* for the time being, we will ignore out of order and duplicate 
	   packets etc. */
	if (seqno > maxseq) {
		rtvec_[seqno%hsz]=now;	
		tsvec_[seqno%hsz]=last_timestamp_;	
		lossvec_[seqno%hsz] = RCVD;
		i = maxseq+1 ;
		if (i < seqno) {
			double delta = (tsvec_[seqno]-tsvec_[maxseq%hsz])/(seqno-maxseq) ; 
			double tstamp = tsvec_[maxseq%hsz]+delta ;
			//double delta = 0 ;
			//double tstamp = last_timestamp_ ;
			while(i < seqno) {
				rtvec_[i%hsz]=now;	
				tsvec_[i%hsz]=tstamp;	
				if ((tsvec_[i%hsz]-lastloss > rtt_) && 
				    (round_id > lastloss_round_id)) {
					// Lost packets are marked as "LOST"
					// at most once per RTT.
					lossvec_[i%hsz] = LOST;
					UrgentFlag = 1 ;
					lastloss = tstamp;
					lastloss_round_id = round_id ;
				}
				else {
					// This lost packet is marked "NOLOSS"
					// because it does not begin a loss event.
					lossvec_[i%hsz] = NOLOSS; 
				}
				i++;
				tstamp = tstamp+delta;
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
	int i;
	double ave_interval1, ave_interval2; 
	int ds ; 

	// sample[i] counts the number of packets since the i-th loss event
	// sample[0] contains the most recent sample.
	for (i = last_sample; i <= maxseq ; i ++) {
		sample[0]++; 
		if (lossvec_[i%hsz] == LOST) {
		        //  new loss event
			sample_count ++;
			shift_array (sample, numsamples+1, 0); 
			multiply_array(mult, numsamples+1, mult_factor_);
			shift_array (mult, numsamples+1, 1.0); 
			mult_factor_ = 1.0;
		}
	}
	last_sample = maxseq+1 ; 

	if (sample_count>numsamples+1)
		// The array of loss intervals is full.
		ds=numsamples+1;
    	else
		ds=sample_count;

	if (sample_count == 1 && false_sample == 0) 
		// no losses yet
		return 0; 
	if (sample_count <= numsamples+1 && false_sample > 0) {
		// slow start just ended; ds should be 2
		// the false sample is added to the array.
		sample[ds-1] += false_sample;
		false_sample = 0 ; 
	}
	/* do we need to discount weights? */
	if (sample_count > 1 && discount && sample[0] > 0) {
		double ave = weighted_average(1, ds, 1.0, mult, weights, sample);
		int factor = 2;
		double ratio = (factor*ave)/sample[0];
		double min_ratio = 0.5;
		if ( ratio < 1.0) {
			// the most recent loss interval is very large
			mult_factor_ = ratio;
			if (mult_factor_ < min_ratio) 
				mult_factor_ = min_ratio;
		}
	}
	// Calculations including the most recent loss interval.
	ave_interval1 = weighted_average(0, ds, mult_factor_, mult, weights, sample);
	// The most recent loss interval does not end in a loss
	// event.  Include the most recent interval in the 
	// calculations only if this increases the estimated loss
	// interval.
	ave_interval2 = weighted_average(1, ds, mult_factor_, mult, weights, sample);
	if (ave_interval2 > ave_interval1)
		ave_interval1 = ave_interval2;
	if (ave_interval1 > 0) { 
		if (printLoss_ > 0) 
			print_loss(sample[0], ave_interval1);
		return 1/ave_interval1; 
	} else return 999;     
}

void TfrcSinkAgent::print_loss(int sample, double ave_interval)
{
	double now = Scheduler::instance().clock();
	double drops = 1/ave_interval;
	printf ("time: %7.5f current_loss_interval %5d\n", now, sample);
	printf ("time: %7.5f loss_rate: %7.5f\n", now, drops);
}

// Calculate the weighted average.
double TfrcSinkAgent::weighted_average(int start, int end, double factor, double *m, double *w, int *sample)
{
	int i; 
	double wsum = 0;
	double answer = 0;
	if (smooth_ == 1 && start == 0) {
		if (end == numsamples+1) {
			// the array is full, but we don't want to uses
			//  the last loss interval in the array
			end = end-1;
		} 
		// effectively shift the weight arrays 
		for (i = start ; i < end; i++) 
			if (i==0)
				wsum += m[i]*w[i+1];
			else 
				wsum += factor*m[i]*w[i+1];
		for (i = start ; i < end; i++)  
			if (i==0)
			 	answer += m[i]*w[i+1]*sample[i]/wsum;
			else 
				answer += factor*m[i]*w[i+1]*sample[i]/wsum;
	        return answer;

	} else {
		for (i = start ; i < end; i++) 
			if (i==0)
				wsum += m[i]*w[i];
			else 
				wsum += factor*m[i]*w[i];
		for (i = start ; i < end; i++)  
			if (i==0)
			 	answer += m[i]*w[i]*sample[i]/wsum;
			else 
				answer += factor*m[i]*w[i]*sample[i]/wsum;
	        return answer;
	}
}

// Shift array a[] up, starting with a[sz-2] -> a[sz-1].
void TfrcSinkAgent::shift_array(int *a, int sz, int defval) 
{
	int i ;
	for (i = sz-2 ; i >= 0 ; i--) {
		a[i+1] = a[i] ;
	}
	a[0] = defval;
}
void TfrcSinkAgent::shift_array(double *a, int sz, double defval) {
	int i ;
	for (i = sz-2 ; i >= 0 ; i--) {
		a[i+1] = a[i] ;
	}
	a[0] = defval;
}

// Multiply array by value, starting with array index 1.
// Array index 0 of the unshifted array contains the most recent interval.
void TfrcSinkAgent::multiply_array(double *a, int sz, double multiplier) {
	int i ;
	for (i = 1; i <= sz-1; i++) {
		double old = a[i];
		a[i] = old * multiplier ;
	}
}

/*
 * compute estimated throughput for report.
 */
double TfrcSinkAgent::est_thput () 
{
	double time_for_rcv_rate;
	double now = Scheduler::instance().clock();
	double thput = 1 ;

	if ((rtt_ > 0) && ((now - last_report_sent) >= rtt_)) {
		// more than an RTT since the last report
		time_for_rcv_rate = (now - last_report_sent);
		if (time_for_rcv_rate > 0 && rcvd_since_last_report > 0) {
			thput = rcvd_since_last_report/time_for_rcv_rate;
		}
	}
	else {
		// count number of packets received in the last RTT
		if (rtt_ > 0){
			double last = rtvec_[maxseq%hsz]; 
			int rcvd = 0;
			int i = maxseq;
			while (i > 0) {
				if (lossvec_[i%hsz] == RCVD) {
					if ((rtvec_[i%hsz] + rtt_) > last) 
						rcvd++; 
					else
						break ;
				}
				i--; 
			}
			if (rcvd > 0)
				thput = rcvd/rtt_; 
		}
	}
	return thput ;
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

	sendpkt(p);

	/* schedule next report rtt/NumFeedback_ later */
	if (rtt_ > 0.0 && NumFeedback_ > 0) 
		nack_timer_.resched(1.5*rtt_/(float)NumFeedback_);
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

	if (last_arrival_ >= last_report_sent) {

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
}

int TfrcSinkAgent::command(int argc, const char*const* argv) 
{
	if (argc == 3) {
		if (strcmp(argv[1], "weights") == 0) {
			/* 
			 * weights is a string of numbers, seperated by + signs
			 * the firs number is the total number of weights.
			 * the rest of them are the actual weights
			 * this overrides the defaults
			 */
			char *w ;
			w = (char *)calloc(strlen(argv[2])+1, sizeof(char)) ;
			if (w == NULL) {
				printf ("error allocating w\n");
				abort();
			}
			strcpy(w, (char *)argv[2]);
			numsamples = atoi(strtok(w,"+"));
			sample = (int *)malloc((numsamples+1)*sizeof(int));
			weights = (double *)malloc((numsamples+1)*sizeof(double));
			mult = (double *)malloc((numsamples+1)*sizeof(double));
			fflush(stdout);
			if (sample && weights) {
				int count = 0 ;
				while (count < numsamples) {
					sample[count] = 0;
					mult[count] = 1;
					char *w;
					w = strtok(NULL, "+");
					if (w == NULL)
						break ; 
					else {
						weights[count++] = atof(w);
					}	
				}
				if (count < numsamples) {
					printf ("error in weights string %s\n", argv[2]);
					abort();
				}
				sample[count] = 0;
				weights[count] = 0;
				mult[count] = 1;
				free(w);
				return (TCL_OK);
			}
			else {
				printf ("error allocating memory for smaple and weights:2\n");
				abort();
			}
		}
	}
	return (Agent::command(argc, argv));
}

void TfrcNackTimer::expire(Event *) {
	a_->nextpkt(-1);
}
