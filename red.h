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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/red.h,v 1.6 1997/07/22 22:21:21 padmanab Exp $ (LBL)
 */

#include <math.h>
#include <string.h>
#include <sys/types.h>

#include "queue.h"
#include "Tcl.h"
#include "packet.h"
#include "random.h"
#include "flags.h"
#include "delay.h"

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

	double th_min;		/* minimum threshold of average queue size */
	double th_max;		/* maximum threshold of average queue size */
	double max_p_inv;	/* 1/max_p, for max_p = maximum prob.  */
	double q_w;		/* queue weight */
				   
	/*
	 * Computed as a function of user supplied paramters.
	 */
	double ptc;		/* packet time constant in packets/second */
	int adjusted_for_bytes_;      /* adjusted for byte-counting
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
		// why? -KF
		virtual PacketQueue *q() { return q_; }
 protected:
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	virtual Packet *pickPacketToDrop();
	Packet* deque();
	void reset();
	void run_estimator(int nqueued, int m);
	void plot();
	void plot1(int qlen);
	int drop_early(Packet* pkt);

		// why? -KF
		virtual Packet* deque(PacketQueue *q) { return q->deque(); }
		virtual void enque(PacketQueue *q, Packet *pkt){
			q->enque(pkt);
		}
		virtual void remove(PacketQueue *q, Packet *pkt){
			q->remove(pkt);
		}
		

	LinkDelay* link_;	/* outgoing link */
	int fifo_;		/* fifo queue? */
        PacketQueue *q_; /* underlying FIFO queue */
		
	int bcount_;	/* byte count */
	int qib_;	/* bool: queue measured in bytes? */
	NsObject* de_drop_;	/* drop_early target */

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
