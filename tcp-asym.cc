/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * tcp-asym.cc: contributed by the Daedalus Research Group, U.C.Berkeley 
 * http://daedalus.cs.berkeley.edu
 */

/*
 * tcp-asym includes modifications to several flavors of TCP to enhance
 * performance over asymmetric networks, where the ack channel is
 * constrained.  Types of asymmetry we have studied and used these mods
 * include bandwidth asymmetry and latency asymmetry (where  variable 
 * latencies cause problems to TCP, e.g., in packet radio networks.
 * The sender-side modifications in this file are structured as helper 
 * functions that are invoked from various points in the TCP code. The main
 * additional functionality is (a) the sender increases its congestion window
 * in proportion to the amount of data acked rather than the number of acks
 * received, (b) it breaks up potentially large bursts into smaller ones 
 * when acks are few and far between by rate-based pacing, and 
 * (c) it copies the ecn_to_echo_ bit from acks into subsequent data packets, 
 * using which the sink can perform ack congestion control (tcp-asym-sink.cc). 
 * The tcp-asym source is usually used in conjunction with a tcp-asym sink, or
 * with a router/end-host implementing ack filtering (semantic-packetqueue.cc).
 */ 

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-asym.cc,v 1.9 1997/11/27 05:28:27 padmanab Exp $ (UCB)";
#endif


#include "tcp.h"
#include "ip.h"
#include "flags.h"
#include "random.h"
#include "template.h"

/* TCP Asym with Tahoe */
class TcpAsymAgent : public virtual TcpAgent {
public:
	TcpAsymAgent();

	/* helper functions */
	virtual void output_helper(Packet* p);
	virtual void send_helper(int maxburst);
	virtual void recv_helper(Packet* p);
	virtual void recv_newack_helper(Packet* pkt);
	virtual void traceAll();
	virtual void traceVar(TracedVar* v);
protected:
	int off_tcpasym_;
	int ecn_to_echo_;
	TracedDouble t_exact_srtt_;
/*	TracedDouble avg_win_; */
	double g_;		/* gain used for exact_srtt_ smoothing */
	int damp_;
};

static class TCPAHeaderClass : public PacketHeaderClass {
public:
        TCPAHeaderClass() : PacketHeaderClass("PacketHeader/TCPA",
					     sizeof(hdr_tcpasym)) {}
} class_tcpahdr;

static class TcpAsymClass : public TclClass {
public:
	TcpAsymClass() : TclClass("Agent/TCP/Asym") {}
	TclObject* create(int, const char*const*) {
		return (new TcpAsymAgent());
	} 
} class_tcpasym;

TcpAsymAgent::TcpAsymAgent() : TcpAgent(), ecn_to_echo_(0), t_exact_srtt_(0)
{
	bind("exact_srtt_", &t_exact_srtt_);
	bind("off_tcpasym_", &off_tcpasym_);
	bind("g_", &g_);
/*	bind("avg_win_", &avg_win_);*/
	bind("damp_", &damp_);
}


