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
#include "flags.h"

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
	bind ("printLoss_", &printLoss_);
	bind ("algo_", &algo); // algo for loss estimation

	// for WALI ONLY
	bind ("NumSamples_", &numsamples);
	bind ("discount_", &discount);
	bind ("smooth_", &smooth_);

	// EWMA use only
	bind ("history_", &history); // EWMA history

	// for RBPH use only
	bind("minlc_", &minlc); 

	rtt_ =  0; 
	tzero_ = 0;
	last_timestamp_ = 0;
	last_arrival_ = 0;
	last_report_sent=0;

	maxseq = -1;
	rcvd_since_last_report  = 0;
	losses_since_last_report = 0;
	loss_seen_yet = 0;
	lastloss = 0;
	lastloss_round_id = -1 ;
	UrgentFlag = 0 ;

	rtvec_ = NULL;
	tsvec_ = NULL;
	lossvec_ = NULL;

	// used by WALI and EWMA
	last_sample = 0;

	// used only for WALI 
	false_sample = 0;
	sample = NULL ; 
	weights = NULL ;
	mult = NULL ;
	sample_count = 1 ;
	mult_factor_ = 1.0;
	init_WALI_flag = 0;

	// used only for EWMA
	avg_loss_int = -1 ;
	loss_int = 0 ;

	// used only bu RBPH
	sendrate = 0 ; // current send rate

}

/*
 * Receive new data packet.  If appropriate, generate a new report.
 */
