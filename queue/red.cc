/*
 * Copyright (c) 1990-1994 Regents of the University of California.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/red.cc,v 1.1.1.1 1996/12/19 03:22:45 mccanne Exp $ (LBL)";
#endif

#include <math.h>
#include <string.h>
#include <sys/types.h>

#include "queue.h"
#include "Tcl.h"
#include "packet.h"
#include "random.h"

/*
 * Early drop parameters, supplied by user
 */
struct edp {
	/*
	 * User supplied.
	 */
	int mean_pktsize;
	int bytes;		/* true if queue in bytes, false if packets */
	int wait;		/* true for waiting between dropped packets */
	int setbit;		/* true to set congestion indication bit */

	double th_min;		/* minimum threshold of average queue size */
	double th_max;		/* maximum threshold of average queue size */
	double max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */
	double q_w;		/* queue weight */
	/*
	 * Computed as a function of user supplied paramters.
	 */
	double ptc;		/* packet time constant in packets/second */
};

/*
 * Early drop variables, maintained by RED
 */
struct edv {
	float v_ave;		/* average queue size */
	float v_slope;		/* used in computing average queue size */
	float v_r;			
	float v_prob;		/* prob. of packet drop */
	float v_a;		/* v_prob = v_a * v_ave + v_b */
	float v_b;
	int count;		/* # of packets since last drop */
	int old;		/* 0 when average queue first exceeds thresh */
};

class REDQueue : public Queue {
 public:	
	REDQueue();
 protected:
	void enque(Packet* pkt);
	Packet* deque();
	virtual void reset();

	void run_estimator(int nqueued, int m);
	void plot();
	void plot1(int qlen);
	int drop_early(Packet* pkt);

	PacketQueue q_;
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
};

static class REDClass : public TclClass {
public:
	REDClass() : TclClass("queue/red") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new REDQueue);
	}
} class_red;

REDQueue::REDQueue()
{
	bind("bytes", &edp_.bytes);
	bind("thresh", &edp_.th_min);
	bind("maxthresh", &edp_.th_max);
	bind("mean_pktsize", &edp_.mean_pktsize);
	bind("q_weight", &edp_.q_w);
	bind_bool("wait", &edp_.wait);
	bind("linterm", &edp_.max_p_inv);
	bind_bool("setbit", &edp_.setbit);
	bind_bool("drop-tail", &drop_tail_);

	bind_bool("doubleq", &doubleq_);
	bind("dqthresh", &dqthresh_);

	reset();
}

void REDQueue::reset()
{
#ifdef notdef
Link::reset();
	/*
	 * Packet time constant -- units are pkts/sec.
	 */
	edp_.ptc = ( bandwidth_ / ( 8. * edp_.mean_pktsize ));
#endif

	edv_.v_ave = 0.0;
	edv_.v_slope = 0.0;
	edv_.count = 0;
	edv_.old = 0;
	edv_.v_a = 1 / (edp_.th_max - edp_.th_min);
	edv_.v_b = - edp_.th_min / (edp_.th_max - edp_.th_min);

	idle_ = 1;
#ifdef notdef
	idletime_ = Scheduler::instance().clock();
#endif
	idletime_ = 0.0;	/* sched not instantiated yet */
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
#ifdef notyet
		log_packet_departure(p);
#endif
	} else {
		idle_ = 1;
		idletime_ = Scheduler::instance().clock();
	}
	return (p);
}
		
int REDQueue::drop_early(Packet* pkt)
{
	if (edv_.v_ave >= edp_.th_max) {
		// policy: if above max thresh, force drop
		edv_.v_prob = 1.0;
	} else {
		double p = edv_.v_a * edv_.v_ave + edv_.v_b;
		p /= edp_.max_p_inv;
		if (edp_.bytes)
			p *= pkt->size_ / edp_.mean_pktsize;
		if (edp_.wait) {
			if (edv_.count * p < 1)
				p = 0;
			else if (edv_.count * p < 2)
				p /= (2 - edv_.count * p);
			else
				p = 1;
		} else if (!edp_.wait) {
			if (edv_.count * p < 1)
				p /= (1 - edv_.count * p);
			else
				p = 1;
		}
		edv_.v_prob = p;
	}
	// drop probability is computed, pick random number and act
	double u = Random::uniform();
	if (u <= edv_.v_prob) {
		edv_.count = 0;
		if (edp_.setbit) 
			pkt->flags_ |= PF_ECN;	// congestion bit in pkt
		else
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
	int m;
        if (idle_) {
		/* To account for the period when the queue was empty.  */
                idle_ = 0;
		m = int(edp_.ptc * (now - idletime_));
        } else
                m = 0;

	run_estimator(edp_.bytes ? q_.bcount() : q_.length(), m + 1);

#ifdef notyet
	if (trace_)
		plot1(qnp_);	// current queue size (# packets)
#endif

	++edv_.count;

	/*
	 * if average exceeds the min threshold, we may drop
	 */
	if (edv_.v_ave >= edp_.th_min ) { 
		if (edv_.old == 0) {
			edv_.count = 1;
			edv_.old = 1;
		} else {
			/*
			 * Drop each packet with probability edv.v_prob.
			 */
			if (drop_early(pkt)) {
#ifdef notyet
				log_packet_arrival(pkt);
				log_packet_drop(pkt);
#endif
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
#ifdef notyet
		log_packet_arrival(pkt);
#endif
		int metric = edp_.bytes ? q_.bcount() : q_.length();
		if (metric > qlim_) {
			int victim;
			if (drop_tail_)
				victim = q_.length() - 1;
			else
				victim = Random::integer(q_.length());
				
			pkt = q_.lookup(victim);
			q_.remove(pkt);
#ifdef notyet
			log_packet_drop(pkt);
#endif
			drop(pkt);
		}
	}
#ifdef notyet
	if (trace_)
		plot();
#endif

	return;	/* end of enque() */
}
