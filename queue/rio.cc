/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * Here is one set of parameters from one of Sally's simulations
 * (this is from tcpsim, the older simulator):
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/rio.cc,v 1.1 2000/06/21 02:41:41 sfloyd Exp $ (LBL)";
#endif

#include "rio.h"

static class RIOClass : public TclClass {
public:
	RIOClass() : TclClass("Queue/RIO") {}
	TclObject* create(int, const char*const*) {
		return (new RIOQueue);
	}
} class_rio;

RIOQueue::RIOQueue() : link_(NULL), bcount_(0), in_len_(0), in_bcount_(0),
	de_drop_(NULL), tchan_(0), idle_(1), in_idle_(1)
{
	bind_bool("bytes_", &edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &qib_);	    // boolean: q in bytes?
	bind("thresh_", &edp_.total_min);	    // minthresh
	bind("maxthresh_", &edp_.total_max);	    // maxthresh
	bind("in_thresh_", &edp_.in_th_min);	    // In_minthresh
	bind("in_maxthresh_", &edp_.in_th_max);	    // In_maxthresh
	bind("out_thresh_", &edp_.out_th_min);	    // Out_minthresh
	bind("out_maxthresh_", &edp_.out_th_max);   // Out_maxthresh
	bind("mean_pktsize_", &edp_.mean_pktsize);  // avg pkt size
	bind("q_weight_", &edp_.q_w);		    // for EWMA
	bind_bool("wait_", &edp_.wait);
	bind("linterm_", &edp_.max_p_inv);
	bind("in_linterm_", &edp_.in_max_p_inv);
	bind_bool("setbit_", &edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_", &edp_.gentle);         // increase the packet
						    // drop prob. slowly
						    // when ave queue
						    // exceeds maxthresh

	bind_bool("drop_tail_", &drop_tail_);	    // drop last pkt
	bind_bool("drop_front_", &drop_front_);	    // drop first pkt
	bind_bool("drop_rand_", &drop_rand_);	    // drop pkt at random
	bind_bool("ns1_compat_", &ns1_compat_);	    // ns-1 compatibility

	bind("ave_", &edv_.v_ave);		    // average queue sie
	bind("in_ave_", &edv_.in_v_ave);	    // In average queue sie
	bind("out_ave_", &edv_.out_v_ave);	    // Out average queue sie
	bind("prob1_", &edv_.v_prob1);		    // dropping probability
	bind("in_prob1_", &edv_.in_v_prob1);	    // In dropping probability
	bind("out_prob1_", &edv_.out_v_prob1);	    // Out dropping probability
	bind("curq_", &curq_);			    // current queue size

	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	reset();
#ifdef notdef
print_edp();
print_edv();
#endif

}

void RIOQueue::reset()
{
	/*
	 * If queue is measured in bytes, scale min/max thresh
	 * by the size of an average packet (which is specified by user).
	 */

	if (qib_) {
		edp_.in_th_min *= edp_.mean_pktsize;
		edp_.in_th_max *= edp_.mean_pktsize;

		edp_.out_th_min *= edp_.mean_pktsize;
		edp_.out_th_max *= edp_.mean_pktsize;
	}

	/*
	 * Compute the "packet time constant" if we know the
	 * link bandwidth.  The ptc is the max number of (avg sized)
	 * pkts per second which can be placed on the link.
	 * The link bw is given in bits/sec, so scale mean psize
	 * accordingly.
	 */
	 
	if (link_)
		edp_.ptc = link_->bandwidth() /
			(8. * edp_.mean_pktsize);

	edv_.v_ave = 0.0;
	edv_.v_slope = 0.0;
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.old = 0;
	edv_.v_a = 1 / (edp_.total_max - edp_.total_min);
	edv_.v_b = - edp_.total_min / (edp_.total_max - edp_.total_min);
	if (edp_.gentle) {
		edv_.v_c = ( 1.0 - 1 / edp_.max_p_inv ) / edp_.total_max;
		edv_.v_d = 2 / edp_.max_p_inv - 1.0;
	}

        /* Added by Wenjia */
        edv_.in_v_ave = 0.0;
        edv_.in_v_slope = 0.0;
        edv_.in_drops = 0;
        edv_.in_count = 0;
        edv_.in_count_bytes = 0;
        edv_.in_old = 0;
        edv_.in_v_a = 1 / (edp_.in_th_max - edp_.in_th_min);
        edv_.in_v_b = - edp_.in_th_min / (edp_.in_th_max - edp_.in_th_min);

        /* Added by Wenjia */
        edv_.out_v_ave = 0.0;
        edv_.out_v_slope = 0.0;
        edv_.out_drops = 0;
        edv_.out_count = 0;
        edv_.out_count_bytes = 0;
        edv_.out_old = 0;
        edv_.out_v_a = 1 / (edp_.out_th_max - edp_.out_th_min);
        edv_.out_v_b = - edp_.out_th_min / (edp_.out_th_max - edp_.out_th_min);

	idle_ = 1;
	in_idle_ = 1;
	if (&Scheduler::instance() != NULL)
		idletime_ = Scheduler::instance().clock();
	else {
		idletime_ = 0.0; /* sched not instantiated yet */
		in_idletime_ = 0.0; /* sched not instantiated yet */
             }
	Queue::reset();

	bcount_ = 0;
}

/*
 * Compute the average queue size.
 * The code contains two alternate methods for this, the plain EWMA
 * and the Holt-Winters method.
 * nqueued can be bytes or packets
 */
void RIOQueue::run_in_estimator(int in_queued, int total_queued, int m)
{
	float in_f, in_f_sl, in_f_old;
        float total_f, total_f_sl, total_f_old;

	in_f = (float)edv_.in_v_ave;
	in_f_sl = (float)edv_.in_v_slope;
#define RED_EWMA
#ifdef RED_EWMA
	while (--m >= 1) {
		in_f_old = in_f;
		in_f *= 1.0 - (float)edp_.q_w;

                total_f_old = total_f;
                total_f *= 1.0 - (float)edp_.q_w;
	}
	in_f_old = in_f;
	in_f *= 1.0 - (float)edp_.q_w;
	in_f += (float)edp_.q_w * in_queued;

        total_f_old = total_f;
        total_f *= 1.0 - (float)edp_.q_w;
        total_f += edp_.q_w * total_queued;
#endif
#ifdef RED_HOLT_WINTERS
	while (--m >= 1) {
		in_f_old = in_f;
		in_f += in_f_sl;
		in_f *= 1.0 - edp_.q_w;
		in_f_sl *= 1.0 - 0.5 * edp_.q_w;
		in_f_sl += 0.5 * edp_.q_w * (in_f - in_f_old);

                total_f_old = total_f;
                total_f += total_f_sl;
                total_f *= 1.0 - edp_.q_w;
                total_f_sl *= 1.0 * 0.5 * edp_.q_w;
                total_f_sl *= 0.5 * edp_.q_w * (total_f - total_f_old);
	}
	in_f_old = in_f;
	in_f += in_f_sl;
	in_f *= 1.0 - edp_.q_w;
	in_f += edp_.q_w * in_queued;
	in_f_sl *= 1.0 - 0.5 * edp_.q_w;
	in_f_sl += 0.5 * edp_.q_w * (in_f - in_f_old);

        total_f_old = total_f;
        total_f += total_f_sl;
        total_f *= 1.0 - edp_.q_w;
        total_f += edp_.q_w * total_queued;
        total_f_sl *= 1.0 - 0.5 * edp_.q_w;
        total_f_sl += 0.5 * edp_.q_w * (total_f - total_f_old);

#endif
	edv_.in_v_ave = in_f;
	edv_.in_v_slope = in_f_sl;

	//for debug
	//printf("%f %f\n", Scheduler::instance().clock(), edv_.in_v_ave);
        edv_.v_ave  = total_f;
        edv_.v_slope = total_f_sl;
	//for debug
	//printf("total %f %f\n", Scheduler::instance().clock(), total_f);
}

/*
 * Compute the average queue size and average Out queue size.
 * The code contains two alternate methods for this, the plain EWMA
 * and the Holt-Winters method.
 * out_queued and total_queued can be bytes or packets
 */
void RIOQueue::run_out_estimator( int out_queued, int total_queued, int m)
{
        float out_f, out_f_sl, out_f_old;
        float total_f, total_f_sl, total_f_old ;

        out_f = edv_.out_v_ave;
        out_f_sl = edv_.out_v_slope;

        total_f = edv_.v_ave;
        total_f_sl = edv_.v_slope;

#define RED_EWMA

#ifdef RED_EWMA
        while (--m >= 1) {
                out_f_old = out_f;
                out_f *= 1.0 - edp_.q_w;

                total_f_old = total_f;
                total_f  *= 1.0 - edp_.q_w;
        }

        out_f_old = out_f;
        out_f *= 1.0 - edp_.q_w;
        out_f += edp_.q_w * out_queued;

        total_f_old = total_f;
        total_f    *= 1.0 - edp_.q_w;
        total_f    += edp_.q_w * total_queued;

#endif
#ifdef RED_HOLT_WINTERS
        while (--m >= 1) {
                out_f_old = out_f;
                out_f += out_f_sl;
                out_f *= 1.0 - edp_.q_w;
                out_f_sl *= 1.0 - 0.5 * edp_.q_w;
                out_f_sl += 0.5 * edp_.q_w * (out_f - out_f_old);

                total_f_old = total_f;
                total_f += total_f_sl;
                total_f *= 1.0 - edp_.q_w;
                total_f_sl *= 1.0 * 0.5 * edp_.q_w;
                total_f_sl *= 0.5 * edp_.q_w * (total_f - total_f_old);

        }

        out_f_old = out_f;
        out_f += out_f_sl;
        out_f *= 1.0 - edp_.q_w;
        out_f += edp_.q_w * out_queued;
        out_f_sl *= 1.0 - 0.5 * edp_.q_w;
        out_f_sl += 0.5 * edp_.q_w * (out_f - out_f_old);

        total_f_old = total_f;
        total_f += total_f_sl;
        total_f *= 1.0 - edp_.q_w;
        total_f += edp_.q_w * total_queued;
        total_f_sl *= 1.0 - 0.5 * edp_.q_w;
        total_f_sl += 0.5 * edp_.q_w * (total_f - total_f_old);

#endif
        edv_.out_v_ave = out_f;
        edv_.out_v_slope = out_f_sl;

	// for debug
	//printf("%f %f\n", Scheduler::instance().clock(), edv_.out_v_ave);

        edv_.v_ave  = total_f;
        edv_.v_slope = total_f_sl;
	//for debug
	//printf("total  out %f %f\n", Scheduler::instance().clock(), total_f);

}

/*
 * Return the next packet in the queue for transmission.
 */
Packet* RIOQueue::deque()
{
	Packet *p;
	p = q_->deque();
        // printf( "qlen %d %d\n", q_->length(), length());
	if (p != 0) {
		idle_ = 0;
		hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
                if (hf->pri_)   /* Regular In packets */
		{
                  in_idle_ = 0;
		  in_bcount_ -= ((hdr_cmn*)p->access(off_cmn_))->size();
		  --in_len_;
		}

		bcount_ -= ((hdr_cmn*)p->access(off_cmn_))->size();
	} else {
		idle_ = 1;
                in_idle_ = 1;
		// deque() may invoked by Queue::reset at init
		// time (before the scheduler is instantiated).
		// deal with this case
		if (&Scheduler::instance() != NULL) {
			idletime_ = Scheduler::instance().clock();
			in_idletime_ = idletime_;
                   }
		else {
			idletime_ = 0.0;
			in_idletime_ = 0.0;
                  }
	}
	return (p);
}

/*
 * should the packet be dropped/marked due to a probabilistic drop?
 */

int
RIOQueue::drop_in_early(Packet* pkt)
{
	hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);

	double p;

	if (edp_.gentle && edv_.v_ave >= edp_.in_th_max) {
		// p ranges from max_p to 1 as the average queue
		// size ranges from th_max to twice th_max 
		p = edv_.v_c * edv_.v_ave + edv_.v_d;
	} else {
		// p ranges from 0 to max_p as the average queue
		// size ranges from th_min to th_max 
		p = edv_.in_v_a * edv_.in_v_ave + edv_.in_v_b;
		p /= edp_.in_max_p_inv;
	}
	edv_.in_v_prob1 = p;
	if (edv_.in_v_prob1 > 1.0)
		edv_.in_v_prob1 = 1.0;
	double count1 = edv_.in_count;
	if (edp_.bytes)
		count1 = (double) (edv_.in_count_bytes/edp_.mean_pktsize);
	if (edp_.wait) {
		if (count1 * p < 1.0)
			p = 0.0;
		else if (count1 * p < 2.0)
			p /= (2 - count1 * p);
		else
			p = 1.0;
	} else {
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
	edv_.in_v_prob = p;

	// drop probability is computed, pick random number and act
	double u = Random::uniform();
	if (u <= edv_.in_v_prob) {
		// DROP or MARK
		edv_.in_count = 0;
		edv_.in_count_bytes = 0;
		hdr_flags* hf = 
                  (hdr_flags*)pickPacketForECN(pkt)->access(off_flags_);
		if (edp_.setbit && hf->ect()) {
			hf->ce() = 1; 	// mark Congestion Experienced bit
			return (0);	// no drop
		} else {
			return (1);	// drop
		}
	}
	return (0);			// no DROP/mark
}

/* The rationale here is that if the edv_.in_v_ave is close
 * to the edp_.in_th_max, (we are about to turn over to the
 * congestion control phase, presumably because of Out packets.
 * then we should drop Out packets more severely.
 */

int RIOQueue::drop_out_early(Packet* pkt)
{
        hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);

        if (edv_.v_ave >= edp_.out_th_max)
            /* this is the case when the total queue is building up
             */
            edv_.out_v_prob = 1.0;
        else {
	        double p = edv_.out_v_a * edv_.v_ave + edv_.out_v_b;
	        p /= edp_.out_max_p_inv;
       		edv_.out_v_prob1 = p;
	        if (edv_.out_v_prob1 > 1.0)
	                edv_.out_v_prob1 = 1.0;
	        double count1 = edv_.out_count;
	        if (edp_.bytes)
	           count1 = (double) (edv_.out_count_bytes/edp_.mean_pktsize);
	        if (edp_.wait) {
	                if (count1 * p < 1.0)
	                        p = 0.0;
	                else if (count1 * p < 2.0)
	                        p /= (2 - count1 * p);
	                else
	                        p = 1.0;
	        } else {
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
	        edv_.out_v_prob = p;
	}

        // drop probability is computed, pick random number and act
        double u = Random::uniform();
        if (u <= edv_.out_v_prob) {
           // DROP or MARK
           edv_.out_count = 0;
           edv_.out_count_bytes = 0;
           hdr_flags* hf = (hdr_flags*)pickPacketForECN(pkt)->access(off_flags_);
           if (edp_.setbit && hf->ecn_capable_) {
             hf->ecn_to_echo_ = 1;
             } else {
             return (1);
             }
        }
        return (0);  // no DROP/mark
}

/*
 * Pick packet for early congestion notification (ECN). This packet is then
 * marked or dropped. Having a separate function do this is convenient for
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the packet for 
 * ECN notification.
 */
Packet*
RIOQueue::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

/*
 * Pick packet to drop. Having a separate function do this is convenient for
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the victim.
 */
Packet*
RIOQueue::pickPacketToDrop() 
{
	int victim;

	if (drop_front_)
		victim = min(1, q_->length()-1);
	else if (drop_rand_)
		victim = Random::integer(q_->length());
	else			/* default is drop_tail_ */
		victim = q_->length() - 1;

	return(q_->lookup(victim)); 
}

/*
 * Receive a new packet arriving at the queue.
 * The average queue size is computed.  If the average size
 * exceeds the threshold, then the dropping probability is computed,
 * and the newly-arriving packet is dropped with that probability.
 * The packet is also dropped if the maximum queue size is exceeded.
 *
 * "Forced" drops mean a packet arrived when the underlying queue was
 * full or when the average q size exceeded maxthresh.
 * "Unforced" means a RED random drop.
 *
 * For forced drops, either the arriving packet is dropped or one in the
 * queue is dropped, depending on the setting of drop_tail_.
 * For unforced drops, the arriving packet is always the victim.
 */

#define	DTYPE_NONE	0	/* ok, no drop */
#define	DTYPE_FORCED	1	/* a "forced" drop */
#define	DTYPE_UNFORCED	2	/* an "unforced" (random) drop */

void RIOQueue::enque(Packet* pkt)
{
	double now = Scheduler::instance().clock();

        /* Duplicate the RED algorithm to carry out a separate
         * calculation for Out packets -- Wenjia */
	hdr_flags* hf = (hdr_flags*)pkt->access(off_flags_);

	//printf("RIOQueue::enque queue %d queue-length %d priority %d\n", 
	//       q_, q_->length(), hf->pri_);
        if (hf->pri_) {  /* Regular In packets */

	/*
	 * if we were idle, we pretend that m packets arrived during
	 * the idle period.  m is set to be the ptc times the amount
	 * of time we've been idle for
	 */

	int m = 0;
	if (in_idle_) {
		double now = Scheduler::instance().clock();
		/* To account for the period when the queue was empty. */
		in_idle_ = 0;
		m = int(edp_.ptc * (now - in_idletime_));
	}

	/*
	 * Run the estimator with either 1 new packet arrival, or with
	 * the scaled version above [scaled by m due to idle time]
	 * (bcount_ maintains the byte count in the underlying queue).
	 * If the underlying queue is able to delete packets without
	 * us knowing, then bcount_ will not be maintained properly!
	 */

        // printf( "qlen %d\n", q_->length());
	run_in_estimator(qib_ ? in_bcount_ : in_len_, 
                         qib_ ? bcount_ : q_->length(), m + 1);

	/*
	 * count and count_bytes keeps a tally of arriving traffic
	 * that has not been dropped (i.e. how long, in terms of traffic,
	 * it has been since the last early drop)
	 */

	hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
	++edv_.count;
	edv_.count_bytes += ch->size();

        /* added by Yun */
        ++edv_.in_count;
        edv_.in_count_bytes += ch->size();

	/*
	 * DROP LOGIC:
	 *	q = current q size, ~q = averaged q size
	 *	1> if ~q > maxthresh, this is a FORCED drop
	 *	2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
	 *	3> if (q+1) > hard q limit, this is a FORCED drop
	 */

	register double qavg = edv_.v_ave;
	register double in_qavg = edv_.in_v_ave;
	int droptype = DTYPE_NONE;
	int qlen = qib_ ? bcount_ : q_->length();
	int in_qlen = qib_ ? in_bcount_ : in_len_;
	int qlim = qib_ ? (qlim_ * edp_.mean_pktsize) : qlim_;

	curq_ = qlen;	// helps to trace queue during arrival, if enabled

	if (in_qavg >= edp_.in_th_min && in_qlen > 1) {
		if ((!edp_.gentle && in_qavg >= edp_.in_th_max) ||
			(edp_.gentle && in_qavg >= 2 * edp_.in_th_max)) {
			droptype = DTYPE_FORCED;
		} else if (edv_.in_old == 0) {
			/* 
			 * The average queue size has just crossed the
			 * threshold from below to above "minthresh", or
			 * from above "minthresh" with an empty queue to
			 * above "minthresh" with a nonempty queue.
			 */
			edv_.in_count = 1;
			edv_.in_count_bytes = ch->size();
			edv_.in_old = 1;
		} else if (drop_in_early(pkt)) {
			droptype = DTYPE_UNFORCED;
		}
	} else {
		/* No packets are being dropped.  */
		edv_.in_v_prob = 0.0;
		edv_.in_old = 0;		
	}
	if (qlen >= qlim) {
		// see if we've exceeded the queue size
		droptype = DTYPE_FORCED;
	}

	if (droptype == DTYPE_UNFORCED) {
		/* pick packet for ECN, which is dropping in this case */
		Packet *pkt_to_drop = pickPacketForECN(pkt);
		/* 
		 * If the packet picked is different that the one that just
		 * arrived, add it to the queue and remove the chosen packet.
		 */
		if (pkt_to_drop != pkt) {
			q_->enque(pkt);
                        // printf( "in: qlen %d %d\n", q_->length(), length());
                        ++in_len_;
			bcount_ += ch->size();
			in_bcount_ += ch->size();
			q_->remove(pkt_to_drop);
                        // printf("remove qlen %d %d\n",q_->length(),length());
			bcount_ -= 
                          ((hdr_cmn*)pkt_to_drop->access(off_cmn_))->size();
                        if(((hdr_flags*)pkt_to_drop->access(off_flags_))->pri_)
                           {
                             in_bcount_ -= 
                               ((hdr_cmn*)pkt_to_drop->access(off_cmn_))->size();
                             --in_len_;
                           }
			pkt = pkt_to_drop; /* ok 'cause pkt not needed anymore */
		}
		// deliver to special "edrop" target, if defined
		if (de_drop_ != NULL)
			de_drop_->recv(pkt);
		else
			drop(pkt);
	} else {
		/* forced drop, or not a drop: first enqueue pkt */
		q_->enque(pkt);
                // printf( "in: qlen %d %d\n", q_->length(), length());
                ++in_len_;
		bcount_ += ch->size();
		in_bcount_ += ch->size();

		/* drop a packet if we were told to */
		if (droptype == DTYPE_FORCED) {
			/* drop random victim or last one */
			pkt = pickPacketToDrop();
			q_->remove(pkt);
                        // printf("remove qlen %d %d\n",q_->length(),length());
			bcount_ -= ((hdr_cmn*)pkt->access(off_cmn_))->size();
                        if(((hdr_flags*)pkt->access(off_flags_))->pri_) {
                          in_bcount_ -= 
                            ((hdr_cmn*)pkt->access(off_cmn_))->size(); 
                          --in_len_;
                          }
			drop(pkt);
			if (!ns1_compat_) {
				// bug-fix from Philip Liu, <phill@ece.ubc.ca>
				edv_.count = 0;
				edv_.count_bytes = 0;
				edv_.in_count = 0;
				edv_.in_count_bytes = 0;
			}
		}
            }
        }

	else {     /* Out packets and default regular packets */
          /*
           * if we were idle, we pretend that m packets arrived during
           * the idle period.  m is set to be the ptc times the amount
           * of time we've been idle for
           */

          int m = 0;
          if (idle_) {
                  /* To account for the period when the queue was empty.  */
                  idle_ = 0;
                  m = int(edp_.ptc * (now - idletime_));
          }

          /*
           * Run the estimator with either 1 new packet arrival, or with
           * the scaled version above [scaled by m due to idle time]
           * (bcount_ maintains the byte count in the underlying queue).
           * If the underlying queue is able to delete packets without
           * us knowing, then bcount_ will not be maintained properly!
           */

		// not sure whether this is correct, Yun
	  //printf("qlen %d\n", q_->length());
          run_out_estimator(qib_ ? bcount_ - in_bcount_ : q_->length() - in_len_,
                        qib_ ? bcount_ : q_->length(), m + 1);
          /*
           * count and count_bytes keeps a tally of arriving traffic
           * that has not been dropped (i.e. how long, in terms of traffic,
           * it has been since the last early drop)
           */

          hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
          ++edv_.count;
          edv_.count_bytes += ch->size();

          /* added by Yun */
          ++edv_.out_count;
          edv_.out_count_bytes += ch->size();

          /*
           * DROP LOGIC:
           *    q = current q size, ~q = averaged q size
           *    1> if ~q > maxthresh, this is a FORCED drop
           *    2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
           *    3> if (q+1) > hard q limit, this is a FORCED drop
           */

                /* if the average queue is below the out_th_min
                 * then we have no need to worry.
                 */

          register double qavg = edv_.v_ave;
          // register double in_qavg = edv_.in_v_ave;
          int droptype = DTYPE_NONE;
          int qlen = qib_ ? bcount_ : q_->length();
          /* added by Yun, seems not useful */
          // int out_qlen = qib_ ? out_bcount_ : q_->out_length();

          int qlim = qib_ ?  (qlim_ * edp_.mean_pktsize) : qlim_;

          curq_ = qlen; // helps to trace queue during arrival, if enabled

          if (qavg >= edp_.out_th_min && qlen > 1) {
                  if (qavg >= edp_.out_th_max) {
                        droptype = DTYPE_FORCED;  // ? not sure, Yun
                  } else if (edv_.out_old == 0) {
			/* 
			 * The average queue size has just crossed the
			 * threshold from below to above "minthresh", or
			 * from above "minthresh" with an empty queue to
			 * above "minthresh" with a nonempty queue.
			 */
                        edv_.out_count = 1;
                        edv_.out_count_bytes = ch->size();
                        edv_.out_old = 1;
                  } else if (drop_out_early(pkt)) {
                        droptype = DTYPE_UNFORCED; // ? not sure, Yun
                  }
          } else {
                  edv_.out_v_prob = 0.0;
                  edv_.out_old = 0;              // explain
          }
          if (qlen >= qlim) {
                  // see if we've exceeded the queue size
                  droptype = DTYPE_FORCED;   //need more consideration, Yun
          }

          if (droptype == DTYPE_UNFORCED) {
                  /* pick packet for ECN, which is dropping in this case */
                  Packet *pkt_to_drop = pickPacketForECN(pkt);
                  /*
                   * If the packet picked is different that the one that just
                   * arrived, add it to the queue and remove the chosen packet.
                   */
                  if (pkt_to_drop != pkt) {
                          q_->enque(pkt);
			  //printf("out: qlen %d %d\n", q_->length(), length());
                          bcount_ += ch->size();
                          q_->remove(pkt_to_drop);
			  //printf("remove qlen %d %d\n",q_->length(),length());
                          bcount_ -= 
                            ((hdr_cmn*)pkt_to_drop->access(off_cmn_))->size();
                          if(((hdr_flags*)pkt_to_drop->access(off_flags_))->pri_)
			  {
                             in_bcount_ -= 
                               ((hdr_cmn*)pkt_to_drop->access(off_cmn_))->size();
			     --in_len_;
			  }
                          pkt = pkt_to_drop;/* ok cause pkt not needed anymore */
                  }
                  // deliver to special "edrop" target, if defined
                  if (de_drop_ != NULL)
                          de_drop_->recv(pkt);
                  else
                          drop(pkt);
          } else {
                  /* forced drop, or not a drop: first enqueue pkt */
                  q_->enque(pkt);
		  //printf("out: qlen %d %d\n", q_->length(), length());
                  bcount_ += ch->size();

                  /* drop a packet if we were told to */
                  if (droptype == DTYPE_FORCED) {
                          /* drop random victim or last one */
                          pkt = pickPacketToDrop();
                          q_->remove(pkt);
			  //printf("remove qlen %d %d\n",q_->length(),length());
                          bcount_ -= ((hdr_cmn*)pkt->access(off_cmn_))->size();
                          if (((hdr_flags*)pkt->access(off_flags_))->pri_)
			  {
                            in_bcount_ -= 
                              ((hdr_cmn*)pkt->access(off_cmn_))->size();
			    --in_len_;
			  }
                          drop(pkt);
                  }
          }
	}

	return;
}

int RIOQueue::command(int argc, const char*const* argv)
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
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
                  int mode;
                  const char* id = argv[2];
                  tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
                  if( tchan_ == 0 ) {
                    tcl.resultf("RIO: trace: can't attach %s for writing", id);
                    return (TCL_ERROR);
                    }
                  return (TCL_OK);
		}
		// tell RIO about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("RIO: no LinkDelay object %s",
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
		if (!strcmp(argv[1], "packetqueue-attach")) {
			delete q_;
			if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
				return (TCL_ERROR);
			else {
				pq_ = q_;
				return (TCL_OK);
			}
		}
	}
	return (Queue::command(argc, argv));
}

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace values of avg queue size, drop probability,
 * and the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void
