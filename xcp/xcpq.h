/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcpq.h,v 1.5 2004/10/28 23:35:40 haldar Exp $ (LBL)
 */


#ifndef NS_XCPQ_H
#define NS_XCPQ_H

#include "drop-tail.h"
#include "packet.h"
#include "xcp-end-sys.h"

#define  INITIAL_Te_VALUE   0.05       // Was 0.3 Be conservative when 
                                       // we don't kow the RTT

#define TRACE  1                       // when 0, we don't race or write 
                                       // var to disk

class XCPWrapQ;
class XCPQueue;

class XCPTimer : public TimerHandler { 
public:
	XCPTimer(XCPQueue *a, void (XCPQueue::*call_back)() ) 
		: a_(a), call_back_(call_back) {};
protected:
	virtual void expire (Event *e);
	XCPQueue *a_; 
	void (XCPQueue::*call_back_)();
}; 


class XCPQueue : public DropTail {
	friend class XCPTimer;
public:
	XCPQueue();
	void Tq_timeout ();  // timeout every propagation delay 
	void Te_timeout ();  // timeout every avg. rtt
	void everyRTT();     // timeout every highest rtt seen by rtr or some
	                     // preset rtt value
	void setupTimers();  // setup timers for xcp queue only
	void setEffectiveRtt(double rtt) ;
	void routerId(XCPWrapQ* queue, int i);
	int routerId(int id = -1); 
  
	int limit(int len = 0);
	void setBW(double bw);
	void setChannel(Tcl_Channel queue_trace_file);
	double totalDrops() { return total_drops_; }
  
	void spread_bytes(int b) { 
		spread_bytes_ = b; 
		if (b) 
			Te_ = BWIDTH;
	}
	
        // Overloaded functions
	void enque(Packet* pkt);
	Packet* deque();
	virtual void drop(Packet* p);
	
	void dropTarget(NsObject *dt) {drop_ = dt; }

  
	// tracing var
	void setNumMice(int mice) {num_mice_ = mice;}

protected:

	// Utility Functions
	double max(double d1, double d2) { return (d1 > d2) ? d1 : d2; }
	double min(double d1, double d2) { return (d1 < d2) ? d1 : d2; }
        int max(int i1, int i2) { return (i1 > i2) ? i1 : i2; }
	int min(int i1, int i2) { return (i1 < i2) ? i1 : i2; }
	double abs(double d) { return (d < 0) ? -d : d; }

	virtual void trace_var(char * var_name, double var);
  
	// Estimation & Control Helpers
	void init_vars();
	
	// called in enque, but packet may be dropped; used for 
	// updating the estimation helping vars such as
	// counting the offered_load_, sum_rtt_by_cwnd_
	virtual void do_on_packet_arrival(Packet* pkt);

	// called in deque, before packet leaves
	// used for writing the feedback in the packet
	virtual void do_before_packet_departure(Packet* p); 
	
  
	// ---- Variables --------
	unsigned int     routerId_;
	XCPWrapQ*        myQueue_;   //pointer to wrapper queue lying on top
	XCPTimer*        queue_timer_;
	XCPTimer*        estimation_control_timer_;
	XCPTimer*        rtt_timer_;
	double           link_capacity_bps_;

	static const double	ALPHA_		= 0.4;
	static const double	BETA_		= 0.226;
	static const double	GAMMA_		= 0.1;
	static const double	XCP_MAX_INTERVAL= 1.0;
	static const double	XCP_MIN_INTERVAL= .001;

	double          Te_;       // control interval
	double          Tq_;    
	double          Tr_;
	double          avg_rtt_;       // average rtt of flows
	double          high_rtt_;      // highest rtt seen in flows
	double          effective_rtt_; // pre-set rtt value 
	double          Cp_;
	double          Cn_;
	double          residue_pos_fbk_;
	double          residue_neg_fbk_;
	double          queue_bytes_;   // our estimate of the fluid model queue
	double          input_traffic_bytes_;       // traffic in Te 
	double          sum_rtt_by_throughput_;
	double          sum_inv_throughput_;
	double          running_min_queue_bytes_;
	unsigned int    num_cc_packets_in_Te_;
  
	int			spread_bytes_; 
	static const int	BSIZE = 4096;
	double			b_[BSIZE];
	double			t_[BSIZE];
	int			maxb_;
	static const double	BWIDTH = 0.01;
	int			min_queue_ci_;
	int			max_queue_ci_;
  
	double		thruput_elep_;
	double		thruput_mice_;
	double		total_thruput_;
	int		num_mice_;
	// drops
	int drops_;
	double total_drops_ ;
  
	// ----- For Tracing Vars --------------//
	Tcl_Channel queue_trace_file_;
  
};


#endif //NS_XCPQ_H
