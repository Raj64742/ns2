/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Here is one set of parameters from one of my simulations:
 * 
 * ed [ q_weight=0.002 thresh=5 linterm=30 maxthresh=15
 *         mean_pktsize=500 dropmech=random-drop queue-size=60
 *         plot-file=none bytes=false doubleq=false dqthresh=50 
 *	   wait=true ]
 * 
 * 1/"linterm" is the max probability of dropping a packet. 
 * There are different options that make the code
 * more messy that it would otherwise be.  For example,
 * "doubleq" and "dqthresh" are for a queue that gives priority to
 *   small (control) packets, 
 * "bytes" indicates whether the queue should be measured in bytes 
 *   or in packets, 
 * "dropmech" indicates whether the drop function should be random-drop 
 *   or drop-tail when/if the queue overflows, and 
 *   the commented-out Holt-Winters method for computing the average queue 
 *   size can be ignored.
 * "wait" indicates whether the gateway should wait between dropping
 *   packets.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/red.cc,v 1.9.2.2 1997/04/26 01:00:33 padmanab Exp $ (LBL)";
#endif

#include <math.h>
#include <string.h>
#include <sys/types.h>

#include "queue.h"
#include "Tcl.h"
#include "packet.h"
#include "random.h"
#include "flags.h"

/*
 * Early drop parameters, supplied by user
 */
struct edp {
	/*
	 * User supplied.
	 */
	int mean_pktsize;	/* avg pkt size, linked into Tcl */
	int bytes;		/* true if queue in bytes, false if packets */
	int wait;		/* true for waiting between dropped packets */
	int setbit;		/* true to set congestion indication bit */
	int fracthresh;         /* true if the thresholds are specified as 
				   fractions of the max. queue length */

	double th_min;		/* minimum threshold of average queue size */
	double th_max;		/* maximum threshold of average queue size */
	double max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */
	double q_w;		/* queue weight */
	/*
	 * Computed as a function of user supplied paramters.
	 */
	double ptc;		/* packet time constant in packets/second */
	/* misc. variables */
	int adjusted_for_fracthresh_; /* whether we have adjust for threshold
					 expressed as fractions of the queue size */
	int adjusted_for_bytes_;      /* whether we have adjusted for byte-counting
					 instead of packet-counting */
};

/*
 * Early drop variables, maintained by RED
 */
struct edv {
	float v_ave;		/* average queue size */
	float v_slope;		/* used in computing average queue size */
	float v_r;			
	float v_prob;		/* prob. of packet drop */
	float v_prob1;		/* prob. of packet drop before "count". */
	float v_a;		/* v_prob = v_a * v_ave + v_b */
	float v_b;
	int count;		/* # of packets since last drop */
	int count_bytes;	/* # of bytes since last drop */
	int old;		/* 0 when average queue first exceeds thresh */
};

class REDQueue : public Queue {
 public:	
	REDQueue();
 protected:
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	Packet* deque();
	void reset();
	void run_estimator(int nqueued, int m);
	void plot();
	void plot1(int qlen);
	int drop_early(Packet* pkt);
	int bcount_;

	/*
	 * Static state.
	 */
	int drop_tail_;
	edp edp_;
	int doubleq_;	/* for experiments with priority for small packets */
	int dqthresh_;	/* for experiments with priority for small packets */

	/*
	 * Dynamic state.
	 */
	int idle_;
	double idletime_;
	edv edv_;
	void print_edp();
	void print_edv();
};

static class REDClass : public TclClass {
public:
	REDClass() : TclClass("Queue/RED") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new REDQueue);
	}
} class_red;

REDQueue::REDQueue()
{
	bind("bytes_", &edp_.bytes);			// boolean: use bytes?
	bind("thresh_", &edp_.th_min);			// minthresh
	bind("maxthresh_", &edp_.th_max);		// maxthresh
	bind("mean_pktsize_", &edp_.mean_pktsize);	// avg pkt size
	bind("ptc_", &edp_.ptc);			// pkt time constant
	bind("q_weight_", &edp_.q_w);			// for EWMA
	bind_bool("wait_", &edp_.wait);
	bind("linterm_", &edp_.max_p_inv);
	bind_bool("setbit_", &edp_.setbit);
	bind_bool("drop-tail_", &drop_tail_);
	bind_bool("fracthresh_", &edp_.fracthresh);

	bind_bool("doubleq_", &doubleq_);
	bind("dqthresh_", &dqthresh_);

	edp_.adjusted_for_fracthresh_ = 0;
	edp_.adjusted_for_bytes_ = 0;

	reset();

#ifdef notdef
print_edp();
print_edv();
#endif
}

