/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef lint
static char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-reno.cc,v 1.2 1997/01/24 17:53:00 mccanne Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "tcp.h"

class RenoTcpAgent : public TcpAgent {
 public:
	RenoTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt);
	virtual void timeout(int tno);
 protected:
	u_int dupwnd_;
};

static class RenoTcpClass : public TclClass {
public:
	RenoTcpClass() : TclClass("agent/tcp/reno") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RenoTcpAgent());
	}
} class_reno;

int RenoTcpAgent::window()
{
	//
	// reno: inflate the window by dupwnd_
	//	dupwnd_ will be non-zero during fast recovery,
	//	at which time it contains the number of dup acks
	//
	int win = int(cwnd_) + dupwnd_;
	if (win > int(wnd_))
		win = int(wnd_);
	return (win);
}

RenoTcpAgent::RenoTcpAgent() : dupwnd_(0)
{
}

void RenoTcpAgent::recv(Packet *pkt)
{
	if (pkt->type_ != PT_ACK) {
		fprintf(stderr,
			"ns: confiuration error: tcp received non-ack\n");
		exit(1);
	}

	if (pkt->flags_ & PF_ECN)
		quench(1);
	if (pkt->seqno_ > last_ack_) {
		dupwnd_ = 0;
		newack(pkt);
                opencwnd();
   	} else if (pkt->seqno_ == last_ack_)  {
		if (++dupacks_ == NUMDUPACKS) {
			/*
			 * Assume we dropped just one packet.
			 * Retransmit last ack + 1
			 * and try to resume the sequence.
			 */
                       /* The line below, for "bug_fix_" true, avoids
                        * problems with multiple fast retransmits after
			* a retransmit timeout.
                        */
			if ( !bug_fix_ || (highest_ack_ > recover_) ||
			    ( recover_cause_ != 2)) {
				recover_cause_ = 1;
				recover_ = maxseq_;
				closecwnd(1);
				reset_rtx_timer(1);
				output(last_ack_ + 1, PF_DUPACK);
                        }
			dupwnd_ = NUMDUPACKS;
		} else if (dupacks_ > NUMDUPACKS)
			++dupwnd_;
	}
	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif

	/*
	 * Try to send more data
	 */

	if (dupacks_ == 0 || dupacks_ > NUMDUPACKS - 1)
		send(0, 0, maxburst_);
}

void RenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupwnd_ = 0;
		dupacks_ = 0;
		if (bug_fix_) recover_ = maxseq_;
		TcpAgent::timeout(tno);
	}
}
