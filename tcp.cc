/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp.cc,v 1.7 1997/02/27 04:39:16 kfall Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "Tcl.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "random.h"

TCPHeader tcphdr;
TCPHeader* TCPHeader::myaddress_ = &tcphdr;

static class TCPHeaderClass : public TclClass {
public:
        TCPHeaderClass() : TclClass("PacketHeader/TCP") {}
        TclObject* create(int argc, const char*const* argv) {
                        return &tcphdr;
        }       
} class_tcphdr;

static class TcpClass : public TclClass {
public:
	TcpClass() : TclClass("Agent/TCP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAgent());
	}
} class_tcp;

TcpAgent::TcpAgent() : Agent(PT_TCP), rtt_active_(0), rtt_seq_(-1)
{
	bind("window_", &wnd_);
	bind("windowInit_", &wnd_init_);
	bind("windowOption_", &wnd_option_);
	bind("windowConstant_", &wnd_const_);
	bind("windowThresh_", &wnd_th_);
	bind("overhead_", &overhead_);
	bind("tcpTick_", &tcp_tick_);
	bind("ecn_", &ecn_);
	bind("packetSize_", &size_);
	bind_bool("bugFix_", &bug_fix_);
	bind("maxburst_", &maxburst_);
	bind("maxcwnd_", &maxcwnd_);

	bind("dupacks_", &dupacks_);
	bind("seqno_", &curseq_);
	bind("ack_", &highest_ack_);
	bind("cwnd_", &cwnd_);
	bind("awnd_", &awnd_);
	bind("ssthresh_", &ssthresh_);
	bind("rtt_", &t_rtt_);
	bind("srtt_", &t_srtt_);
	bind("rttvar_", &t_rttvar_);
	bind("backoff_", &t_backoff_);
}

void TcpAgent::reset()
{
	rtt_init();
	/*XXX lookup variables */
	dupacks_ = 0;
	curseq_ = 0;
	cwnd_ = wnd_init_;
	t_seqno_ = 0;
	maxseq_ = -1;
	last_ack_ = -1;
	highest_ack_ = -1;
	ssthresh_ = int(wnd_);
	awnd_ = wnd_init_ / 2.0;
	recover_ = 0;
	recover_cause_ = 0;
}

/*
 * Initialize variables for the retransmit timer.
 */
void TcpAgent::rtt_init()
{
	t_rtt_ = t_srtt_ = 0;
	/* the setting of t_rttvar depends on the value for tcp_tick_ */
	t_rttvar_ = int(3. / tcp_tick_);
	t_backoff_ = 1;
}

/* This has been modified to use the tahoe code. */
double TcpAgent::rtt_timeout()
{
	double timeout = ((t_srtt_ >> 3) + t_rttvar_) * tcp_tick_ ;
        timeout *= t_backoff_;

	/* XXX preclude overflow */
	if (timeout > 2000 || timeout < -2000)
		abort();

        if (timeout < 2 * tcp_tick_)
		timeout = 2 * tcp_tick_;
	return (timeout);
}

/* This has been modified to use the tahoe code. */
void TcpAgent::rtt_update(double tao)
{
	t_rtt_ = int((tao / tcp_tick_) + 0.5);
	if (t_rtt_ < 1)
		t_rtt_ = 1;

        if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> 3);
		if ((t_srtt_ += delta) <= 0)
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (t_rttvar_ >> 2);
		if ((t_rttvar_ += delta) <= 0)
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << 3;
		t_rttvar_ = t_rtt_ << 1;
	}
}

void TcpAgent::rtt_backoff()
{
	if (t_backoff_ < 64)
		t_backoff_ <<= 1;

	if (t_backoff_ > 8)
		/*
		 * If backed off this far, clobber the srtt
		 * value, storing it in the mean deviation
		 * instead.
		 */
		t_srtt_ = 0;
}

void TcpAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	TCPHeader *tcph = TCPHeader::access(p->bits());
	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->reason() = reason;
	Connector::send(p, 0);
	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_)
				rtt_seq_ = seqno;
		}
	}
	if (!pending_[TCP_TIMER_RTX])
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

int TcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			curseq_ += atoi(argv[2]);
			send(0, 0);
			return (TCL_OK);
		}
		/*
		 * Curtis Villamizar's trick to transfer tcp connection
		 * parameters to emulate http persistent connections.
		 */
		if (strcmp(argv[1], "persist") == 0) {
			TcpAgent *other
			  = (TcpAgent*)TclObject::lookup(argv[2]);
			cwnd_ = other->cwnd_;
			awnd_ = other->awnd_;
			ssthresh_ = other->ssthresh_;
			t_rtt_ = other->t_rtt_;
			t_srtt_ = other->t_srtt_;
			t_rttvar_ = other->t_rttvar_;
			t_backoff_ = other->t_backoff_;
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

int TcpAgent::window()
{
	return (int(cwnd_ < wnd_ ? cwnd_ : wnd_));
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpAgent::send(int force, int reason, int maxburst)
{
	int win = window();
	int npackets = 0;

	while (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
		if (overhead_ == 0 || force) {
			output(t_seqno_++, reason);
			npackets++;
		} else if (!pending_[TCP_TIMER_DELSND]) {
			/*
			 * Set a delayed send timeout.
			 */
			sched(Random::uniform(overhead_), TCP_TIMER_DELSND);
			return;
		}
		if (maxburst && npackets == maxburst)
			break;
	}
}

/*
 * We got a timeout or too many duplicate acks.  Clear the retransmit timer.  
 * Resume the sequence one past the last packet acked.  
 * "mild" is 0 for timeouts and Tahoe dup acks, 1 for Reno dup acks.
 */
void TcpAgent::reset_rtx_timer(int mild)
{
	set_rtx_timer();
	rtt_backoff();
	if (!mild)
		t_seqno_ = highest_ack_ + 1;
	rtt_active_ = 0;
}

/*
 * Set retransmit timer.  If one is already pending, cancel it
 * and set a new one based on the current rtt estimate.
 */
void TcpAgent::set_rtx_timer()
{
	if (pending_[TCP_TIMER_RTX])
		cancel(TCP_TIMER_RTX);
	sched(rtt_timeout(), TCP_TIMER_RTX);
}

/*
 * Set new retransmission timer if not all outstanding
 * data has been acked.  Otherwise, if a timer is still
 * outstanding, cancel it.
 */
void TcpAgent::newtimer(Packet* pkt)
{
	TCPHeader *tcph = TCPHeader::access(pkt->bits());
        if (t_seqno_ > tcph->seqno())
		set_rtx_timer();
        else if (pending_[TCP_TIMER_RTX])
                cancel(TCP_TIMER_RTX);
}

/*
 * open up the congestion window
 */
void TcpAgent::opencwnd()
{
	if (cwnd_ < ssthresh_) {
		/* slow-start (exponential) */
		cwnd_ += 1;
	} else {
		/* linear */
		double f;
		switch (wnd_option_) {
		case 0:
			if (++count_ >= cwnd_) {
				count_ = 0;
				++cwnd_;
			}
			break;

		case 1:
			/* This is the standard algorithm. */
			cwnd_ += 1 / cwnd_;
			break;

		case 2:
			/* These are window increase algorithms
			 * for experimental purposes only. */
			f = (t_srtt_ >> 3) * tcp_tick_;
			f *= f;
			f *= wnd_const_;
			f += fcnt_;
			if (f > cwnd_) {
				fcnt_ = 0;
				++cwnd_;
			} else
				fcnt_ = f;
			break;

		case 3:
			f = awnd_;
			f *= f;
			f *= wnd_const_;
			f += fcnt_;
			if (f > cwnd_) {
				fcnt_ = 0;
				++cwnd_;
			} else
				fcnt_ = f;
			break;

                case 4:
                        f = awnd_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd_) {
                                fcnt_ = 0;
                                ++cwnd_;
                        } else
                                fcnt_ = f;
                        break;
		case 5:
                        f = (t_srtt_ >> 3) * tcp_tick_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd_) {
                                fcnt_ = 0;
                                ++cwnd_;
                        } else
                                fcnt_ = f;
                        break;

		default:
#ifdef notdef
			/*XXX*/
			error("illegal window option %d", wnd_option_);
#endif
			abort();
		}
	}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;

	return;
}