void REDQueue::reset()
{
	if (edp_.fracthresh && !edp_.adjusted_for_fracthresh_) {
		edp_.th_min *= qlim_;
		edp_.th_max *= qlim_;
		edp_.adjusted_for_fracthresh_ = 1;
	}

	if (edp_.bytes && !edp_.adjusted_for_bytes_) {
		edp_.th_min *= edp_.mean_pktsize;
		edp_.th_max *= edp_.mean_pktsize;
		edp_.adjusted_for_bytes_ = 1;
	}
	edv_.v_ave = 0.0;
	edv_.v_slope = 0.0;
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.old = 0;
	edv_.v_a = 1 / (edp_.th_max - edp_.th_min);
	edv_.v_b = - edp_.th_min / (edp_.th_max - edp_.th_min);

	idle_ = 1;
	if (&Scheduler::instance() != NULL)
		idletime_ = Scheduler::instance().clock();
	else
		idletime_ = 0.0; /* sched not instantiated yet */
	Queue::reset();

	bcount_ = 0;

}

/*
 * Compute the average queue size.
 * The code contains two alternate methods for this, the plain EWMA
 * and the Holt-Winters method.
 * nqueued can be bytes or packets
 */
void REDQueue::run_estimator(int nqueued, int m)
{
	float f, f_sl, f_old;

	f = edv_.v_ave;
	f_sl = edv_.v_slope;
#define RED_EWMA
#ifdef RED_EWMA
        while (--m >= 1) {
                f_old = f;
                f *= 1.0 - edp_.q_w;
        }
        f_old = f;
        f *= 1.0 - edp_.q_w;
        f += edp_.q_w * nqueued;
#endif
#ifdef RED_HOLT_WINTERS
	while (--m >= 1) {
		f_old = f;
		f += f_sl;
		f *= 1.0 - edp_.q_w;
		f_sl *= 1.0 - 0.5 * edp_.q_w;
		f_sl += 0.5 * edp_.q_w * (f - f_old);
	}
	f_old = f;
	f += f_sl;
	f *= 1.0 - edp_.q_w;
	f += edp_.q_w * nqueued;
	f_sl *= 1.0 - 0.5 * edp_.q_w;
	f_sl += 0.5 * edp_.q_w * (f - f_old);
#endif
	edv_.v_ave = f;
	edv_.v_slope = f_sl;
}

/*
 * Output the average queue size and the dropping probability. 
 */
void REDQueue::plot()
{
	double t = Scheduler::instance().clock();
#ifdef notyet
	sprintf(trace_->buffer(), "a %g %g", t, edv_.v_ave);
	trace_->dump();
	sprintf(trace_->buffer(), "p %g %g", t, edv_.v_prob);
	trace_->dump();
#endif
}

/*
 * print the queue seen by arriving packets only
 */
void REDQueue::plot1(int length)
{
        double t =  Scheduler::instance().clock();
#ifdef notyet
        sprintf(trace_->buffer(), "Q %g %d", t, length);
	trace_->dump();
#endif
}

/*
 * Return the next packet in the queue for transmission.
 */
Packet* REDQueue::deque()
{
	Packet* p = q_.deque();
	if (p != 0) {
		idle_ = 0;
		bcount_ -= ((hdr_cmn*)p->access(off_cmn_))->size_;
	} else {
		idle_ = 1;
		// deque() may invoked by Queue::reset at init
		// time (before the scheduler is instantiated).
		// deal with this case
		if (&Scheduler::instance() != NULL)
			idletime_ = Scheduler::instance().clock();
		else
			idletime_ = 0.0;
	}
	return (p);
}
		
