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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/red.cc,v 1.16 1997/07/16 23:19:26 sfloyd Exp $ (LBL)";
#endif

#include "red.h"

static class REDClass : public TclClass {
public:
	REDClass() : TclClass("Queue/RED") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new REDQueue);
	}
} class_red;

REDQueue::REDQueue() : link_(NULL), bcount_(0), de_drop_(NULL), idle_(1)
{
	memset(&edp_, '\0', sizeof(edp_));
	memset(&edv_, '\0', sizeof(edv_));

	bind_bool("bytes_", &edp_.bytes);		// boolean: use bytes?
	bind_bool("queue-in-bytes_", &qib_);		// boolean: q in bytes?
	bind("thresh_", &edp_.th_min);			// minthresh
	bind("maxthresh_", &edp_.th_max);		// maxthresh
	bind("mean_pktsize_", &edp_.mean_pktsize);	// avg pkt size
	bind("q_weight_", &edp_.q_w);			// for EWMA
	bind_bool("wait_", &edp_.wait);
	bind("linterm_", &edp_.max_p_inv);
	bind_bool("setbit_", &edp_.setbit);
	bind_bool("drop-tail_", &drop_tail_);
	bind_bool("fracthresh_", &edp_.fracthresh);
	bind("fracminthresh_", &edp_.frac_th_min);			// frac minthresh
	bind("fracmaxthresh_", &edp_.frac_th_max);		// frac maxthresh

	bind_bool("doubleq_", &doubleq_);
	bind("dqthresh_", &dqthresh_);

	q_ = new PacketQueue();
	reset();
#ifdef notdef
print_edp();
print_edv();
#endif
}

void REDQueue::reset()
{
	if (qib_) {
		edp_.th_min *= edp_.mean_pktsize;
		edp_.th_max *= edp_.mean_pktsize;
	}
	if (link_)
		edp_.ptc = link_->bandwidth() /
			(8. * edp_.mean_pktsize);
	if (edp_.fracthresh) {
		edp_.th_min = edp_.frac_th_min*qlim_;
		edp_.th_max = edp_.frac_th_max*qlim_;
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
	Packet *p;
	p = deque(q());
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
		// THIS IS A FORCED PACKET DROP.
		edv_.v_prob = 1.0;
	} else {
	        // THIS IS AN UNFORCED PACKET DROP.
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
	/* Trace RED queue parameters. */
	if (channel_) {
		char wrk[500];
		int n;
		double now = Scheduler::instance().clock();
		sprintf(wrk, "time: %-6.3f  v_ave: %-6.3f  v_prob: %-6.3f  count: %2d\n", now, edv_.v_ave, edv_.v_prob, edv_.count);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(channel_, wrk, n+1);
		wrk[n] = 0;
	}

// drop probability is computed, pick random number and act
	double u = Random::uniform();
	if (u <= edv_.v_prob) {
		edv_.count = 0;
		edv_.count_bytes = 0;
		if (edp_.setbit) {
			hdr_flags* hf = (hdr_flags*)pkt->access(off_flags_);
			hf->ecn_to_echo_ = 1; 
		} else {
			return (1);
		}
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

	run_estimator(qib_ ? bcount_ : q()->length(), m + 1);

	++edv_.count;
	edv_.count_bytes += ch->size();

	/*
	 * if average exceeds the min threshold, we may drop
	 */
	if (edv_.v_ave >= edp_.th_min && q()->length() > 1) { 
		if (edv_.old == 0) {
			edv_.count = 1;
			edv_.count_bytes = ch->size();
			edv_.old = 1;
		} else {
			/*
			 * Drop each packet with probability edv.v_prob.
			 */
			if (drop_early(pkt)) {
				// this could be either a forced or an
				// unforced packet drop, depending on
				// whether the ave queue size exceeds
				// maxthresh
				if (de_drop_ != NULL)
					de_drop_->recv(pkt);
				else
					drop(pkt);
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
		enque(q(), pkt);
		bcount_ += ch->size();
		int metric = qib_ ? bcount_ : q()->length();
		int limit = qib_ ?
			(qlim_ * edp_.mean_pktsize) : qlim_;
		if (metric > limit) {
			int victim;
			if (drop_tail_)
				victim = q()->length() - 1;
			else
				victim = Random::integer(q()->length());
				
			pkt = q()->lookup(victim);
			remove(q(), pkt);
			bcount_ -= ((hdr_cmn*)pkt->access(off_cmn_))->size_;
			drop(pkt);
			// forced packet drop because of queue overflow
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
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
                if (strcmp(argv[1], "early-drop-target") == 0) {
                        if (de_drop_ != NULL)
                                tcl.resultf("%s", de_drop_->name());
                        return (TCL_OK);
                }
	} else if (argc == 3) {
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("RED: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			// set ptc now
			link_ = del;
			edp_.ptc = link_->bandwidth() /
			    (8. * edp_.mean_pktsize);

			return (TCL_OK);
		}
                if (strcmp(argv[1], "early-drop-target") == 0) {
                        NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
                        if (p == 0) {
                                tcl.resultf("no object %s", argv[2]);
                                return (TCL_ERROR);
                        }
                        de_drop_ = p;
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
