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
 * and Ashish Savla <asavla@usc.edu>.
 *
 * Rate-based pacing is an experimental addition to TCP
 * to address the slow-start restart problem.
 * See <http://www.isi.edu/lsam/publications/rate_based_pacing/index.html>
 * for details.
 *
 * A paper analysing RBP performance is in progress (as of 19-Jun-97).
 */


#ifndef lint
static const char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-rbp.cc,v 1.9 1997/07/25 05:26:02 padmanab Exp $ (NCSU/IBM)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#ifndef MIN
#define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif /* ! MIN */

#if 0
#define RBP_DEBUG_PRINTF(x) printf x
#else /* ! 0 */
#define RBP_DEBUG_PRINTF(x)
#endif /* 0 */


#define RBP_MIN_SEGMENTS 2

class RBPVegasTcpAgent;

class RBPVegasPaceTimer : public TimerHandler {
public:
	RBPVegasPaceTimer(RBPVegasTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RBPVegasTcpAgent *a_;
};
// Hmmm... ``a is a'' in the construction of the RBPVegasPaceTimer edifice :->

class RBPVegasTcpAgent : public virtual VegasTcpAgent {
	friend RBPVegasPaceTimer;
 public:
	RBPVegasTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
	virtual void RBPVegasTcpAgent::send_much(int force, int reason, int maxburst);

	double rbp_scale_;   // conversion from actual -> rbp send rates
	enum rbp_rate_algorithms { RBP_NO_ALGORITHM, RBP_VEGAS_RATE_ALGORITHM, RBP_CWND_ALGORITHM };
	int rbp_rate_algorithm_;
protected:
	void paced_send_one();
	int able_to_rbp_send_one();

	enum rbp_modes { RBP_GOING, RBP_POSSIBLE, RBP_OFF };
	enum rbp_modes rbp_mode_;
	double inter_pace_delay_;
	RBPVegasPaceTimer pace_timer_;
};

static class RBPVegasTcpClass : public TclClass {
public:
	RBPVegasTcpClass() : TclClass("Agent/TCP/Vegas/RBP") {}
	TclObject* create(int, const char*const*) {
		return (new RBPVegasTcpAgent());
	}
} class_vegas_rbp;


void RBPVegasPaceTimer::expire(Event *) { a_->paced_send_one(); }

RBPVegasTcpAgent::RBPVegasTcpAgent() : TcpAgent(),
	rbp_mode_(RBP_OFF),
	pace_timer_(this)
{
	bind("rbp_scale_", &rbp_scale_);
	bind("rbp_rate_algorithm_", &rbp_rate_algorithm_);
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
	if (rbp_mode_ == RBP_POSSIBLE && able_to_rbp_send_one()) {
		// start paced mode
		rbp_mode_ = RBP_GOING; 
		double rbwin_vegas;
		switch (rbp_rate_algorithm_) {
		case RBP_VEGAS_RATE_ALGORITHM:
			// Try to follow tcp_output.c here
			// Calculate the vegas window as its reported rate
			// times the rtt.
			rbwin_vegas = v_actual_ * v_rtt_;
			RBP_DEBUG_PRINTF(("-----------------\n"));
			RBP_DEBUG_PRINTF(("rbwin_vegas = %g\nv_actual = %g\nv_rtt =%g\nbase_rtt=%g\n",
					  rbwin_vegas, v_actual_, v_rtt_, v_baseRTT_));
			// Smooth the vegas window
			rbwin_vegas *= rbp_scale_;
			break;
		case RBP_CWND_ALGORITHM:
			// Pace out scaled cwnd.
			rbwin_vegas = cwnd_ * rbp_scale_;
			break;
		default:
			abort();
		};
		rbwin_vegas = int(rbwin_vegas + 0.5);   // round
		// Always pace at least RBP_MIN_SEGMENTS
		if (rbwin_vegas <= RBP_MIN_SEGMENTS) {
			rbwin_vegas = RBP_MIN_SEGMENTS;
		};

		// Conservatively set the congestion window to min of
		// congestion window and the smoothed rbwin_vegas
		RBP_DEBUG_PRINTF(("cwnd before check = %g\n", cwnd_));
		cwnd_ = MIN(cwnd_, rbwin_vegas);
		RBP_DEBUG_PRINTF(("cwnd after check = %g\n", cwnd_));
		RBP_DEBUG_PRINTF(("recv win = %g\n", wnd_));
		// RBP timer calculations must be based on the actual
		// window which is the min of the receiver's
		// advertised window and the congestion window.
		// TcpAgent::window() does this job.
		// What this means is we expect to send window() pkts
		// in v_rtt_ time.
		inter_pace_delay_ = (v_rtt_)/(window() * 1.0);
		RBP_DEBUG_PRINTF(("window is %d\n", window()));
		RBP_DEBUG_PRINTF(("ipt = %g\n", inter_pace_delay_));
		paced_send_one();
	} else {
		VegasTcpAgent::send_much(force,reason, maxburst);
	};
}

void
RBPVegasTcpAgent::paced_send_one()
{
	if (rbp_mode_ == RBP_GOING && able_to_rbp_send_one()) {
		RBP_DEBUG_PRINTF(("Sending one rbp packet\n"));
		// send one packet
		output(t_seqno_++, TCP_REASON_RBP);
		// schedule next pkt
		pace_timer_.resched(inter_pace_delay_);
	};
}

int
RBPVegasTcpAgent::able_to_rbp_send_one()
{
	return t_seqno_ < curseq_ && t_seqno_ <= highest_ack_ + window();
}


/***********************************************************************
 *
 * The reno-based version
 *
 */


class RBPRenoTcpAgent;

class RBPRenoPaceTimer : public TimerHandler {
public:
	RBPRenoPaceTimer(RBPRenoTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	RBPRenoTcpAgent *a_;
};
// Hmmm... ``a is a'' in the construction of the RBPRenoPaceTimer edifice :->

class RBPRenoTcpAgent : public virtual RenoTcpAgent {
	friend RBPRenoPaceTimer;
 public:
	RBPRenoTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
	virtual void RBPRenoTcpAgent::send_much(int force, int reason, int maxburst);