int REDQueue::drop_early(Packet* pkt)
{
	hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
	if (edv_.v_ave >= edp_.th_max) {
		// policy: if above max thresh, force drop
		edv_.v_prob = 1.0;
	} else {
		double p = edv_.v_a * edv_.v_ave + edv_.v_b;
		p /= edp_.max_p_inv;
		edv_.v_prob1 = p;
		if (edv_.v_prob1 > 1.0)
			edv_.v_prob1 = 1.0;
		double count1 = edv_.count;
		if (edp_.bytes)
			count1 = (double) (edv_.count_bytes/edp_.mean_pktsize);
		if (edp_.wait) {
			if (count1 * p < 1.0)
				p = 0.0;
			else if (count1 * p < 2.0)
				p /= (2 - count1 * p);
			else
				p = 1.0;
		} else if (!edp_.wait) {
			if (count1 * p < 1.0)
				p /= (1.0 - count1 * p);
			else
				p = 1.0;
		}
		if (edp_.bytes && p < 1.0) {
			p = p * ch->size() / edp_.mean_pktsize;
		}
		if (p > 1.0)
			p = 1.0;
		edv_.v_prob = p;
	}
	{
		double now = Scheduler::instance().clock();
		if (now >= 3.0 && now <= 5.0) 
			printf("time: %g  v_ave: %f  v_prob: %f  count: %d\n", now, edv_.v_ave, edv_.v_prob, edv_.count);
	}

	// drop probability is computed, pick random number and act
	double u = Random::uniform();
	if (u <= edv_.v_prob) {
		edv_.count = 0;
		edv_.count_bytes = 0;
		if (edp_.setbit) {
			hdr_flags* hf = (hdr_flags*)pkt->access(off_flags_);
			hf->ecn_to_echo_ = 1;
		} else
			return (1);
	}
	return (0);
}

/*
 * Receive a new packet arriving at the queue.
 * The average queue size is computed.  If the average size
 * exceeds the threshold, then the dropping probability is computed,
 * and the newly-arriving packet is dropped with that probability.
 * The packet is also dropped if the maximum queue size is exceeded.
 */
void REDQueue::enque(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
	int m;

        if (idle_) {
		/* To account for the period when the queue was empty.  */
                idle_ = 0;
		m = int(edp_.ptc * (now - idletime_));
        } else
                m = 0;

	run_estimator(edp_.bytes ? bcount_ : q_.length(), m + 1);

	++edv_.count;
	edv_.count_bytes += ch->size();

	/*
	 * if average exceeds the min threshold, we may drop
	 */
	if (edv_.v_ave >= edp_.th_min && q_.length() > 1) { 
		if (edv_.old == 0) {
			edv_.count = 1;
			edv_.count_bytes = ch->size();
			edv_.old = 1;
		} else {
			/*
			 * Drop each packet with probability edv.v_prob.
			 */
			if (drop_early(pkt)) {
				drop(pkt);	// shouldn't this be here??-K
				pkt = 0;
			}
		}
	} else {
		edv_.v_prob = 0.0;
		edv_.old = 0;
	}
	/*
	 * If we didn't drop the packet above, send it to the interface,
	 * checking for absolute queue overflow.
	 */
	if (pkt != 0) {
		q_.enque(pkt);
		bcount_ += ch->size();
		int metric = edp_.bytes ? bcount_ : q_.length();
		int limit = edp_.bytes ?
			(qlim_ * edp_.mean_pktsize) : qlim_;
		if (metric > limit) { /* XXXX changed from qlim_ to limit */  
			int victim;
			if (drop_tail_)
				victim = q_.length() - 1;
			else
				victim = Random::integer(q_.length());
				
			pkt = q_.lookup(victim);
			q_.remove(pkt);
			bcount_ -= ((hdr_cmn*)pkt->access(off_cmn_))->size_;
			drop(pkt);
		}
	}
#ifdef notyet
	if (trace_)
		plot();
#endif

	return;	/* end of enque() */
}
int REDQueue::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}
/* for debugging help */
void REDQueue::print_edp()
{
        printf("mean_pktsz: %d\n", edp_.mean_pktsize); 
        printf("bytes: %d, wait: %d, setbit: %d\n",
                edp_.bytes, edp_.wait, edp_.setbit);
        printf("minth: %f, maxth: %f\n", edp_.th_min, edp_.th_max);
        printf("max_p_inv: %f, qw: %f, ptc: %f\n",
                edp_.max_p_inv, edp_.q_w, edp_.ptc);
	printf("qlim: %d, idletime: %f\n", qlim_, idletime_);
	printf("=========\n");
}

void REDQueue::print_edv()
{
	printf("v_a: %f, v_b: %f\n", edv_.v_a, edv_.v_b);
}