/*
 * close down the congestion window
 */
void TcpAgent::closecwnd(int how)
{   
	switch (how) {
	case 0:
		/* timeouts, Tahoe dup acks */
		ssthresh_ = int( window() / 2 );
		/* Old code: ssthresh_ = int(cwnd_ / 2); */
		cwnd_ = int(wnd_init_);
		break;

	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
		cwnd_ = window()/2;
		ssthresh_ = int(cwnd_);
		break;

	case 2:
		/* Tahoe dup acks  		*
		 * after a recent congestion indication */
		cwnd_ = wnd_init_;
		break;

	default:
		abort();
	}
	fcnt_ = 0.;
	count_ = 0;
}


/*
 * Process a packet that acks previously unacknowleges data.
 */
void TcpAgent::newack(Packet* pkt)
{
	TCPHeader *tcph = TCPHeader::access(pkt->bits());
	newtimer(pkt);
	dupacks_ = 0;
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;
	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
		rtt_active_ = 0;
		t_backoff_ = 1;
	}
	/* with timestamp option */
	double tao = Scheduler::instance().clock() - tcph->ts();
	rtt_update(tao);
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
}

void TcpAgent::plot()
{
#ifdef notyet
	double t = Scheduler::instance().clock();
	sprintf(trace_->buffer(), "t %g %d rtt %g\n", t, class_, t_rtt_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d dev %g\n", t, class_, t_rttvar_ * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d win %f\n", t, class_, cwnd_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d bck %d\n", t, class_, t_backoff_);
	trace_->dump();
#endif
}

/*
 * Respond either to a source quench or to a congestion indication bit.
 * This is done at most once a roundtrip time;  after a source quench,
 * another one will not be done until the last packet transmitted before
 * the previous source quench has been ACKed.
 */
void TcpAgent::quench(int how)
{
	if (highest_ack_ >= recover_) {
		recover_ = t_seqno_ - 1;
		recover_cause_ = 3;
		closecwnd(how);
#ifdef notdef
		if (trace_) {
			double now = Scheduler::instance().clock();
			sprintf(trace_->buffer(),
				"%g pkt %d\n", now, t_seqno_ - 1);
			trace_->dump();
		}
#endif
	}
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAgent::recv(Packet *pkt, Handler*)
{
	TCPHeader *tcph = TCPHeader::access(pkt->bits());
	IPHeader *iph = IPHeader::access(pkt->bits());
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	/*XXX only if ecn_ true*/
	if (iph->flags() & IP_ECN)
		quench(1);
	if (tcph->seqno() > last_ack_) {
		newack(pkt);
		opencwnd();
	} else if (tcph->seqno() == last_ack_) {
		if (++dupacks_ == NUMDUPACKS) {
                   /* The line below, for "bug_fix_" true, avoids
		    * problems with multiple fast retransmits in one
		    * window of data. 
		    */
		   	if (!bug_fix_ || highest_ack_ > recover_) {
				recover_ = maxseq_;
				recover_cause_ = 1;
				closecwnd(0);
				reset_rtx_timer(0);
			}
			else if (ecn_ && recover_cause_ != 1) {
				closecwnd(2);
				reset_rtx_timer(0);
			}
		}
	}
	Packet::free(pkt);
#ifdef notdef
	if (trace_)
		plot();
#endif
	/*
	 * Try to send more data.
	 */
	send(0, 0, maxburst_);
}

void TcpAgent::timeout(int tno)
{
	/* retransmit timer */
	if (tno == TCP_TIMER_RTX) {
		recover_ = maxseq_;
		recover_cause_ = 2;
		closecwnd(0);
		reset_rtx_timer(0);
		send(0, TCP_REASON_TIMEOUT);
	} else {
		/*
		 * delayed-send timer, with random overhead
		 * to avoid phase effects
		 */
		send(1, TCP_REASON_TIMEOUT);
	}
}
