
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcpq.cc,v 1.5 2004/10/28 23:35:40 haldar Exp $ (LBL)
 */

/* Author : Dina Katabi, dina@ai.mit.edu 1/15/2002 */

#include "xcpq.h"
#include "xcp.h"
#include "random.h"

static class XCPQClass : public TclClass {
public:
	XCPQClass() : TclClass("Queue/DropTail/XCPQ") {}
	TclObject* create(int, const char*const*) {
		return (new XCPQueue);
	}
} class_droptail_xcpq;


XCPQueue::XCPQueue(): queue_timer_(NULL), 
		      estimation_control_timer_(NULL),
		      rtt_timer_(NULL), effective_rtt_(0.0)
{
	init_vars();
}

void XCPQueue::setupTimers()
{
	queue_timer_ = new XCPTimer(this, &XCPQueue::Tq_timeout);
	estimation_control_timer_ = new XCPTimer(this, &XCPQueue::Te_timeout);
	rtt_timer_ = new XCPTimer(this, &XCPQueue::everyRTT);

	// Scheduling timers randomly so routers are not synchronized
	double T;
  
	T = max(0.004, Random::normal(Tq_, 0.2 * Tq_));
	queue_timer_->sched(T);
  
	T = max(0.004, Random::normal(Te_, 0.2 * Te_));
	estimation_control_timer_->sched(T);
	
	T = max(0.004, Random::normal(Tr_, 0.2 * Tr_));
	rtt_timer_->sched(T);
}


void XCPQueue::routerId(XCPWrapQ* q, int id)
{
	if (id < 0 && q == 0)
		fprintf(stderr, "XCP:invalid routerId and queue\n");
	routerId_ = id;
	myQueue_ = q;
}

int XCPQueue::routerId(int id)
{
	if (id > -1) 
		routerId_ = id;
	return ((int)routerId_); 
}

int XCPQueue::limit(int qlim)
{
	if (qlim > 0) 
		qlim_ = qlim;
	return (qlim_);
}

void XCPQueue::setBW(double bw)
{
	if (bw > 0) 
		link_capacity_bps_ = bw;
}

void XCPQueue::setChannel(Tcl_Channel queue_trace_file)
{
	queue_trace_file_ = queue_trace_file;
}

Packet* XCPQueue::deque()
{
	double inst_queue = byteLength();
/* L 36 */
	if (inst_queue < running_min_queue_bytes_) 
		running_min_queue_bytes_= inst_queue;
  
	Packet* p = DropTail::deque();
	do_before_packet_departure(p);
  
	max_queue_ci_ = max(length(), max_queue_ci_);
	min_queue_ci_ = min(length(), min_queue_ci_);
  
	return (p);
}

void XCPQueue::enque(Packet* pkt)
{
	max_queue_ci_ = max(length(), max_queue_ci_);
	min_queue_ci_ = min(length(), min_queue_ci_);

	do_on_packet_arrival(pkt);
	DropTail::enque(pkt);
}

void XCPQueue::do_on_packet_arrival(Packet* pkt){
  
	double pkt_size = double(hdr_cmn::access(pkt)->size());

	/* L 1 */
	input_traffic_bytes_ += pkt_size;
  
	hdr_xcp *xh = hdr_xcp::access(pkt); 
	if (spread_bytes_) {
		int i = int(xh->rtt_/BWIDTH + .5);
		if (i > maxb_)
			maxb_ = i;
		b_[i] += pkt_size;
    
		if (xh->rtt_ != 0.0 && xh->throughput_ != 0.0)
			t_[i] += pkt_size/xh->throughput_;
	}
	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;	// Estimates depend only on Forward XCP Traffic
      
	++num_cc_packets_in_Te_;
  
	if (xh->rtt_ != 0.0 && xh->throughput_ != 0.0) {
		/* L 2 */
		sum_inv_throughput_ += pkt_size / xh->throughput_;
		/* L 3 */
		if (xh->rtt_ < XCP_MAX_INTERVAL) {
			/* L 4 */
			sum_rtt_by_throughput_ += (xh->rtt_ 
						   * pkt_size 
						   / xh->throughput_);
			/* L 5 */
		} else {
			/* L 6 */
			sum_rtt_by_throughput_ += (XCP_MAX_INTERVAL 
						   * pkt_size 
						   / xh->throughput_);	
		}
	}
}


