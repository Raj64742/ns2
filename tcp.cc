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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp.cc,v 1.59 1998/04/28 21:27:48 bajaj Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"

static class TCPHeaderClass : public PacketHeaderClass {
public:
        TCPHeaderClass() : PacketHeaderClass("PacketHeader/TCP",
					     sizeof(hdr_tcp)) {}
} class_tcphdr;

static class TcpClass : public TclClass {
public:
	TcpClass() : TclClass("Agent/TCP") {}
	TclObject* create(int , const char*const*) {
		return (new TcpAgent());
	}
} class_tcp;

TcpAgent::TcpAgent() : Agent(PT_TCP), rtt_active_(0), rtt_seq_(-1),
	rtt_ts_(0.0), ts_peer_(0),dupacks_(0), t_seqno_(0),
	highest_ack_(0), cwnd_(0),
	ssthresh_(0), t_rtt_(0), t_srtt_(0), t_rttvar_(0),
	t_backoff_(0), curseq_(0), maxseq_(0), closed_(0), restart_bugfix_(1),
	rtx_timer_(this), delsnd_timer_(this), burstsnd_timer_(this),
	count_(0), fcnt_(0)
{
	// Defaults for bound variables should be set in ns-default.tcl.
	bind("window_", &wnd_);
	bind("windowInit_", &wnd_init_);
	bind("windowInitOption_", &wnd_init_option_);
	bind_bool("syn_", &syn_);
	bind("windowOption_", &wnd_option_);
	bind("windowConstant_", &wnd_const_);
	bind("windowThresh_", &wnd_th_);
	bind_bool("delay_growth_", &delay_growth_);
	bind("overhead_", &overhead_);
	bind("tcpTick_", &tcp_tick_);
	bind_bool("ecn_", &ecn_);
        bind("eln_", &eln_);
        bind("eln_rxmit_thresh_", &eln_rxmit_thresh_);
	bind("packetSize_", &size_);
	bind("tcpip_base_hdr_size_", &tcpip_base_hdr_size_);
	bind_bool("bugFix_", &bug_fix_);
	bind_bool("slow_start_restart_", &slow_start_restart_);
	bind_bool("restart_bugfix_", &restart_bugfix_);
	bind_bool("timestamps_", &ts_option_);
	bind("maxburst_", &maxburst_);
	bind("maxcwnd_", &maxcwnd_);
	bind("maxrto_", &maxrto_);
	bind("srtt_init_", &srtt_init_);
	bind("rttvar_init_", &rttvar_init_);
	bind("rtxcur_init_", &rtxcur_init_);

	bind("dupacks_", &dupacks_);
	bind("seqno_", &curseq_);
	bind("t_seqno_", &t_seqno_);
	bind("ack_", &highest_ack_);
	bind("cwnd_", &cwnd_);
	bind("awnd_", &awnd_);
	bind("ssthresh_", &ssthresh_);
	bind("rtt_", &t_rtt_);
	bind("srtt_", &t_srtt_);
	bind("rttvar_", &t_rttvar_);
	bind("backoff_", &t_backoff_);
	bind("maxseq_", &maxseq_);
	bind("off_ip_", &off_ip_);
	bind("off_tcp_", &off_tcp_);
        bind("ndatapack_", &ndatapack_);
        bind("ndatabytes_", &ndatabytes_);
        bind("nackpack_", &nackpack_);
        bind("nrexmit_", &nrexmit_);
        bind("nrexmitpack_", &nrexmitpack_);
        bind("nrexmitbytes_", &nrexmitbytes_);
	bind_bool("trace_all_oneline_", &trace_all_oneline_);

	// reset used for dynamically created agent
	reset();
}

/* Print out all the traced variables whenever any one is changed */
void
TcpAgent::traceAll() {
	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[500];
	int n;

	curtime = &s ? s.clock() : 0;
	sprintf(wrk,"time: %-8.5f saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d maxseq: %-4d hiack: %-4d seqno: %-4d cwnd: %-6.3f ssthresh: %-3d dupacks: %-2d rtt: %-6.3f srtt: %-6.3f rttvar: %-6.3f bkoff: %-d", curtime, addr_/256, addr_%256, dst_/256, dst_%256, int(maxseq_), int(highest_ack_), int(t_seqno_), double(cwnd_), int(ssthresh_), int(dupacks_), int(t_rtt_)*tcp_tick_, (int(t_srtt_) >> T_SRTT_BITS)*tcp_tick_, int(t_rttvar_)*tcp_tick_/4.0, int(t_backoff_));
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;
	return;
}

