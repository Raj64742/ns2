/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp.cc,v 1.122 2001/05/21 19:27:31 haldar Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "random.h"
#include "basetrace.h"

int hdr_tcp::offset_;

static class TCPHeaderClass : public PacketHeaderClass {
public:
        TCPHeaderClass() : PacketHeaderClass("PacketHeader/TCP",
					     sizeof(hdr_tcp)) {
		bind_offset(&hdr_tcp::offset_);
	}
} class_tcphdr;

static class TcpClass : public TclClass {
public:
	TcpClass() : TclClass("Agent/TCP") {}
	TclObject* create(int , const char*const*) {
		return (new TcpAgent());
	}
} class_tcp;

TcpAgent::TcpAgent() : Agent(PT_TCP), 
	t_seqno_(0), t_rtt_(0), t_srtt_(0), t_rttvar_(0), 
	t_backoff_(0), ts_peer_(0), 
	rtx_timer_(this), delsnd_timer_(this), 
	burstsnd_timer_(this), 
	dupacks_(0), curseq_(0), highest_ack_(0), cwnd_(0), ssthresh_(0), 
	count_(0), fcnt_(0), rtt_active_(0), rtt_seq_(-1), rtt_ts_(0.0), 
	maxseq_(0), cong_action_(0), ecn_burst_(0), ecn_backoff_(0),
	ect_(0), restart_bugfix_(1), closed_(0), nrexmit_(0),
	first_decrease_(1), et_(0)
	
{
#ifdef TCP_DELAY_BIND_ALL
#else /* ! TCP_DELAY_BIND_ALL */
	// not delay-bound because delay-bound tracevars aren't yet supported
	bind("t_seqno_", &t_seqno_);
	bind("rtt_", &t_rtt_);
	bind("srtt_", &t_srtt_);
	bind("rttvar_", &t_rttvar_);
	bind("backoff_", &t_backoff_);
	bind("dupacks_", &dupacks_);
	bind("seqno_", &curseq_);
	bind("ack_", &highest_ack_);
	bind("cwnd_", &cwnd_);
	bind("ssthresh_", &ssthresh_);
	bind("maxseq_", &maxseq_);
        bind("ndatapack_", &ndatapack_);
        bind("ndatabytes_", &ndatabytes_);
        bind("nackpack_", &nackpack_);
        bind("nrexmit_", &nrexmit_);
        bind("nrexmitpack_", &nrexmitpack_);
        bind("nrexmitbytes_", &nrexmitbytes_);
	bind("singledup_", &singledup_);
#endif /* TCP_DELAY_BIND_ALL */

}