void XCPQueue::do_before_packet_departure(Packet* p)
{
	if (!p) return;
  
	hdr_xcp *xh = hdr_xcp::access(p);
    
	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;
	if (xh->rtt_ == 0.0) {
		xh->delta_throughput_ = 0;
		return;
	}
	if (xh->throughput_ == 0.0)
		xh->throughput_ = .1;    // XXX 1bps is small enough
	
	double inv = 1.0/xh->throughput_;
	double pkt_size = double(hdr_cmn::access(p)->size());
	double frac = 1.0;
	if (spread_bytes_) {
		// rtt-scaling
		frac = (xh->rtt_ > Te_) ? Te_ / xh->rtt_ : xh->rtt_ / Te_;
	}
	/* L 19 */
	double pos_fbk = min(residue_pos_fbk_, Cp_ * inv * pkt_size);
	pos_fbk *= frac;
	/* L 20 */
	double neg_fbk = min(residue_neg_fbk_, Cn_ * pkt_size);
	neg_fbk *= frac;
	/* L 21 */
	double feedback = pos_fbk - neg_fbk;
  
	/* L 22 */	
	if (xh->delta_throughput_ >= feedback) {
		/* L 23 */
		xh->delta_throughput_ = feedback;
		xh->controlling_hop_ = routerId_;
		/* L 26 */
	} else {
		/* L 27-33 corrected */
		neg_fbk = min(residue_neg_fbk_, neg_fbk + (feedback - xh->delta_throughput_));
		pos_fbk = xh->delta_throughput_ + neg_fbk;
	}
	/* L 24-25 */
	residue_pos_fbk_ = max(0.0, residue_pos_fbk_ - pos_fbk);
	residue_neg_fbk_ = max(0.0, residue_neg_fbk_ - neg_fbk);
	/* L 34 */
	if (residue_pos_fbk_ == 0.0)
		Cp_ = 0.0;
	/* L 35 */
	if (residue_neg_fbk_ == 0.0)
		Cn_ = 0.0;
  
	if (TRACE && (queue_trace_file_ != 0 )) {
		trace_var("pos_fbk", pos_fbk);
		trace_var("neg_fbk", neg_fbk);
		trace_var("delta_throughput", xh->delta_throughput_);
		int id = hdr_ip::access(p)->flowid();
		char buf[25];
		sprintf(buf, "Thruput%d",id);
		trace_var(buf, xh->throughput_);
		
		// tracing measured thruput info
		if (xh->rtt_ > high_rtt_)
			high_rtt_ = xh->rtt_;
		total_thruput_ += pkt_size;
		
		if (num_mice_ != 0) {
			if (id >= num_mice_) 
				thruput_elep_ += pkt_size;
			else 
				thruput_mice_ += pkt_size;
		}
	}
}


void XCPQueue::Tq_timeout()
{
	double inst_queue = byteLength();
	/* L 37 */
	queue_bytes_ = running_min_queue_bytes_;
	/* L 37.5 corrected: min_queue = inst_queue */
	running_min_queue_bytes_ = inst_queue;
	/* L 38 */
	Tq_ = max(0.002, (avg_rtt_ - inst_queue/link_capacity_bps_)/2.0); 
	/* L 39 */
	queue_timer_->resched(Tq_);
	
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("Tq_", Tq_);
		trace_var("queue_bytes_", queue_bytes_);
		trace_var("routerId_", routerId_);
	}
}

void XCPQueue::Te_timeout()
{

	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("residue_pos_fbk_not_allocated", residue_pos_fbk_);
		trace_var("residue_neg_fbk_not_allocated", residue_neg_fbk_);
	}
	if (spread_bytes_) {
		double spreaded_bytes = b_[0];
		double tp = t_[0];
		for (int i = 1; i <= maxb_; ++i) {
			double spill = b_[i]/(i+1);
			spreaded_bytes += spill;
			b_[i-1] = b_[i] - spill;
			
			spill = t_[i]/(i+1);
		        tp += spill;
			t_[i-1] = t_[i] - spill;
		}
		
		b_[maxb_] = t_[maxb_] = 0;
		if (maxb_ > 0)
			--maxb_;
		input_traffic_bytes_ = spreaded_bytes;
		sum_inv_throughput_ = tp;
	}
		
	double input_bw = input_traffic_bytes_ / Te_;
	double phi_bps = 0.0;
	double shuffled_traffic_bps = 0.0;

	if (spread_bytes_) {
		avg_rtt_ = (maxb_ + 1)*BWIDTH/2; // XXX fix me
	} else {
		if (sum_inv_throughput_ != 0.0) {
/* L 7 */
			avg_rtt_ = sum_rtt_by_throughput_ / sum_inv_throughput_;
		} else
			avg_rtt_ = INITIAL_Te_VALUE;
	}
	
	if (input_traffic_bytes_ > 0) {
/* L 8 */
		phi_bps = ALPHA_ * (link_capacity_bps_- input_bw) 
			- BETA_ * queue_bytes_ / avg_rtt_;
/* L 9 corrected */
		shuffled_traffic_bps = GAMMA_ * input_bw;

		if (shuffled_traffic_bps > abs(phi_bps))
			shuffled_traffic_bps -= abs(phi_bps);
		else
			shuffled_traffic_bps = 0.0;
/* L 9 ends here */
	}