	double rbp_scale_;   // conversion from actual -> rbp send rates
	// enum rbp_rate_algorithms { RBP_NO_ALGORITHM, RBP_VEGAS_RATE_ALGORITHM, RBP_CWND_ALGORITHM };
	// int rbp_rate_algorithm_;
protected:
	void paced_send_one();
	int able_to_rbp_send_one();

	enum rbp_modes { RBP_GOING, RBP_POSSIBLE, RBP_OFF };
	enum rbp_modes rbp_mode_;
	double inter_pace_delay_;
	RBPRenoPaceTimer pace_timer_;
};

static class RBPRenoTcpClass : public TclClass {
public:
	RBPRenoTcpClass() : TclClass("Agent/TCP/Reno/RBP") {}
	TclObject* create(int, const char*const*) {
		return (new RBPRenoTcpAgent());
	}
} class_reno_rbp;


void RBPRenoPaceTimer::expire(Event *) { a_->paced_send_one(); }

RBPRenoTcpAgent::RBPRenoTcpAgent() : TcpAgent(),
	rbp_mode_(RBP_OFF),
	pace_timer_(this)
{
	bind("rbp_scale_", &rbp_scale_);
	// algorithm is not used in Reno
	// bind("rbp_rate_algorithm_", &rbp_rate_algorithm_);
}

void
RBPRenoTcpAgent::recv(Packet *pkt, Handler *hand)
{
	if (rbp_mode_ != RBP_OFF) {
		// reciept of anything disables rbp
		rbp_mode_ = RBP_OFF;
	};
	RenoTcpAgent::recv(pkt, hand);
}

void
RBPRenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_) {
			// Idle for a while => RBP next time.
			rbp_mode_ = RBP_POSSIBLE;
			return;
		};
	};
	RenoTcpAgent::timeout(tno);
}

void
RBPRenoTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (rbp_mode_ == RBP_POSSIBLE && able_to_rbp_send_one()) {
		// start paced mode
		rbp_mode_ = RBP_GOING; 
		double rbwin_reno;

		// Pace out scaled cwnd.
		rbwin_reno = cwnd_ * rbp_scale_;

		rbwin_reno = int(rbwin_reno + 0.5);   // round
		// Always pace at least RBP_MIN_SEGMENTS
		if (rbwin_reno <= RBP_MIN_SEGMENTS) {
			rbwin_reno = RBP_MIN_SEGMENTS;
		};

		// Conservatively set the congestion window to min of
		// congestion window and the smoothed rbwin_reno
		RBP_DEBUG_PRINTF(("cwnd before check = %g\n", cwnd_));
		cwnd_ = MIN(cwnd_, rbwin_reno);
		RBP_DEBUG_PRINTF(("cwnd after check = %g\n", cwnd_));
		RBP_DEBUG_PRINTF(("recv win = %g\n", wnd_));
		// RBP timer calculations must be based on the actual
		// window which is the min of the receiver's
		// advertised window and the congestion window.
		// TcpAgent::window() does this job.
		// What this means is we expect to send window() pkts
		// in v_rtt_ time.
		inter_pace_delay_ = (t_rtt_)/(window() * 1.0);
		RBP_DEBUG_PRINTF(("window is %d\n", window()));
		RBP_DEBUG_PRINTF(("ipt = %g\n", inter_pace_delay_));
		paced_send_one();
	} else {
		RenoTcpAgent::send_much(force,reason, maxburst);
	};
}

void
RBPRenoTcpAgent::paced_send_one()
{
	if (rbp_mode_ == RBP_GOING && able_to_rbp_send_one()) {
		RBP_DEBUG_PRINTF(("Sending one rbp packet\n"));
		// send one packet
		output(t_seqno_++, TCP_REASON_RBP);
		// schedule next pkt
		pace_timer_.resched(inter_pace_delay_);
	};
}

int
RBPRenoTcpAgent::able_to_rbp_send_one()
{
	return t_seqno_ < curseq_ && t_seqno_ <= highest_ack_ + window();
}