void
TcpAgent::delay_bind_init_all()
{

        // Defaults for bound variables should be set in ns-default.tcl.
        delay_bind_init_one("window_");
        delay_bind_init_one("windowInit_");
        delay_bind_init_one("windowInitOption_");

        delay_bind_init_one("syn_");
        delay_bind_init_one("windowOption_");
        delay_bind_init_one("windowConstant_");
        delay_bind_init_one("windowThresh_");
        delay_bind_init_one("delay_growth_");
        delay_bind_init_one("overhead_");
        delay_bind_init_one("tcpTick_");
        delay_bind_init_one("ecn_");
        delay_bind_init_one("old_ecn_");
        delay_bind_init_one("eln_");
        delay_bind_init_one("eln_rxmit_thresh_");
        delay_bind_init_one("packetSize_");
        delay_bind_init_one("tcpip_base_hdr_size_");
        delay_bind_init_one("bugFix_");
        delay_bind_init_one("slow_start_restart_");
        delay_bind_init_one("restart_bugfix_");
        delay_bind_init_one("timestamps_");
        delay_bind_init_one("maxburst_");
        delay_bind_init_one("maxcwnd_");
	delay_bind_init_one("numdupacks_");
        delay_bind_init_one("maxrto_");
	delay_bind_init_one("minrto_");
        delay_bind_init_one("srtt_init_");
        delay_bind_init_one("rttvar_init_");
        delay_bind_init_one("rtxcur_init_");
        delay_bind_init_one("T_SRTT_BITS");
        delay_bind_init_one("T_RTTVAR_BITS");
        delay_bind_init_one("rttvar_exp_");
        delay_bind_init_one("awnd_");
        delay_bind_init_one("decrease_num_");
        delay_bind_init_one("increase_num_");
	delay_bind_init_one("k_parameter_");
	delay_bind_init_one("l_parameter_");
        delay_bind_init_one("trace_all_oneline_");
        delay_bind_init_one("nam_tracevar_");

        delay_bind_init_one("QOption_");
        delay_bind_init_one("EnblRTTCtr_");
        delay_bind_init_one("control_increase_");
	delay_bind_init_one("noFastRetrans_");
	delay_bind_init_one("precisionReduce_");
	delay_bind_init_one("oldCode_");
	delay_bind_init_one("timerfix_");

#ifdef TCP_DELAY_BIND_ALL
	// out because delay-bound tracevars aren't yet supported
        delay_bind_init_one("t_seqno_");
        delay_bind_init_one("rtt_");
        delay_bind_init_one("srtt_");
        delay_bind_init_one("rttvar_");
        delay_bind_init_one("backoff_");
        delay_bind_init_one("dupacks_");
        delay_bind_init_one("seqno_");
        delay_bind_init_one("ack_");
        delay_bind_init_one("cwnd_");
        delay_bind_init_one("ssthresh_");
        delay_bind_init_one("maxseq_");
        delay_bind_init_one("ndatapack_");
        delay_bind_init_one("ndatabytes_");
        delay_bind_init_one("nackpack_");
        delay_bind_init_one("nrexmit_");
        delay_bind_init_one("nrexmitpack_");
        delay_bind_init_one("nrexmitbytes_");
	delay_bind_init_one("singledup_");
#endif /* TCP_DELAY_BIND_ALL */

	Agent::delay_bind_init_all();

        reset();
}

