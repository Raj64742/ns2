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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/rio.h,v 1.1 2000/06/21 02:41:41 sfloyd Exp $ (LBL)
 */

#ifndef ns_rio_h
#define ns_rio_h

#include "queue.h"
#include "tclcl.h"
#include "packet.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "template.h"

class LinkDelay;

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
	double q_w;		/* queue weight given to cur q size sample */

	int gentle;		/* true to increases dropping prob. slowly *
				 * when ave queue exceeds maxthresh. */
	double in_th_min;	/* minimum threshold of average queue size */
	double in_th_max;	/* maximum threshold of average queue size */
	double in_max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */

	double out_th_min;	/* minimum threshold of average queue size */
	double out_th_max;	/* maximum threshold of average queue size */
	double out_max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */

        double total_min;
        double total_max;
        double max_p_inv;

	/*
	 * Computed as a function of user supplied paramters.
	 */
	double ptc;		/* packet time constant in packets/second */
};

/*
 * Early drop variables, maintained by RIO
 */
struct edv {
	TracedDouble v_ave;	/* average queue size */
	TracedDouble v_prob1;	/* prob. of packet drop before "count". */
	double v_slope;		/* used in computing average queue size */
	double v_prob;		/* prob. of packet drop */
	double v_a;		/* v_prob = v_a * v_ave + v_b */
	double v_b;
	double v_c;		/* used for "gentle" mode */
	double v_d;		/* used for "gentle" mode */
	int count;		/* # of packets since last drop */
	int count_bytes;	/* # of bytes since last drop */
	int old;		/* 0 when average queue first exceeds thresh */

        /* added by Wenjia modified by Yun: a new set In packets */
        double in_v_ave;         /* average In queue size */
	double in_v_prob1;	/* prob. of packet drop before "count". */
        double in_v_slope;       /* used in computing average queue size */
        double in_v_r;
        double in_v_prob;        /* prob. of packet drop */
        struct dlist* in_drops;
        double in_v_a;           /* v_prob = v_a * v_ave + v_b */
        double in_v_b;
        int in_count;           /* # of packets since last drop */
        int in_count_bytes;     /* # of bytes since last drop */
        int in_old;             /* 0 when average queue first exceeds thresh */

        /* added by Wenjia modified by Yun: a new set for Out packets */
        double out_v_ave;        /* average queue size */
	double out_v_prob1;	/* prob. of packet drop before "count". */
        double out_v_slope;      /* used in computing average queue size */
        double out_v_r;
        double out_v_prob;       /* prob. of packet drop */
        struct dlist* out_drops;
        double out_v_a;          /* v_prob = v_a * v_ave + v_b */
        double out_v_b;
        int out_count;          /* # of packets since last drop */
        int out_count_bytes;    /* # of bytes since last drop */
        int out_old;            /* 0 when average queue first exceeds thresh */

	edv() : v_ave(0.0), v_prob1(0.0), v_slope(0.0), v_prob(0.0),
		v_a(0.0), v_b(0.0), count(0), count_bytes(0), old(0) { }
};

class RIOQueue : public Queue {
 public:	
	RIOQueue();
 protected:
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	virtual Packet *pickPacketForECN(Packet* pkt);
	virtual Packet *pickPacketToDrop();
	Packet* deque();
	void reset();

        // modified by Yun: convert to two funtion for In and Out pkts
	// void run_estimator(int nqueued, int m);
	void run_in_estimator(int in, int total, int m);
	void run_out_estimator(int out, int total, int m);

	// int drop_early(Packet* pkt);
	int drop_in_early(Packet* pkt);
	int drop_out_early(Packet* pkt);

	LinkDelay* link_;	/* outgoing link */
	int fifo_;		/* fifo queue? */
        PacketQueue *q_; 	/* underlying (usually) FIFO queue */
		
	int bcount_;	/* byte count */

	/* added by Yun: In packets byte count */
	int in_len_;	/* In Packets count */
	int in_bcount_;	/* In packets byte count */

	int qib_;	/* bool: queue measured in bytes? */
	NsObject* de_drop_;	/* drop_early target */

	Tcl_Channel tchan_;	/* place to write trace records */
	TracedInt curq_;	/* current qlen seen by arrivals */
	void trace(TracedVar*);	/* routine to write trace records */

	/*
	 * Static state.
	 */
	int drop_tail_;		/* drop-tail */
	int drop_front_;	/* drop-from-front */
	int drop_rand_;		/* drop-tail, or drop random? */
	int ns1_compat_;	/* for ns-1 compatibility, bypass a */
				/*   small bugfix */

	edp edp_;	/* early-drop params */
	int doubleq_;	/* for experiments with priority for small packets */
	int dqthresh_;	/* for experiments with priority for small packets */

	/*
	 * Dynamic state.
	 */
	int idle_;		/* queue is idle? */
	double idletime_;	/* if so, since this time */

	/* added by Wenjia noticed by Yun: to trace the idle */
	int in_idle_;
	double in_idletime_;

	edv edv_;		/* early-drop variables */

	void print_edp();	// for debugging
	void print_edv();	// for debugging
};

#endif