void TfrcSinkAgent::recv(Packet *pkt, Handler *)
{
	hdr_tfrc *tfrch = hdr_tfrc::access(pkt); 
	hdr_flags* hf = hdr_flags::access(pkt);
	double now = Scheduler::instance().clock();
	double p = -1;
	int ecn = 0;

	rcvd_since_last_report ++;

	if (maxseq < 0) {
		maxseq = tfrch->seqno - 1 ;
		rtvec_=(double *)malloc(sizeof(double)*hsz);
		tsvec_=(double *)malloc(sizeof(double)*hsz);
		lossvec_=(char *)malloc(sizeof(double)*hsz);
		if (rtvec_ && lossvec_) {
			int i;
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
	/* for the time being, we will ignore out of order and duplicate 
	   packets etc. */
	int seqno = tfrch->seqno ;
	if (seqno > maxseq) {
		UrgentFlag = tfrch->UrgentFlag;
		round_id = tfrch->round_id ;
		rtt_=tfrch->rtt;
		tzero_=tfrch->tzero;
		psize_=tfrch->psize;
		sendrate = tfrch->rate;
		last_arrival_=now;
		last_timestamp_=tfrch->timestamp;
		rtvec_[seqno%hsz]=now;	
		tsvec_[seqno%hsz]=last_timestamp_;	
		lossvec_[seqno%hsz] = RCVD;
		int i = maxseq+1 ;
		double delta = (tsvec_[seqno%hsz]-tsvec_[maxseq%hsz])/(seqno-maxseq) ; 
		double tstamp = tsvec_[maxseq%hsz]+delta ;
		if (i < seqno) {
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
				losses_since_last_report++;
				tstamp = tstamp+delta;
			}
		}
		if (hf->ect() == 1 && hf->ce() == 1) {
			if ((tsvec_[i%hsz]-lastloss > rtt_) && 
			    (round_id > lastloss_round_id)) {
				// ECN action
				ecn = 1;
				lossvec_[seqno%hsz] = ECNLOST;
				UrgentFlag = 1 ;
				lastloss = tstamp;
				lastloss_round_id = round_id ;
				losses_since_last_report++;
				tstamp = tstamp+delta;
			} else lossvec_[seqno%hsz] = ECNNOLOSS;
		}
		int x = maxseq ; 
		maxseq = tfrch->seqno ;
		// if we are in slow start (i.e. (loss_seen_yet ==0)), 
		// and if we saw a loss, report it immediately
		if ((algo == WALI) && (loss_seen_yet ==0) && 
		  (tfrch->seqno-x > 1 || ecn )) {
			UrgentFlag = 1 ; 
			loss_seen_yet = 1;
			if (adjust_history_after_ss) {
				p = adjust_history(tfrch->timestamp);
			}

		}
		if ((rtt_ > SMALLFLOAT) && 
				(now - last_report_sent >= rtt_/NumFeedback_)) {
			UrgentFlag = 1 ;
		}
		if (UrgentFlag) {
			UrgentFlag = 0 ;
			nextpkt(p);
		}
	}
	Packet::free(pkt);
}

double TfrcSinkAgent::est_loss () 
{	
	double p = 0 ;
	switch (algo) {
		case WALI:
			p = est_loss_WALI () ;
			break;
		case EWMA:
			p = est_loss_EWMA () ;
			break;
		case RBPH:
			p = est_loss_RBPH () ;
			break;
		case EBPH:
			p = est_loss_EBPH () ;
			break;
		default:
			printf ("invalid algo specified\n");
			abort();
			break ; 
	}
	return p;
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

/*
 * Schedule sending this report, and set timer for the next one.
 */
void TfrcSinkAgent::nextpkt(double p) {

	sendpkt(p);

	/* schedule next report rtt/NumFeedback_ later */
	if (rtt_ > 0.0 && NumFeedback_ > 0) 
		nack_timer_.resched(1.5*rtt_/NumFeedback_);
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
        /*
	 * Do we want to send a report even if we have not received
	 * any new data?
         */ 

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
		tfrc_ackh->losses = losses_since_last_report;
		last_report_sent = now; 
		rcvd_since_last_report = 0;
		losses_since_last_report = 0;
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

void TfrcSinkAgent::print_loss(int sample, double ave_interval)
{
	double now = Scheduler::instance().clock();
	double drops = 1/ave_interval;
	// This is ugly to include this twice, but the first one is
	//   for backward compatibility with earlier scripts. 
	printf ("time: %7.5f loss_rate: %7.5f \n", now, drops);
	printf ("time: %7.5f sample 0: %5d loss_rate: %7.5f \n", 
		now, sample, drops);
	//printf ("time: %7.5f send_rate: %7.5f\n", now, sendrate);
	//printf ("time: %7.5f maxseq: %d\n", now, maxseq);
}

void TfrcSinkAgent::print_loss_all(int *sample) 
{
	double now = Scheduler::instance().clock();
	printf ("%f: sample 0: %5d 1: %5d 2: %5d 3: %5d 4: %5d\n", 
		now, sample[0], sample[1], sample[2], sample[3], sample[4]); 
}

////////////////////////////////////////
// algo specific code /////////////////
///////////////////////////////////////


////
/// WALI Code
////
double TfrcSinkAgent::est_loss_WALI () 
{
	int i;
	double ave_interval1, ave_interval2; 
	int ds ; 
		
	if (!init_WALI_flag) {
		init_WALI () ;
	}
	// sample[i] counts the number of packets since the i-th loss event
	// sample[0] contains the most recent sample.
	for (i = last_sample; i <= maxseq ; i ++) {
		sample[0]++;
		if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST) {
		        //  new loss event
			// double now = Scheduler::instance().clock();
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
		if (printLoss_ > 0) {
			print_loss(sample[0], ave_interval1);
			print_loss_all(sample);
		}
		return 1/ave_interval1; 
	} else return 999;     
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

double TfrcSinkAgent::adjust_history (double ts)
{
	int i;
	double p;
	for (i = maxseq; i >= 0 ; i --) {
		if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST ) {
			lossvec_[i%hsz] = NOLOSS; 
		}
	}
	lastloss = ts; 
	lastloss_round_id = round_id ;
	p=b_to_p(est_thput()*psize_, rtt_, tzero_, psize_, 1);
	false_sample = (int)(1.0/p);
	sample[1] = false_sample;
	sample[0] = 0;
	sample_count++; 
	if (printLoss_) {
		print_loss_all (sample);
	}
	false_sample = -1 ; 
	return p;
}


void TfrcSinkAgent::init_WALI () {
	int i;
	if (numsamples < 0)
		numsamples = DEFAULT_NUMSAMPLES ;	
	if (smooth_ == 1) {
		numsamples = numsamples + 1;
	}
	sample = (int *)malloc((numsamples+1)*sizeof(int));
	weights = (double *)malloc((numsamples+1)*sizeof(double));
	mult = (double *)malloc((numsamples+1)*sizeof(double));
	for (i = 0 ; i < numsamples+1 ; i ++) {
		sample[i] = 0 ; 
	}
	if (smooth_ == 1) {
		int mid = int(numsamples/2);
		for (i = 0; i < mid; i ++) {
			weights[i] = 1.0;
		}
		for (i = mid; i <= numsamples; i ++){
			weights[i] = 1.0 - (i-mid)/(mid + 1.0);
		}
	} else {
		int mid = int(numsamples/2);
		for (i = 0; i < mid; i ++) {
			weights[i] = 1.0;
		}
		for (i = mid; i <= numsamples; i ++){
			weights[i] = 1.0 - (i+1-mid)/(mid + 1.0);
		}
	}
	for (i = 0; i < numsamples+1; i ++) {
		mult[i] = 1.0 ; 
	}
	init_WALI_flag = 1;  /* initialization done */
}

///////////////////////////
// EWMA //////////////////
//////////////////////////

double TfrcSinkAgent::est_loss_EWMA () {
	double p1, p2 ;
	for (int i = last_sample; i <= maxseq ; i ++) {
		loss_int++; 
		if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST ) {
			if (avg_loss_int < 0) {
				avg_loss_int = loss_int ; 
			} else {
				avg_loss_int = history*avg_loss_int + (1-history)*loss_int ;
			}
			loss_int = 0 ;
		}
	}
	last_sample = maxseq+1 ; 

	if (avg_loss_int < 0) { 
		p1 = 0;
	} else {
		p1 = 1.0/avg_loss_int ; 
	}
	if (loss_int == 0) {
		p2 = p1 ; 
	} else {
		p2 = 1.0/(history*avg_loss_int + (1-history)*loss_int) ;
	}
	if (p2 < p1) {
		p1 = p2 ; 
	}
	if (printLoss_ > 0) {
		if (p1 > 0) 
			print_loss(loss_int, 1.0/p1);
		else
			print_loss(loss_int, 0.00001);
		print_loss_all(sample);
	}
	return p1 ;
}

///////////////////////////
// RBPH //////////////////
//////////////////////////
double TfrcSinkAgent::est_loss_RBPH () {

	double numpkts = hsz ;
	double p ; 

	// how many pkts we should go back?
	if (sendrate > 0 && rtt_ > 0) {
		double x = b_to_p(sendrate, rtt_, tzero_, psize_, 1);
		if (x > 0) 
			numpkts = minlc/x ; 
		else
			numpkts = hsz ;
	}

	// that number must be below maxseq and hsz 
	if (numpkts > maxseq)
		numpkts = maxseq ;
	if (numpkts > hsz)
		numpkts = hsz ;

	int lc = 0;
	int pc = 0;
	int i = maxseq ;

	// first see if how many lc's we find in numpkts 
	while (pc < numpkts) {
		pc ++ ;
		if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST )
			lc ++ ; 
		i -- ;
	}

	// if not enough lsos events, keep going back ...
	if (lc < minlc) {

		// but only as far as the history allows ...
		numpkts = maxseq ;
		if (numpkts > hsz)
			numpkts = hsz ;

		while ((lc < minlc) && (pc < numpkts)) {
			pc ++ ;
			if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST )
				lc ++ ;
			i -- ;
		
		}
	}

	if (pc == 0) 
		p = 0; 
	else
		p = (double)lc/(double)pc ; 
	if (printLoss_ > 0) {
		if (p > 0) 
			print_loss(0, 1.0/p);
		else
			print_loss(0, 0.00001);
		print_loss_all(sample);
	}
	return p ;
}

///////////////////////////
// EBPH //////////////////
//////////////////////////
double TfrcSinkAgent::est_loss_EBPH () {

	double numpkts = hsz ;
	double p ; 

	int lc = 0;
	int pc = 0;
	int i = maxseq ;

	numpkts = maxseq ;
	if (numpkts > hsz)
		numpkts = hsz ;

	while ((lc < minlc) && (pc < numpkts)) {
		pc ++ ;
		if (lossvec_[i%hsz] == LOST || lossvec_[i%hsz] == ECNLOST)
			lc ++ ;
		i -- ;
	}

	if (pc == 0) 
		p = 0; 
	else
		p = (double)lc/(double)pc ; 
	if (printLoss_ > 0) {
		if (p > 0) 
			print_loss(0, 1.0/p);
		else
			print_loss(0, 0.00001);
		print_loss_all(sample);
	}
	return p ;
}
