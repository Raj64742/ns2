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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp.cc,v 1.13.2.4 1997/04/26 01:00:39 padmanab Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "Tcl.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"

/* Functions to read and write members of TcpClass */
int& maxseqf(TclObject *t) {
	return(((TcpAgent *)t)->maxseq());
}

int& highest_ackf(TclObject *t) {
	return(((TcpAgent *)t)->highest_ack());
}

int& t_seqnof(TclObject *t) {
	return(((TcpAgent *)t)->t_seqno());
}

double& cwndf(TclObject *t) {
	return(((TcpAgent *)t)->cwnd());
}

int& ssthreshf(TclObject *t) {
	return(((TcpAgent *)t)->ssthresh());
}

int& dupacksf(TclObject *t) {
	return(((TcpAgent *)t)->dupacks());
}

int& t_rttf(TclObject *t) {
	return(((TcpAgent *)t)->t_rtt());
}

int& t_srttf(TclObject *t) {
	return(((TcpAgent *)t)->t_srtt());
}

int& t_rttvarf(TclObject *t) {
	return(((TcpAgent *)t)->t_rttvar());
}

int& t_backofff(TclObject *t) {
	return(((TcpAgent *)t)->t_backoff());
}


static class TCPHeaderClass : public PacketHeaderClass {
public:
        TCPHeaderClass() : PacketHeaderClass("PacketHeader/TCP",
					     sizeof(hdr_tcp)) {}
} class_tcphdr;

static class TcpClass : public TclClass {
public:
	TcpClass() : TclClass("Agent/TCP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpAgent());
	}
} class_tcp;

TcpAgent::TcpAgent() : Agent(PT_TCP), rtt_active_(0), rtt_seq_(-1), last_log_time_(0), old_maxseq_(0), old_highest_ack_(0), old_t_seqno_(0), old_cwnd_(0), old_ssthresh_(0), old_dupacks_(0), old_t_rtt_(0), old_t_srtt_(0), old_t_rttvar_(0), old_t_backoff_(0), ts_peer_(0), dupacks_(0), t_seqno_(0), highest_ack_(0), cwnd_(0), ssthresh_(0), t_rtt_(0), t_srtt_(0), t_rttvar_(0), t_backoff_(0), curseq_(0), maxseq_(0)
{
/*	InstVarTrace *ivt;*/

/*	cwnd_trace_.cwnd_ptr() = &cwnd_;*/
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

	bind("dupacks_", &dupacks_, (TclObject *)this, dupacksf);
	bind("seqno_", &curseq_);
	bind("t_seqno_", &t_seqno_, (TclObject *)this, t_seqnof);
	bind("ack_", &highest_ack_, (TclObject *)this, highest_ackf);
	bind("cwnd_", &cwnd_, (TclObject *)this, cwndf);
	bind("awnd_", &awnd_);
	bind("ssthresh_", &ssthresh_, (TclObject *)this, ssthreshf);
	bind("rtt_", &t_rtt_, (TclObject *)this, t_rttf);
	bind("srtt_", &t_srtt_, (TclObject *)this, t_srttf);
	bind("rttvar_", &t_rttvar_, (TclObject *)this, t_rttvarf);
	bind("backoff_", &t_backoff_, (TclObject *)this, t_backofff);

	bind("off_ip_", &off_ip_);
	bind("off_tcp_", &off_tcp_);

	// reset used for dynamically created agent
	reset();
/*	ivt = (InstVarTrace *) &cwnd_trace_;
	ivt->update();*/
}

void TcpAgent::reset()
{
	rtt_init();
	/*XXX lookup variables */
	dupacks() = 0;
	curseq_ = 0;
	cwnd() = wnd_init_;
	t_seqno() = 0;
	maxseq() = -1;
	last_ack_ = -1;
	highest_ack() = -1;
	ssthresh() = int(wnd_);
	awnd_ = wnd_init_ / 2.0;
	recover_ = 0;
	recover_cause_ = 0;
}

/*
 * Initialize variables for the retransmit timer.
 */
void TcpAgent::rtt_init()
{
	t_rtt() = t_srtt() = 0;
	/* the setting of t_rttvar depends on the value for tcp_tick_ */
	t_rttvar() = int(3. / tcp_tick_);
	t_backoff() = 1;
}

/* This has been modified to use the tahoe code. */
double TcpAgent::rtt_timeout()
{
	double timeout = ((t_srtt() >> 3) + t_rttvar()) * tcp_tick_ ;
        timeout *= t_backoff();

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
	t_rtt() = int((tao / tcp_tick_) + 0.5);
	if (t_rtt() < 1)
		t_rtt() = 1;

        if (t_srtt() != 0) {
		register short delta;
		delta = t_rtt() - (t_srtt() >> 3);
		if ((t_srtt() += delta) <= 0)
			t_srtt() = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (t_rttvar() >> 2);
		if ((t_rttvar() += delta) <= 0)
			t_rttvar() = 1;
	} else {
		t_srtt() = t_rtt() << 3;
		t_rttvar() = t_rtt() << 1;
	}
}

void TcpAgent::rtt_backoff()
{
	if (t_backoff() < 64)
		t_backoff() <<= 1;

	if (t_backoff() > 8)
		/*
		 * If backed off this far, clobber the srtt
		 * value, storing it in the mean deviation
		 * instead.
		 */
		t_srtt() = 0;
}

void TcpAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	Connector::send(p, 0);
	if (seqno > maxseq()) {
		maxseq() = seqno;
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
			curseq_ = atoi(argv[2]);
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
			cwnd() = other->cwnd();
			awnd_ = other->awnd_;
			ssthresh() = other->ssthresh();
			t_rtt() = other->t_rtt();
			t_srtt() = other->t_srtt();
			t_rttvar() = other->t_rttvar();
			t_backoff() = other->t_backoff();
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

int TcpAgent::window()
{
	return (int(cwnd() < wnd_ ? cwnd() : wnd_));
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpAgent::send(int force, int reason, int maxburst)
{
	int win = window();
	int npackets = 0;

	if (!force && pending_[TCP_TIMER_DELSND])
		return;
	while (t_seqno() <= highest_ack() + win && t_seqno() < curseq_) {
		if (overhead_ == 0 || force) {
			output(t_seqno()++, reason);
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
		t_seqno() = highest_ack() + 1;
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
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
        if (t_seqno() > tcph->seqno())
		set_rtx_timer();
        else if (pending_[TCP_TIMER_RTX])
                cancel(TCP_TIMER_RTX);
}

/*
 * open up the congestion window
 */
void TcpAgent::opencwnd()
{
	if (cwnd() < ssthresh()) {
		/* slow-start (exponential) */
		cwnd() += 1;
	} else {
		/* linear */
		double f;
		switch (wnd_option_) {
		case 0:
			if (++count_ >= cwnd()) {
				count_ = 0;
				++cwnd();
			}
			break;

		case 1:
			/* This is the standard algorithm. */
			cwnd() += 1 / cwnd();
			break;

		case 2:
			/* These are window increase algorithms
			 * for experimental purposes only. */
			f = (t_srtt() >> 3) * tcp_tick_;
			f *= f;
			f *= wnd_const_;
			f += fcnt_;
			if (f > cwnd()) {
				fcnt_ = 0;
				++cwnd();
			} else
				fcnt_ = f;
			break;

		case 3:
			f = awnd_;
			f *= f;
			f *= wnd_const_;
			f += fcnt_;
			if (f > cwnd()) {
				fcnt_ = 0;
				++cwnd();
			} else
				fcnt_ = f;
			break;

                case 4:
                        f = awnd_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd()) {
                                fcnt_ = 0;
                                ++cwnd();
                        } else
                                fcnt_ = f;
                        break;
		case 5:
                        f = (t_srtt() >> 3) * tcp_tick_;
                        f *= wnd_const_;
                        f += fcnt_;
                        if (f > cwnd()) {
                                fcnt_ = 0;
                                ++cwnd();
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
	if (maxcwnd_ && (int(cwnd()) > maxcwnd_))
		cwnd() = maxcwnd_;

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
		ssthresh() = int( window() / 2 );
		/* Old code: ssthresh() = int(cwnd() / 2); */
		cwnd() = int(wnd_init_);
		break;

	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
		cwnd() = window()/2;
		ssthresh() = int(cwnd());
		break;

	case 2:
		/* Tahoe dup acks  		*
		 * after a recent congestion indication */
		cwnd() = wnd_init_;
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
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	newtimer(pkt);
	dupacks() = 0;
	last_ack_ = tcph->seqno();
	highest_ack() = last_ack_;
	if (t_seqno() < last_ack_ + 1)
		t_seqno() = last_ack_ + 1;
	if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
		rtt_active_ = 0;
		t_backoff() = 1;
	}
	/* with timestamp option */
	double tao = Scheduler::instance().clock() - tcph->ts_echo();
	rtt_update(tao);
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd();
}

void TcpAgent::plot()
{
#ifdef notyet
	double t = Scheduler::instance().clock();
	sprintf(trace_->buffer(), "t %g %d rtt %g\n", t, class_, t_rtt() * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d dev %g\n", t, class_, t_rttvar() * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d win %f\n", t, class_, cwnd());
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d bck %d\n", t, class_, t_backoff());
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
	if (highest_ack() >= recover_) {
		recover_ = t_seqno() - 1;
		recover_cause_ = 3;
		closecwnd(how);
#ifdef notdef
		if (trace_) {
			double now = Scheduler::instance().clock();
			sprintf(trace_->buffer(),
				"%g pkt %d\n", now, t_seqno() - 1);
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
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	ts_peer_ = tcph->ts();
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
	if (tcph->seqno() > last_ack_) {
		newack(pkt);
		opencwnd();
	} else if (tcph->seqno() == last_ack_) {
		if (++dupacks() == NUMDUPACKS) {
                   /* The line below, for "bug_fix_" true, avoids
		    * problems with multiple fast retransmits in one
		    * window of data. 
		    */
		   	if (!bug_fix_ || highest_ack() > recover_) {
				recover_ = maxseq();
				recover_cause_ = 1;
				closecwnd(0);
				reset_rtx_timer(0);
				fprintf(stderr,"doing fastrxt\n");
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
		recover_ = maxseq();
		recover_cause_ = 2;
		closecwnd(0);
		reset_rtx_timer(0);
		send(0, TCP_REASON_TIMEOUT, maxburst_);
	} else {
		/*
		 * delayed-send timer, with random overhead
		 * to avoid phase effects
		 */
		send(1, TCP_REASON_TIMEOUT, maxburst_);
	}
}