/* Print out just the variable that is modified */
void
TcpAgent::traceVar(TracedVar* v) 
{
	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[500];
	int n;

	curtime = &s ? s.clock() : 0;
	if (!strcmp(v->name(), "cwnd_") || !strcmp(v->name(), "maxrto_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", curtime, addr_/256, addr_%256, dst_/256, dst_%256, v->name(), double(*((TracedDouble*) v)));
	else if (!strcmp(v->name(), "rtt_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", curtime, addr_/256, addr_%256, dst_/256, dst_%256, v->name(), int(*((TracedInt*) v))*tcp_tick_);
	else if (!strcmp(v->name(), "srtt_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", curtime, addr_/256, addr_%256, dst_/256, dst_%256, v->name(), (int(*((TracedInt*) v)) >> T_SRTT_BITS)*tcp_tick_);
	else if (!strcmp(v->name(), "rttvar_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", curtime, addr_/256, addr_%256, dst_/256, dst_%256, v->name(), int(*((TracedInt*) v))*tcp_tick_/4.0);
	else
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %d", curtime, addr_/256, addr_%256, dst_/256, dst_%256, v->name(), int(*((TracedInt*) v)));
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;
	return;
}

void
TcpAgent::trace(TracedVar* v) 
{
	if (trace_all_oneline_)
		traceAll();
	else 
		traceVar(v);
}

//
// in 1-way TCP, syn_ indicates we are modeling
// a SYN exchange at the beginning.  If this is true
// and we are delaying growth, then use an initial
// window of one.  If not, we do whatever initial_window()
// says to do.
//

void
TcpAgent::set_initial_window()
{
	if (syn_ && delay_growth_)
		cwnd_ = 1.0; 
	else
		cwnd_ = initial_window();
}

void
TcpAgent::reset()
{
	rtt_init();
	/*XXX lookup variables */
	dupacks_ = 0;
	curseq_ = 0;
	set_initial_window();

	t_seqno_ = 0;
	maxseq_ = -1;
	last_ack_ = -1;
	highest_ack_ = -1;
	ssthresh_ = int(wnd_);
	wnd_restart_ = 1.;
	awnd_ = wnd_init_ / 2.0;
	recover_ = 0;
	
	recover_cause_ = 0;
	boot_time_ = Random::uniform(tcp_tick_);
}

/*
 * Initialize variables for the retransmit timer.
 */
void TcpAgent::rtt_init()
{
	t_rtt_ = 0;
	t_srtt_ = int(srtt_init_ / tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_ / tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	t_backoff_ = 1;
}

/* This has been modified to use the tahoe code. */
double TcpAgent::rtt_timeout()
{
	double timeout;
        timeout = t_rtxcur_ * t_backoff_;

	if (timeout > maxrto_)
		timeout = maxrto_;

        if (timeout < 2 * tcp_tick_) {
		if (timeout < 0) {
			fprintf(stderr, "TcpAgent: negative RTO! (%f)\n",
				timeout);
			exit(1);
		}
		timeout = 2 * tcp_tick_;
	}
	return (timeout);
}

/* This has been modified to use the tahoe code. */
void TcpAgent::rtt_update(double tao)
{
	double now = Scheduler::instance().clock();
	if (ts_option_)
		t_rtt_ = int(tao /tcp_tick_ + 0.5);
	else {
		double sendtime = now - tao;
		sendtime += boot_time_;
		double tickoff = fmod(sendtime, tcp_tick_);
		t_rtt_ = int((tao + tickoff) / tcp_tick_);
	}
	if (t_rtt_ < 1)
		t_rtt_ = 1;

	//
	// srtt has 3 bits to the right of the binary point
	// rttvar has 2
	//
        if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);	// d = (m - a0)
		if ((t_srtt_ += delta) <= 0)	// a1 = 7/8 a0 + 1/8 m
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (t_rttvar_ >> T_RTTVAR_BITS);
		if ((t_rttvar_ += delta) <= 0)	// var1 = 3/4 var0 + 1/4 |d|
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		// srtt = rtt
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	// rttvar = rtt / 2
	}
	//
	// Current retransmit value is 
	//    (unscaled) smoothed round trip estimate
	//    plus 4 times (unscaled) rttvar. 
	//
	t_rtxcur_ = (((t_rttvar_ << (2 + (T_SRTT_BITS - T_RTTVAR_BITS))) +
		t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;

	return;
}

void TcpAgent::rtt_backoff()
{
	if (t_backoff_ < 64)
		t_backoff_ <<= 1;

	if (t_backoff_ > 8) {
		/*
		 * If backed off this far, clobber the srtt
		 * value, storing it in the mean deviation
		 * instead.
		 */
		t_rttvar_ += (t_srtt_ >> T_SRTT_BITS);
		t_srtt_ = 0;
	}
}

void TcpAgent::output(int seqno, int reason)
{
	int force_set_rtx_timer = 0;
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	tcph->seqno() = seqno;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	if (ecn_) {
		hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
		hf->ecn_capable_ = 1;
	}
	/* Check if this is the initial SYN packet. */
	if (syn_ && (seqno == 0)) 
		((hdr_cmn*)p->access(off_cmn_))->size() = tcpip_base_hdr_size_;
        int bytes = ((hdr_cmn*)p->access(off_cmn_))->size();

	/* if no outstanding data, be sure to set rtx timer again */
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	/* call helper function to fill in additional fields */
	output_helper(p);

        ++ndatapack_;
        ndatabytes_ += bytes;
	send(p, 0);
	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;
				rtt_ts_ = Scheduler::instance().clock();
			}
					
		}
	} else {
        	++nrexmitpack_;
        	nrexmitbytes_ += bytes;
	}
	if (!(rtx_timer_.status() == TIMER_PENDING) || force_set_rtx_timer)
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

void TcpAgent::advanceby(int delta)
{
        curseq_ += delta;
	if (delta > 0)
		closed_ = 0;
	send_much(0, 0, maxburst_); 
}


int TcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			int newseq = atoi(argv[2]);
			if (newseq >= curseq_)   // this is kind of silly but avoids duplicating ::advanceby
				advanceby(newseq - curseq_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advanceby") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		/*
		 * Curtis Villamizar's trick to transfer tcp connection
		 * parameters to emulate http persistent connections.
		 *
		 * Another way to do the same thing is to open one tcp
		 * object and use start/stop/maxpkts_ or advanceby to control
		 * how much data is sent in each burst.
		 * With a single connection, slow_start_restart_
		 * should be configured as desired.
		 *
		 * This implementation (persist) may not correctly
		 * emulate pure-BSD-based systems which close cwnd
		 * after the connection goes idle (slow-start
		 * restart).  See Jacobson and Karels ``Congestion
		 * Avoidance and Control'' in CCR (*not* the original
		 * '88 paper) for why BSD does this.  See
		 * ``Performance Interactions Between P-HTTP and TCP
		 * Implementations'' in CCR 27(2) for descriptions of
		 * what other systems do the same.
		 *
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
	return (cwnd_ < wnd_ ? (int)cwnd_ : (int)wnd_);
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpAgent::send_much(int force, int reason, int maxburst)
{
	send_idle_helper();
	int win = window();
	int npackets = 0;

	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	/* Save time when first packet was sent, for newreno  --Allman */
	if (t_seqno_ == 0)
		firstsent_ = Scheduler::instance().clock();

	if (burstsnd_timer_.status() == TIMER_PENDING)
		return;
	while (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
		if (overhead_ == 0 || force) {
			output(t_seqno_++, reason);
			npackets++;
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npackets == maxburst)
			break;
	}
	/* call helper function */
	send_helper(maxburst);
}

/*
 * We got a timeout or too many duplicate acks.  Clear the retransmit timer.  
 * Resume the sequence one past the last packet acked.  
 * "mild" is 0 for timeouts and Tahoe dup acks, 1 for Reno dup acks.
 * "backoff" is 1 if the timer should be backed off, 0 otherwise.
 */
void TcpAgent::reset_rtx_timer(int mild, int backoff)
{
	if (backoff)
		rtt_backoff();
	set_rtx_timer();
	if (!mild)
		t_seqno_ = highest_ack_ + 1;
	rtt_active_ = 0;
}

/*
 * Set retransmit timer using current rtt estimate.  By calling resched(), 
 * it does not matter whether the timer was already running.
 */
void TcpAgent::set_rtx_timer()
{
	rtx_timer_.resched(rtt_timeout());
}

/*
 * Set new retransmission timer if not all outstanding
 * data has been acked.  Otherwise, if a timer is still
 * outstanding, cancel it.
 */
void TcpAgent::newtimer(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	if (t_seqno_ > tcph->seqno())
		set_rtx_timer();
	else
		cancel_rtx_timer();
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
			f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
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
                        f = (t_srtt_ >> T_SRTT_BITS) * tcp_tick_;
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
		/* timeouts */
		ssthresh_ = int( window() / 2 );
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		cwnd_ = int(wnd_restart_);
		break;

	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
		cwnd_ = window()/2;
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;		
		break;

	case 2:
		/* Tahoe dup acks  		
		 * after a recent congestion indication */
		cwnd_ = wnd_init_;
		break;

	case 3:
		/* Retransmit timeout, but no outstanding data. */ 
		cwnd_ = int(wnd_init_);
		break;
	case 4:
		/* Tahoe dup acks */
		ssthresh_ = int( window() / 2 );
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		cwnd_ = 1;
		break;

	default:
		abort();
	}
	fcnt_ = 0.;
	count_ = 0;
}


/*
 * Process a packet that acks previously unacknowleged data.
 */
void TcpAgent::newack(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	newtimer(pkt);
	dupacks_ = 0;
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = (hdr_flags *)pkt->access(off_flags_);
	if (!fh->no_ts_) {
		if (ts_option_)
			rtt_update(now - tcph->ts_echo());

		if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
			t_backoff_ = 1;
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - rtt_ts_);
		}
	}
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

void TcpAgent::recv_newack_helper(Packet *pkt) {
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	newack(pkt);
	if ( !((hdr_flags*)pkt->access(off_flags_))->ecn_ || !ecn_ ) {
	        opencwnd();
	}
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

/*
 * Set the initial window. 
 */
double
TcpAgent::initial_window()
{
	//
	// init_option = 1: static iw of wnd_init_
	//
	if (wnd_init_option_ == 1) {
		return (wnd_init_);
	}
        else if (wnd_init_option_ == 2) {
		// do iw according to Internet draft
 		if (size_ <= 1095) {
			return (4.0);
	 	} else if (size_ < 2190) {
			return (3.0);
		} else {
			return (2.0);
		}
	}
}


/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif
	++nackpack_;
	ts_peer_ = tcph->ts();
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_ && ecn_)
		quench(1);
	recv_helper(pkt);
	/* grow cwnd and check if the connection is done */ 
	if (tcph->seqno() > last_ack_) {
		recv_newack_helper(pkt);
		if (last_ack_ == 0 && delay_growth_) {
			cwnd_ = initial_window();
		}
	} else if (tcph->seqno() == last_ack_) {
                if (((hdr_flags*)pkt->access(off_flags_))->eln_ && eln_) {
                        tcp_eln(pkt);
                        return;
                }
		if (++dupacks_ == NUMDUPACKS) {
                   /* The line below, for "bug_fix_" true, avoids
		    * problems with multiple fast retransmits in one
		    * window of data. 
		    */
		   	if (!bug_fix_ || highest_ack_ > recover_) {
				recover_ = maxseq_;
				recover_cause_ = 1;
				closecwnd(4);
				reset_rtx_timer(0,0);
			}
			else if (ecn_ && recover_cause_ != 1) {
				closecwnd(2);
				reset_rtx_timer(0,0);
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
	send_much(0, 0, maxburst_);
}

/*
 * Process timeout events other than rtx timeout. Having this as a separate 
 * function allows derived classes to make alterations/enhancements (e.g.,
 * response to new types of timeout events).
 */ 
void TcpAgent::timeout_nonrtx(int) 
{
	/*
	 * delayed-send timer, with random overhead
	 * to avoid phase effects
	 */
	send_much(1, TCP_REASON_TIMEOUT, maxburst_);
}
	
void TcpAgent::timeout(int tno)
{
	/* retransmit timer */
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 */
			return;
		};
		recover_ = maxseq_;
		recover_cause_ = 2;
		if (highest_ack_ == -1 && wnd_init_option_ == 2)
			/* 
			 * First packet dropped, so don't use larger
			 * initial windows. 
			 */
			wnd_init_option_ = 1;
		/* if there is no outstanding data, don't cut down ssthresh_ */
		if (highest_ack_ == maxseq_ && restart_bugfix_)
			closecwnd(3);
		else {
			++nrexmit_;
			closecwnd(0);
		}
		/* if there is no outstanding data, don't back off rtx timer */
		if (highest_ack_ == maxseq_ && restart_bugfix_) {
			reset_rtx_timer(0,0);
		}
		else {
			reset_rtx_timer(0,1);
		}
		send_much(0, TCP_REASON_TIMEOUT, maxburst_);

	} 
	else {
		timeout_nonrtx(tno);
	}
}

/* 
 * Check if the packet (ack) has the ELN bit set, and if it does, and if the
 * last ELN-rxmitted packet is smaller than this one, then retransmit the
 * packet.  Do not adjust the cwnd when this happens.
 */
void TcpAgent::tcp_eln(Packet *pkt)
{
        int eln_rxmit;
        hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
        int ack = tcph->seqno();

        if (++dupacks_ == eln_rxmit_thresh_ && ack > eln_last_rxmit_) {
                /* Retransmit this packet */
                output(last_ack_ + 1, TCP_REASON_DUPACK);
                eln_last_rxmit_ = last_ack_+1;
        } else
                send_much(0, 0, maxburst_);

        Packet::free(pkt);
        return;
}

/*
 * This function is invoked when the connection is done. It in turn
 * invokes the Tcl finish procedure that was registered with TCP.
 */
void TcpAgent::finish()
{
	Tcl::instance().evalf("%s done", this->name());
}

void RtxTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_RTX);
}

void DelSndTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_DELSND);
}

void BurstSndTimer::expire(Event*)
{
	a_->timeout(TCP_TIMER_BURSTSND);
}