/* L 12, 13 */	
	residue_pos_fbk_ = max(0.0,  phi_bps) + shuffled_traffic_bps;
	residue_neg_fbk_ = max(0.0, -phi_bps) + shuffled_traffic_bps;

	if (sum_inv_throughput_ == 0.0)
		sum_inv_throughput_ = 1.0;
	if (input_traffic_bytes_ > 0) {
/* L 10 */
		Cp_ =  residue_pos_fbk_ / sum_inv_throughput_;
/* L 11 */
		Cn_ =  residue_neg_fbk_ / input_traffic_bytes_;
	} else 
		Cp_ = Cn_ = 0.0;
		
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("input_traffic_bytes_", input_traffic_bytes_);
		trace_var("avg_rtt_", avg_rtt_);
		trace_var("residue_pos_fbk_", residue_pos_fbk_);
		trace_var("residue_neg_fbk_", residue_neg_fbk_);
		//trace_var("Qavg", edv_.v_ave);
		trace_var("Qsize", length());
		trace_var("min_queue_ci_", double(min_queue_ci_));
		trace_var("max_queue_ci_", double(max_queue_ci_));

		trace_var("routerId", routerId_);
	} 
	num_cc_packets_in_Te_ = 0;

/* L 14 */  
	input_traffic_bytes_ = 0.0;
/* L 15 */
	sum_inv_throughput_ = 0.0;
/* L 16 */
	sum_rtt_by_throughput_ = 0.0;
/* L 17 */
	if (spread_bytes_)
		Te_ = BWIDTH;
	else
		Te_ = max(avg_rtt_, XCP_MIN_INTERVAL);

/* L 18 */
	estimation_control_timer_->resched(Te_);

	min_queue_ci_ = max_queue_ci_ = length();
}


void XCPQueue::everyRTT ()
{
	if (effective_rtt_ != 0.0)
		Tr_ = effective_rtt_;
	else 
		if (high_rtt_ != 0.0)
			Tr_ = high_rtt_;

	// measure drops, if any
	trace_var("d", drops_);
	drops_=0;

	// sample the current queue size
	trace_var("q",length());
  
	// Update utilization 
	trace_var("u", total_thruput_/(Tr_*link_capacity_bps_));
	trace_var("u_elep", thruput_elep_/(Tr_*link_capacity_bps_));
	trace_var("u_mice", thruput_mice_/(Tr_*link_capacity_bps_));
	total_thruput_ = 0;
  
	rtt_timer_->resched(Tr_);
}


void  XCPQueue::drop(Packet* p)
{
	drops_++;
	total_drops_ = total_drops_++;
  
	Connector::drop(p);
}

void  XCPQueue::setEffectiveRtt(double rtt)
{
	effective_rtt_ = rtt;
	rtt_timer_->resched(effective_rtt_);
}

// Estimation & Control Helpers

void XCPQueue::init_vars() 
{
	link_capacity_bps_	= 0.0;
	avg_rtt_		= INITIAL_Te_VALUE;
	if (spread_bytes_)
		Te_		= BWIDTH;
	else
		Te_		= INITIAL_Te_VALUE;
  
	Tq_			= INITIAL_Te_VALUE; 
	Tr_                     = 0.1;
	high_rtt_               = 0.0;
	Cp_			= 0.0;
	Cn_			= 0.0;     
	residue_pos_fbk_	= 0.0;
	residue_neg_fbk_	= 0.0;     
	queue_bytes_		= 0.0; // our estimate of the fluid model queue
	  
	input_traffic_bytes_	= 0.0; 
	sum_rtt_by_throughput_	= 0.0;
	sum_inv_throughput_	= 0.0;
	running_min_queue_bytes_= 0;
	num_cc_packets_in_Te_   = 0;
  
	queue_trace_file_ = 0;
	myQueue_ = 0;
  
	spread_bytes_ 		= 0;
	for (int i = 0; i<BSIZE; ++i)
		b_[i] = t_[i] = 0;
	maxb_ = 0;
  
	min_queue_ci_ = max_queue_ci_ = length();
  
	// measuring drops
	drops_ = 0;
	total_drops_ = 0;

	// utilisation
	num_mice_ = 0;
	thruput_elep_ = 0.0;
	thruput_mice_ = 0.0;
	total_thruput_ = 0.0;
}


void XCPTimer::expire(Event *) { 
	(*a_.*call_back_)();
}


void XCPQueue::trace_var(char * var_name, double var)
{
	char wrk[500];
	double now = Scheduler::instance().clock();

	if (queue_trace_file_) {
		int n;
		sprintf(wrk, "%s %g %g",var_name, now, var);
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
	}
	return; 
}