int
TcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind(varName, localName, "window_", &wnd_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowInit_", &wnd_init_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowInitOption_", &wnd_init_option_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "syn_", &syn_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowOption_", &wnd_option_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowConstant_",  &wnd_const_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "windowThresh_", &wnd_th_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "delay_growth_", &delay_growth_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "overhead_", &overhead_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcpTick_", &tcp_tick_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "ecn_", &ecn_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "old_ecn_", &old_ecn_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "eln_", &eln_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "eln_rxmit_thresh_", &eln_rxmit_thresh_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "packetSize_", &size_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcpip_base_hdr_size_", &tcpip_base_hdr_size_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "bugFix_", &bug_fix_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "slow_start_restart_", &slow_start_restart_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "restart_bugfix_", &restart_bugfix_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "timestamps_", &ts_option_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxburst_", &maxburst_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxcwnd_", &maxcwnd_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "numdupacks_", &numdupacks_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxrto_", &maxrto_ , tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "minrto_", &minrto_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "srtt_init_", &srtt_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_init_", &rttvar_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rtxcur_init_", &rtxcur_init_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "T_SRTT_BITS", &T_SRTT_BITS , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "T_RTTVAR_BITS", &T_RTTVAR_BITS , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_exp_", &rttvar_exp_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "awnd_", &awnd_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "decrease_num_", &decrease_num_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "increase_num_", &increase_num_, tracer)) return TCL_OK;
	if (delay_bind(varName, localName, "k_parameter_", &k_parameter_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "l_parameter_", &l_parameter_, tracer)) return TCL_OK;


        if (delay_bind_bool(varName, localName, "trace_all_oneline_", &trace_all_oneline_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nam_tracevar_", &nam_tracevar_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "QOption_", &QOption_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "EnblRTTCtr_", &EnblRTTCtr_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "control_increase_", &control_increase_ , tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "oldCode_", &oldCode_, tracer)) return TCL_OK;
	if (delay_bind_bool(varName, localName, "timerfix_", &timerfix_, tracer)) return TCL_OK;


#ifdef TCP_DELAY_BIND_ALL
	// not if (delay-bound delay-bound tracevars aren't yet supported
        if (delay_bind(varName, localName, "t_seqno_", &t_seqno_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rtt_", &t_rtt_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "srtt_", &t_srtt_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "rttvar_", &t_rttvar_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "backoff_", &t_backoff_ , tracer)) return TCL_OK;

        if (delay_bind(varName, localName, "dupacks_", &dupacks_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "seqno_", &curseq_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ack_", &highest_ack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "cwnd_", &cwnd_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ssthresh_", &ssthresh_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "maxseq_", &maxseq_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ndatapack_", &ndatapack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ndatabytes_", &ndatabytes_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nackpack_", &nackpack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmit_", &nrexmit_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmitpack_", &nrexmitpack_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "nrexmitbytes_", &nrexmitbytes_ , tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "singledup_", &singledup_ , tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "noFastRetrans_", &noFastRetrans_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "precisionReduce_", &precision_reduce_, tracer)) return TCL_OK;
#endif

        return Agent::delay_bind_dispatch(varName, localName, tracer);
}

/* Print out all the traced variables whenever any one is changed */
void
TcpAgent::traceAll() {
	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[500];
	int n;

	curtime = &s ? s.clock() : 0;
	sprintf(wrk,"time: %-8.5f saddr: %-2d sport: %-2d daddr: %-2d dport:"
		" %-2d maxseq: %-4d hiack: %-4d seqno: %-4d cwnd: %-6.3f"
		" ssthresh: %-3d dupacks: %-2d rtt: %-6.3f srtt: %-6.3f"
		" rttvar: %-6.3f bkoff: %-d", curtime, addr(), port(),
		daddr(), dport(), int(maxseq_), int(highest_ack_),
		int(t_seqno_), double(cwnd_), int(ssthresh_),
		int(dupacks_), int(t_rtt_)*tcp_tick_, 
		(int(t_srtt_) >> T_SRTT_BITS)*tcp_tick_, 
		int(t_rttvar_)*tcp_tick_/4.0, int(t_backoff_)); 
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
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), double(*((TracedDouble*) v))); 
	else if (!strcmp(v->name(), "minrto_")) 
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), double(*((TracedDouble*) v))); 
	else if (!strcmp(v->name(), "rtt_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), int(*((TracedInt*) v))*tcp_tick_); 
	else if (!strcmp(v->name(), "srtt_")) 
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), 
			(int(*((TracedInt*) v)) >> T_SRTT_BITS)*tcp_tick_); 
	else if (!strcmp(v->name(), "rttvar_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), 
			int(*((TracedInt*) v))*tcp_tick_/4.0); 
	else
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %d",
			curtime, addr(), port(), daddr(), dport(),
			v->name(), int(*((TracedInt*) v))); 
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
	if (nam_tracevar_) {
		Agent::trace(v);
	} else if (trace_all_oneline_)
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
TcpAgent::reset_qoption()
{
	int now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);

	T_start = now ; 
	RTT_count = 0 ; 
	RTT_prev = 0 ; 
	RTT_goodcount = 1 ; 
	F_counting = 0 ; 
	W_timed = -1 ; 
	F_full = 0 ;
	Backoffs = 0 ; 
}

void
TcpAgent::reset()
{
	rtt_init();
	rtt_seq_ = -1;
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
	closed_ = 0;
	last_cwnd_action_ = 0;
	boot_time_ = Random::uniform(tcp_tick_);
	first_decrease_ = 1;

	/* Now these variables will be reset 
	   - Debojyoti Dutta 12th Oct'2000 */
 
	ndatapack_ = 0;
	ndatabytes_ = 0;
	nackpack_ = 0;
	nrexmitbytes_ = 0;
	nrexmit_ = 0;
	nrexmitpack_ = 0;

	if (control_increase_) {
		prev_highest_ack_ = highest_ack_ ; 
	}

	if (QOption_) {
		int now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
		T_last = now ; 
		T_prev = now ; 
		W_used = 0 ;
		if (EnblRTTCtr_) {
			reset_qoption();
		}
	}
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
	if (timeout < minrto_)
		timeout = minrto_;

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
	//    plus 2^rttvar_exp_ times (unscaled) rttvar. 
	//
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) +
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
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags* hf = hdr_flags::access(p);
	tcph->seqno() = seqno;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	if (ecn_) {
		hf->ect() = 1;	// ECN-capable transport
	}
	if (cong_action_) {
		hf->cong_action() = TRUE;  // Congestion action.
		cong_action_ = FALSE;
        }
	/* Check if this is the initial SYN packet. */
	if (seqno == 0) {
		if (syn_) {
			hdr_cmn::access(p)->size() = tcpip_base_hdr_size_;
		}
		if (ecn_) {
			hf->ecnecho() = 1;
//			hf->cong_action() = 1;
			hf->ect() = 0;
		}
	}
        int bytes = hdr_cmn::access(p)->size();

	/* if no outstanding data, be sure to set rtx timer again */
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	/* call helper function to fill in additional fields */
	output_helper(p);

        ++ndatapack_;
        ndatabytes_ += bytes;
	send(p, 0);
	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far
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

/*
 * Must convert bytes into packets for one-way TCPs.
 * If nbytes == -1, this corresponds to infinite send.  We approximate
 * infinite by a very large number (TCP_MAXSEQ).
 */
void TcpAgent::sendmsg(int nbytes, const char* /*flags*/)
{
	if (nbytes == -1 && curseq_ <= TCP_MAXSEQ)
		curseq_ = TCP_MAXSEQ; 
	else
		curseq_ += (nbytes/size_ + (nbytes%size_ ? 1 : 0));
	send_much(0, 0, maxburst_);
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
			if (newseq > maxseq_)
				advanceby(newseq - curseq_);
			else
				advanceby(maxseq_ - curseq_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advanceby") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
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
		 * restart).  See appendix C in
		 * Jacobson and Karels ``Congestion
		 * Avoidance and Control'' at
		 * <ftp://ftp.ee.lbl.gov/papers/congavoid.ps.Z>
		 * (*not* the original
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

double TcpAgent::windowd()
{
	return (cwnd_ < wnd_ ? (double)cwnd_ : (double)wnd_);
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
			output(t_seqno_, reason);
			npackets++;
			if (QOption_)
				process_qoption_after_send () ; 
			t_seqno_ ++ ;
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		win = window();
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
 * or available data acked, or if we are unable to send because 
 * cwnd is less than one (as when the ECN bit is set when cwnd was 1).
 * Otherwise, if a timer is still outstanding, cancel it.
 */
void TcpAgent::newtimer(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/*
	 * t_seqno_, the next packet to send, is reset (decreased) 
	 *   to highest_ack_ + 1 after a timeout,
	 *   so we also have to check maxseq_, the highest seqno sent.
	 * In addition, if the packet sent after the timeout has
	 *   the ECN bit set, then the returning ACK caused cwnd_ to
	 *   be decreased to less than one, and we can't send another
	 *   packet until the retransmit timer again expires.
	 *   So we have to check for "cwnd_ < 1" as well.
	 */
	if (t_seqno_ > tcph->seqno() || tcph->seqno() < maxseq_ || cwnd_ < 1) 
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
			cwnd_ += increase_num_ / cwnd_;
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
                case 6:
                        /* binomial controls */ 
                        cwnd_ += increase_num_ / (cwnd_*pow(cwnd_,k_parameter_));                
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

void
TcpAgent::slowdown(int how)
{
	double win, halfwin, decreasewin;
	int slowstart = 0;
	// we are in slowstart for sure if cwnd < ssthresh
	if (cwnd_ < ssthresh_)
		slowstart = 1;
	// we are in slowstart - need to trace this event
	trace_event("SLOW_START");

        if (precision_reduce_) {
		halfwin = windowd() / 2;
                if (wnd_option_ == 6) {
                        /* binomial controls */
                        decreasewin = windowd() - (1.0-decrease_num_)*pow(windowd(),l_parameter_);
                } else
	 		decreasewin = decrease_num_ * windowd();
		win = windowd();
	} else  {
		int temp;
		temp = (int)(window() / 2);
		halfwin = (double) temp;
                if (wnd_option_ == 6) {
                        /* binomial controls */
                        temp = (int)(window() - (1.0-decrease_num_)*pow(window(),l_parameter_));
                } else
	 		temp = (int)(decrease_num_ * window());
		decreasewin = (double) temp;
		win = (double) window();
	}
	if (how & CLOSE_SSTHRESH_HALF)
		// For the first decrease, decrease by half
		// even for non-standard values of decrease_num_.
		if (first_decrease_ == 1 || slowstart ||
			last_cwnd_action_ == CWND_ACTION_TIMEOUT) {
			// Do we really want halfwin instead of decreasewin
			// after a timeout?
			ssthresh_ = (int) halfwin;
		} else {
			ssthresh_ = (int) decreasewin;
		}
        else if (how & THREE_QUARTER_SSTHRESH)
		if (ssthresh_ < 3*cwnd_/4)
			ssthresh_  = (int)(3*cwnd_/4);
	if (how & CLOSE_CWND_HALF)
		// For the first decrease, decrease by half
		// even for non-standard values of decrease_num_.
		if (first_decrease_ == 1 || slowstart || decrease_num_ == 0.5) {
			cwnd_ = halfwin;
		} else cwnd_ = decreasewin;
        else if (how & CWND_HALF_WITH_MIN) {
		// We have not thought about how non-standard TCPs, with
		// non-standard values of decrease_num_, should respond
		// after quiescent periods.
                cwnd_ = decreasewin;
                if (cwnd_ < 1)
                        cwnd_ = 1;
	}
	else if (how & CLOSE_CWND_RESTART) 
		cwnd_ = int(wnd_restart_);
	else if (how & CLOSE_CWND_INIT)
		cwnd_ = int(wnd_init_);
	else if (how & CLOSE_CWND_ONE)
		cwnd_ = 1;
	else if (how & CLOSE_CWND_HALF_WAY) {
		// cwnd_ = win - (win - W_used)/2 ;
		cwnd_ = W_used + decrease_num_ * (win - W_used);
                if (cwnd_ < 1)
                        cwnd_ = 1;
	}
	if (ssthresh_ < 2)
		ssthresh_ = 2;
	if (how & (CLOSE_CWND_HALF|CLOSE_CWND_RESTART|CLOSE_CWND_INIT|CLOSE_CWND_ONE))
		cong_action_ = TRUE;

	fcnt_ = count_ = 0;
	if (first_decrease_ == 1)
		first_decrease_ = 0;
}



/*
 * Process a packet that acks previously unacknowleged data.
 */
void TcpAgent::newack(Packet* pkt)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	/* 
	 * Wouldn't it be better to set the timer *after*
	 * updating the RTT, instead of *before*? 
	 */
	if (!timerfix_) newtimer(pkt);
	dupacks_ = 0;
	last_ack_ = tcph->seqno();
	prev_highest_ack_ = highest_ack_ ;
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* 
	 * Update RTT only if it's OK to do so from info in the flags header.
	 * This is needed for protocols in which intermediate agents
	 * in the network intersperse acks (e.g., ack-reconstructors) for
	 * various reasons (without violating e2e semantics).
	 */	
	hdr_flags *fh = hdr_flags::access(pkt);
	if (!fh->no_ts_) {
		if (ts_option_)
			rtt_update(now - tcph->ts_echo());

		if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
			if (!ect_ || !ecn_backoff_ || 
				!hdr_flags::access(pkt)->ecnecho()) {
				/* 
				 * Don't end backoff if still in ECN-Echo with
			 	 * a congestion window of 1 packet. 
				 */
				t_backoff_ = 1;
				ecn_backoff_ = 0;
			}
			rtt_active_ = 0;
			if (!ts_option_)
				rtt_update(now - rtt_ts_);
		}
	}
	if (timerfix_) newtimer(pkt);
	/* update average window */
	awnd_ *= 1.0 - wnd_th_;
	awnd_ += wnd_th_ * cwnd_;
}


/*
 * Respond either to a source quench or to a congestion indication bit.
 * This is done at most once a roundtrip time;  after a source quench,
 * another one will not be done until the last packet transmitted before
 * the previous source quench has been ACKed.
 *
 * Note that this procedure is called before "highest_ack_" is
 * updated to reflect the current ACK packet.  
 */
void TcpAgent::ecn(int seqno)
{
	if (seqno > recover_ || 
	      last_cwnd_action_ == CWND_ACTION_TIMEOUT) {
		recover_ =  maxseq_;
		last_cwnd_action_ = CWND_ACTION_ECN;
		if (cwnd_ <= 1.0) {
			if (ecn_backoff_) 
				rtt_backoff();
			else ecn_backoff_ = 1;
		} else ecn_backoff_ = 0;
		slowdown(CLOSE_CWND_HALF|CLOSE_SSTHRESH_HALF);
	}
}

/*
 *  Is the connection limited by the network (instead of by a lack
 *    of data from the application?
 */
int TcpAgent::network_limited() {
	int win = window () ;
	if (t_seqno_ > (prev_highest_ack_ + win))
		return 1;
	else
		return 0;
}

void TcpAgent::recv_newack_helper(Packet *pkt) {
	//hdr_tcp *tcph = hdr_tcp::access(pkt);
	newack(pkt);
	if (!ect_ || !hdr_flags::access(pkt)->ecnecho() ||
		(old_ecn_ && ecn_burst_)) {
		/* If "old_ecn", this is not the first ACK carrying ECN-Echo
		 * after a period of ACKs without ECN-Echo.
		 * Therefore, open the congestion window. */
		/* if control option is set, and the sender is not
			 window limited, then do not increase the window size */
		
		if (!control_increase_ || 
		   (control_increase_ && (network_limited() == 1))) 
	      		opencwnd();
	}
	if (ect_) {
		if (!hdr_flags::access(pkt)->ecnecho())
			ecn_backoff_ = 0;
		if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = TRUE;
		else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = FALSE;
	}
	if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
		!hdr_flags::access(pkt)->cong_action())
		ect_ = 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
	if (QOption_ && curseq_ == highest_ack_ +1) {
		cancel_rtx_timer();
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
	// XXX what should we return here???
	fprintf(stderr, "Wrong number of wnd_init_option_ %d\n", 
		wnd_init_option_);
	abort();
	return (2.0); // XXX make msvc happy.
}

/*
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 *
 *	bugfix	ecn	last-cwnd == ecn	action
 *
 *	0	0	0			tahoe_action
 *	0	0	1			tahoe_action	[impossible]
 *	0	1	0			tahoe_action
 *	0	1	1			slow-start, return
 *	1	0	0			nothing
 *	1	0	1			nothing		[impossible]
 *	1	1	0			nothing
 *	1	1	1			slow-start, return
 */

/* 
 * A first or second duplicate acknowledgement has arrived, and
 * singledup_ is enabled.
 * If the receiver's advertised window permits, and we are exceeding our
 * congestion window by less than numdupacks_, then send a new packet.
 */
void
TcpAgent::send_one()
{
	if (t_seqno_ <= highest_ack_ + wnd_ && t_seqno_ < curseq_ &&
		t_seqno_ <= highest_ack_ + cwnd_ + dupacks_ ) {
		output(t_seqno_, 0);
		if (QOption_)
			process_qoption_after_send () ;
		t_seqno_ ++ ;
		// send_helper(); ??
	}
	return;
}

void
TcpAgent::dupack_action()
{
	int recovered = (highest_ack_ > recover_);
	if (recovered || (!bug_fix_ && !ecn_)) {
		goto tahoe_action;
	}

	if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		last_cwnd_action_ = CWND_ACTION_DUPACK;
		slowdown(CLOSE_CWND_ONE);
		reset_rtx_timer(0,0);
		return;
	}

	if (bug_fix_) {
		/*
		 * The line below, for "bug_fix_" true, avoids
		 * problems with multiple fast retransmits in one
		 * window of data. 
		 */
		return;
	}

tahoe_action:
	// we are now going to fast-retransmit and willtrace that event
	trace_event("FAST_RETX");

	recover_ = maxseq_;
	last_cwnd_action_ = CWND_ACTION_DUPACK;
	slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_ONE);
	reset_rtx_timer(0,0);
	return;
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
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
	int ecnecho = hdr_flags::access(pkt)->ecnecho();
	if (ecnecho && ecn_)
		ecn(tcph->seqno());
	recv_helper(pkt);
	/* grow cwnd and check if the connection is done */ 
	if (tcph->seqno() > last_ack_) {
		recv_newack_helper(pkt);
		if (last_ack_ == 0 && delay_growth_) { 
			cwnd_ = initial_window();
		}
	} else if (tcph->seqno() == last_ack_) {
                if (hdr_flags::access(pkt)->eln_ && eln_) {
                        tcp_eln(pkt);
                        return;
                }
		if (++dupacks_ == numdupacks_ && !noFastRetrans_) {
			dupack_action();
		} else if (dupacks_ < numdupacks_ && singledup_ ) {
			send_one();
		}
	}

	if (QOption_ && EnblRTTCtr_)
		process_qoption_after_ack (tcph->seqno());

	Packet::free(pkt);
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
void TcpAgent::timeout_nonrtx(int tno) 
{
	if (tno == TCP_TIMER_DELSND)  {
	 /*
	 	* delayed-send timer, with random overhead
	 	* to avoid phase effects
	 	*/
		send_much(1, TCP_REASON_TIMEOUT, maxburst_);
	}
}
	
void TcpAgent::timeout(int tno)
{
	/* retransmit timer */
	if (tno == TCP_TIMER_RTX) {

		// There has been a timeout - will trace this event
		trace_event("TCP_TIMEOUT");

	        if (cwnd_ < 1) cwnd_ = 1;
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.  
			 */
			 // Should this return be here?
			 // What if CWND_ACTION_ECN and cwnd < 1?
			 // return;
		} else {
			recover_ = maxseq_;
			if (highest_ack_ == -1 && wnd_init_option_ == 2)
				/* 
				 * First packet dropped, so don't use larger
				 * initial windows. 
				 */
				wnd_init_option_ = 1;
			if (highest_ack_ == maxseq_ && restart_bugfix_)
			       /* 
				* if there is no outstanding data, don't cut 
				* down ssthresh_.
				*/
				slowdown(CLOSE_CWND_ONE);
			else if (highest_ack_ < recover_ &&
			  last_cwnd_action_ == CWND_ACTION_ECN) {
			       /*
				* if we are in recovery from a recent ECN,
				* don't cut down ssthresh_.
				*/
				slowdown(CLOSE_CWND_ONE);
			}
			else {
				++nrexmit_;
				last_cwnd_action_ = CWND_ACTION_TIMEOUT;
				slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
			}
		}
		/* if there is no outstanding data, don't back off rtx timer */
		if (highest_ack_ == maxseq_ && restart_bugfix_) {
			reset_rtx_timer(0,0);
		}
		else {
			reset_rtx_timer(0,1);
		}
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
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
        //int eln_rxmit;
        hdr_tcp *tcph = hdr_tcp::access(pkt);
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

/*
 * THE FOLLOWING FUNCTIONS ARE OBSOLETE, but REMAIN HERE
 * DUE TO OTHER PEOPLE's TCPs THAT MIGHT USE THEM
 *
 * These functions are now replaced by ecn() and slowdown(),
 * respectively.
 */

/*
 * Respond either to a source quench or to a congestion indication bit.
 * This is done at most once a roundtrip time;  after a source quench,
 * another one will not be done until the last packet transmitted before
 * the previous source quench has been ACKed.
 */
void TcpAgent::quench(int how)
{
	if (highest_ack_ >= recover_) {
		recover_ =  maxseq_;
		last_cwnd_action_ = CWND_ACTION_ECN;
		closecwnd(how);
	}
}

/*
 * close down the congestion window
 */
void TcpAgent::closecwnd(int how)
{   
	static int first_time = 1;
	if (first_time == 1) {
		fprintf(stderr, "the TcpAgent::closecwnd() function is now deprecated, please use the function slowdown() instead\n");
	}
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
		// cwnd_ = window()/2;
		cwnd_ = decrease_num_ * window();
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
 * Check if the sender has been idle or application-limited for more
 * than an RTO, and if so, reduce the congestion window.
 */
void TcpAgent::process_qoption_after_send ()
{
	int tcp_now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
	int rto = (int)(t_rtxcur_/tcp_tick_) ; 
	/*double ct = Scheduler::instance().clock();*/

	if (!EnblRTTCtr_) {
		if (tcp_now - T_last >= rto) {
			// The sender has been idle.
		 	slowdown(THREE_QUARTER_SSTHRESH) ;
			for (int i = 0 ; i < (tcp_now - T_last)/rto; i ++) {
				slowdown(CWND_HALF_WITH_MIN);
			}
			T_prev = tcp_now ;
			W_used = 0 ;
		}
		T_last = tcp_now ;
		if (t_seqno_ == highest_ack_+ window()) {
			T_prev = tcp_now ; 
			W_used = 0 ; 
		}
		else if (t_seqno_ == curseq_-1) {
			// The sender has no more data to send.
			int tmp = t_seqno_ - highest_ack_ ;
			if (tmp > W_used)
				W_used = tmp ;
			if (tcp_now - T_prev >= rto) {
				// The sender has been application-limited.
				slowdown(THREE_QUARTER_SSTHRESH);
				slowdown(CLOSE_CWND_HALF_WAY);
				T_prev = tcp_now ;
				W_used = 0 ;
			}
		}
	} else {
		rtt_counting();
	}
}

/*
 * Check if the sender has been idle or application-limited for more
 * than an RTO, and if so, reduce the congestion window, for a TCP sender
 * that "counts RTTs" by estimating the number of RTTs that fit into
 * a single clock tick.
 */
void
TcpAgent::rtt_counting()
{
        int tcp_now = (int)(Scheduler::instance().clock()/tcp_tick_ + 0.5);
	int rtt = (int(t_srtt_) >> T_SRTT_BITS) ;

	if (rtt < 1) 
		rtt = 1 ;
	if (tcp_now - T_last >= 2*rtt) {
		// The sender has been idle.
		int RTTs ; 
		RTTs = (tcp_now -T_last)*RTT_goodcount/(rtt*2) ; 
		RTTs = RTTs - Backoffs ; 
		Backoffs = 0 ; 
		if (RTTs > 0) {
			slowdown(THREE_QUARTER_SSTHRESH) ;
			for (int i = 0 ; i < RTTs ; i ++) {
				slowdown(CWND_HALF_WITH_MIN);
				RTT_prev = RTT_count ; 
				W_used = 0 ;
			}
		}
	}
	T_last = tcp_now ;
	if (tcp_now - T_start >= 2*rtt) {
		if ((RTT_count > RTT_goodcount) || (F_full == 1)) {
			RTT_goodcount = RTT_count ; 
			if (RTT_goodcount < 1) RTT_goodcount = 1 ; 
		}
		RTT_prev = RTT_prev - RTT_count ;
		RTT_count = 0 ; 
		T_start  = tcp_now ;
		F_full = 0;
	}
	if (t_seqno_ == highest_ack_ + window()) {
		W_used = 0 ; 
		F_full = 1 ; 
		RTT_prev = RTT_count ;
	}
	else if (t_seqno_ == curseq_-1) {
		// The sender has no more data to send.
		int tmp = t_seqno_ - highest_ack_ ;
		if (tmp > W_used)
			W_used = tmp ;
		if (RTT_count - RTT_prev >= 2) {
			// The sender has been application-limited.
			slowdown(THREE_QUARTER_SSTHRESH) ;
			slowdown(CLOSE_CWND_HALF_WAY);
			RTT_prev = RTT_count ; 
			Backoffs ++ ; 
			W_used = 0;
		}
	}
	if (F_counting == 0) {
		W_timed = t_seqno_  ;
		F_counting = 1 ;
	}
}

void TcpAgent::process_qoption_after_ack (int seqno)
{
	if (F_counting == 1) {
		if (seqno >= W_timed) {
			RTT_count ++ ; 
			F_counting = 0 ; 
		}
		else {
			if (dupacks_ == numdupacks_)
				RTT_count ++ ;
		}
	}
}

void TcpAgent::trace_event(char *eventtype)
{
	if (et_ == NULL) return;
	
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();
	if (wrk != 0)
		sprintf(wrk,
			"E -t "TIME_FORMAT" -o TCP -e %s -s %d.%d -d %d.%d",
			et_->round(Scheduler::instance().clock()),   // time
			eventtype,                    // event type
			addr(),                       // owner (src) node id
			port(),                       // owner (src) port id
			daddr(),                      // dst node id
			dport()                       // dst port id
			);
	
	if (nwrk != 0)
		sprintf(nwrk,
			"E -t "TIME_FORMAT" -o TCP -e %s -s %d.%d -d %d.%d",
			et_->round(Scheduler::instance().clock()),   // time
			eventtype,                    // event type
			addr(),                       // owner (src) node id
			port(),                       // owner (src) port id
			daddr(),                      // dst node id
			dport()                       // dst port id
			);
	et_->trace();
}