/* TCP Asym with Reno */
class TcpRenoAsymAgent : public RenoTcpAgent,
			 public TcpAsymAgent {
public:
	 TcpRenoAsymAgent() : RenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

static class TcpRenoAsymClass : public TclClass {
public:
	TcpRenoAsymClass() : TclClass("Agent/TCP/Reno/Asym") {}
	TclObject* create(int, const char*const*) {
		return (new TcpRenoAsymAgent());
	} 
} class_tcprenoasym;


/* TCP Asym with New Reno */
class TcpNewRenoAsymAgent : public NewRenoTcpAgent,
			    public TcpAsymAgent {
public:
	 TcpNewRenoAsymAgent() : NewRenoTcpAgent(), TcpAsymAgent() { }

	/* helper functions */
	virtual void output_helper(Packet* p) { TcpAsymAgent::output_helper(p); }
	virtual void send_helper(int maxburst) { TcpAsymAgent::send_helper(maxburst); }
	virtual void recv_helper(Packet* p) { TcpAsymAgent::recv_helper(p); }
	virtual void recv_newack_helper(Packet* pkt) { TcpAsymAgent::recv_newack_helper(pkt); }

};

static class TcpNewRenoAsymClass : public TclClass {
public:
	TcpNewRenoAsymClass() : TclClass("Agent/TCP/Newreno/Asym") {}
	TclObject* create(int, const char*const*) {
		return (new TcpNewRenoAsymAgent());
	} 
} class_tcpnewrenoasym;


/* The helper functions */

/* fill in the TCP asym header before packet is sent out */
void TcpAsymAgent::output_helper(Packet* p) 
{
	hdr_tcpasym *tcpha = (hdr_tcpasym*)p->access(off_tcpasym_);
	hdr_flags *flagsh = (hdr_flags*)p->access(off_flags_);

	tcpha->win() = min(highest_ack_+window(), curseq_) - t_seqno_;
	tcpha->highest_ack() = highest_ack_;
	tcpha->max_left_to_send() = curseq_ - highest_ack_; /* XXXX not needed? */

	flagsh->ecn_ = ecn_to_echo_;
	ecn_to_echo_ = 0;
}

/* schedule the next burst of data (of size at most maxburst) */
void TcpAsymAgent::send_helper(int maxburst) 
{
	/* 
	 * If there is still data to be sent and there is space in the
	 * window, set a timer to schedule the next burst. Note that
	 * we wouldn't get here if TCP_TIMER_BURSTSEND were pending,
	 * so we do not need an explicit check here.
	 */
	if (t_seqno_ <= highest_ack_ + window() && t_seqno_ < curseq_) {
		double xmit_freq = t_exact_srtt_*maxburst/window();
/*		double xmit_freq = t_exact_srtt_*maxburst/avg_win_;*/

		if (damp_)
			xmit_freq /= 2;
		burstsnd_timer_.resched(xmit_freq);
	}
}

/* check if the received ack has an ECN that needs to be echoed back to the sink */
void TcpAsymAgent::recv_helper(Packet *pkt) 
{
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_to_echo_)
		ecn_to_echo_ = 1;
}

/* open cwnd in proportion to the amount of data acked */
void TcpAsymAgent::recv_newack_helper(Packet *pkt) 
{
	int i;
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	int ackcount = tcph->seqno() - last_ack_;
	double tao = Scheduler::instance().clock() - tcph->ts_echo();

	newack(pkt);
	/* update our fine-grained estimate of the smoothed RTT */
	if (t_exact_srtt_ != 0) 
		t_exact_srtt_ = g_*tao + (1-g_)*t_exact_srtt_;
	else
		t_exact_srtt_ = tao;
/*	avg_win_ = g_*window() + (1-g_)*avg_win_;*/
	/* grow cwnd */
	for (i=0; i<ackcount; i++)
		opencwnd();
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}	


/* Print out if tcp-asym-specific variables have been modified */
void TcpAsymAgent::traceVar(TracedVar* v) {
	Scheduler& s = Scheduler::instance();
	char wrk[500];

	double curtime = &s ? s.clock() : 0;
	if (!strcmp(v->name(), "exact_srtt_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", 
			curtime, addr_/256, addr_%256, dst_/256, dst_%256, 
			v->name(), double(*((TracedDouble*) v)));
	else if (!strcmp(v->name(), "avg_win_"))
		sprintf(wrk,"%-8.5f %-2d %-2d %-2d %-2d %s %-6.3f", 
			curtime, addr_/256, addr_%256, dst_/256, dst_%256, 
			v->name(), double(*((TracedDouble*) v)));
	else {
		TcpAgent::traceVar(v);
		return;
	}

	int n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;
	return;
}

/* Print out all the traced variables whenever any one is changed */
void
TcpAsymAgent::traceAll() {
	double curtime;
	Scheduler& s = Scheduler::instance();
	char wrk[500];
	int n;

	TcpAgent::traceAll();
	curtime = &s ? s.clock() : 0;
	sprintf(wrk, "time: %-8.5f saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d exact_srtt %-6.3f", curtime, addr_/256, addr_%256, dst_/256, dst_%256, 
		(int(t_exact_srtt_)));
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	wrk[n] = 0;
	return;
}
