/*
 * Copyright (c) 1997 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 *
 * Tcp-vegas with Rate-based pacing by John Heidemann <johnh@isi.edu>
 * and Vikram Visweswaraiah <visweswa@isi.edu>.
 * The original SunOS implementation was by Vikram Visweswaraiah
 * and Ashish Savla <asavla@usc.edu> 
 *
 * Rate-based pacing is an experimental addition to TCP
 * to address the slow-start restart problem.
 * See <http://www.isi.edu/lsam/publications/rate_based_pacing/index.html>
 * for details.
 *
 * A paper analysing RBP performance is in progress (as of 19-Jun-97).
 */


#ifndef lint
static char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-rbp.cc,v 1.2 1997/06/23 17:03:17 heideman Exp $ (NCSU/IBM)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

class RBPVegasTcpAgent;

class RBPPaceTimer : public TimerHandler {
public:
	RBPPaceTimer(RBPVegasTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RBPVegasTcpAgent *a_;
};

class RBPVegasTcpAgent : public virtual VegasTcpAgent {
	friend RBPPaceTimer;
 public:
	RBPVegasTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
	virtual void RBPVegasTcpAgent::send_much(int force, int reason, int maxburst);

	double rbp_scale_;   // conversion from actual -> rbp send rates
protected:
	void paced_send_one();

	enum rbp_modes { RBP_GOING, RBP_POSSIBLE, RBP_OFF };
	enum rbp_modes rbp_mode_;
	double inter_pace_delay_;
	RBPPaceTimer pace_timer_;
};

static class RBPVegasTcpClass : public TclClass {
public:
	RBPVegasTcpClass() : TclClass("Agent/TCP/Vegas/RBP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RBPVegasTcpAgent());
	}
} class_rbp;


void RBPPaceTimer::expire(Event *e) { a_->paced_send_one(); }

RBPVegasTcpAgent::RBPVegasTcpAgent() : TcpAgent(),
	rbp_mode_(RBP_OFF),
	pace_timer_(this)
{
	bind("rbp_scale_", &rbp_scale_);
}

void
RBPVegasTcpAgent::recv(Packet *pkt, Handler *hand)
{
	if (rbp_mode_ != RBP_OFF) {
		// reciept of anything disables rbp
		rbp_mode_ = RBP_OFF;
	};
	VegasTcpAgent::recv(pkt, hand);
}

void
RBPVegasTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_) {
			// Idle for a while => RBP next time.
			rbp_mode_ = RBP_POSSIBLE;
			return;
		};
	};
	VegasTcpAgent::timeout(tno);
}

void
RBPVegasTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (rbp_mode_ == RBP_POSSIBLE && t_seqno() < curseq_) {
		// start paced mode
		rbp_mode_ = RBP_GOING;
		double rbp_rate = v_actual_ * rbp_scale_;
		inter_pace_delay_ = 1 / rbp_rate;
		paced_send_one();
	} else {
		VegasTcpAgent::send_much(force,reason, maxburst);
	};
}

void
RBPVegasTcpAgent::paced_send_one()
{
	// xxx: should prevent from overrunning cwnd as well
	if (rbp_mode_ == RBP_GOING && t_seqno() < curseq_) {
		// send one packet
		output(t_seqno()++, TCP_REASON_RBP);
		// schedule next pkt
		pace_timer_.resched(inter_pace_delay_);
	};
}