RIOQueue::trace(TracedVar* v)
{
	char wrk[500], *p;

	if (((p = strstr(v->name(), "ave")) == NULL) &&
	    ((p = strstr(v->name(), "in_ave")) == NULL) &&
	    ((p = strstr(v->name(), "out_ave")) == NULL) &&
	    ((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "in_prob")) == NULL) &&
	    ((p = strstr(v->name(), "out_prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL)) {
		fprintf(stderr, "RIO:unknown trace var %s\n",
			v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t,
				double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}

/* for debugging help */
void RIOQueue::print_edp()
{
	printf("mean_pktsz: %d\n", edp_.mean_pktsize); 
	printf("bytes: %d, wait: %d, setbit: %d\n",
	       edp_.bytes, edp_.wait, edp_.setbit);
	printf("minth: %f, maxth: %f\n", edp_.total_min, edp_.total_max);
	printf("in_minth: %f, in_maxth: %f\n", edp_.in_th_min, edp_.in_th_max);
	printf("out_minth: %f, out_maxth: %f\n", 
                edp_.out_th_min, edp_.out_th_max);
	printf("max_p_inv: %f, qw: %f, ptc: %f\n",
	       edp_.max_p_inv, edp_.q_w, edp_.ptc);
	printf("qlim: %d, idletime: %f\n", qlim_, idletime_);
	printf("qlim: %d, in_idletime: %f\n", qlim_, in_idletime_);
	printf("=========\n");
}

void RIOQueue::print_edv()
{
	printf("v_a: %f, v_b: %f\n", edv_.v_a, edv_.v_b);
	printf("in_v_a: %f, in_v_b: %f\n", edv_.in_v_a, edv_.in_v_b);
	printf("out_v_a: %f, out_v_b: %f\n", edv_.out_v_a, edv_.out_v_b);
}
