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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-simple.cc,v 1.4 1997/04/18 00:55:26 bajaj Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include "Tcl.h"
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "random.h"

class TcpSimpleAgent : public Agent {
public:
	TcpSimpleAgent();
	virtual void recv(Packet*, Handler*);
	int command(int argc, const char*const* argv);
protected:
	virtual int window();

	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	int t_seqno_;		/* sequence number */

	/*XXX start/stop */
	void output(int seqno);
	virtual void send();

	void opencwnd();
	void closecwnd();

	void reset();
	void newack(Packet* pkt);

	double wnd_;
	/*
	 * Dynamic state.
	 */
	int curseq_;		/* highest seqno "produced by app" */
	int ack_;	/* not frozen during Fast Recovery */
	int dupacks_;
	double cwnd_;		/* current window */
	int maxseq_;		/* used for Karn algorithm */
				/* highest seqno sent so far */

	int off_tcp_;
};

static class TcpSimpleClass : public TclClass {
public:
	TcpSimpleClass() : TclClass("Agent/TCPSimple") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpSimpleAgent());
	}
} class_tcpsimple;

TcpSimpleAgent::TcpSimpleAgent() : Agent(PT_TCP)
{
	bind("window_", &wnd_);
	bind("packetSize_", &size_);

	bind("off_tcp_", &off_tcp_);

	bind("seqno_", &curseq_);
	bind("ack_", &ack_);
	bind("t_seqno_", &t_seqno_);
	bind("cwnd_", &cwnd_);
	bind("dupacks_", &dupacks_);
}

void TcpSimpleAgent::reset()
{
	/*XXX lookup variables */
	curseq_ = 0;
	cwnd_ = 1;
	t_seqno_ = 0;
	maxseq_ = -1;
	ack_ = -1;
	dupacks_ = 0;
}

void TcpSimpleAgent::output(int seqno)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	Connector::send(p, 0);
	if (seqno > maxseq_) {
		maxseq_ = seqno;
	}
}

int TcpSimpleAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "send") == 0) {
			send();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			curseq_ += atoi(argv[2]);
			send();
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

int TcpSimpleAgent::window()
{
	return (int(cwnd_ < wnd_ ? cwnd_ : wnd_));
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void TcpSimpleAgent::send()
{
	int win = window();
	while (t_seqno_ <= ack_ + win && t_seqno_ < curseq_)
		output(t_seqno_++);
}

/*
 * Process a packet that acks previously unacknowleges data.
 */
void TcpSimpleAgent::newack(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	int ack = tcph->seqno();
	if (ack_ < ack)
		ack_ = ack;
	if (t_seqno_ < ack + 1)
		t_seqno_ = ack + 1;
}

/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void TcpSimpleAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
	if (tcph->seqno() == ack_)
		++dupacks_;
	else if (tcph->seqno() > ack_)
		newack(pkt);

	Packet::free(pkt);
	/*
	 * Try to send more data.
	 */
	send();
}
