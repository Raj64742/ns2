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
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-newreno.cc,v 1.1 1996/12/19 03:22:45 mccanne Exp $ (LBL)";
#endif

//
// newreno-tcp: a revised reno TCP source, without sack
//
// first cut, Nov 1995; not yet debugged
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "tcp.h"

class NewRenoTcpAgent : public TcpAgent {
 public:
	NewRenoTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt);
	virtual void timeout(int tno);
 protected:
	u_int dupwnd_;
	void partialnewack(Packet *pkt);
};

static class NewRenoTcpClass : public TclClass {
public:
	NewRenoTcpClass() : TclClass("agent/tcp-newreno") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NewRenoTcpAgent());
	}
} matcher_reno;

int NewRenoTcpAgent::window()
{
        //
        // reno: inflate the window by dupwnd_
        //      dupwnd_ will be non-zero during fast recovery,
        //      at which time it contains the number of dup acks
        //
	int win = int(cwnd_) + dupwnd_;
        if (win > int(wnd_))
                win = int(wnd_);
        return (win);
}

NewRenoTcpAgent::NewRenoTcpAgent() : dupwnd_(0)
{
}

/* 
 * Process a packet that acks previously unacknowleges data, but
 * does not take us out of Fast Retransmit.
 */
void NewRenoTcpAgent::partialnewack(Packet* pkt)
{
        newtimer(pkt);
#ifdef notyet
        if (pkt->seqno_ == stp->maxpkts && stp->maxpkts > 0)
                stp->endtime = (float) realtime();
#endif
        last_ack_ = pkt->seqno_;
        highest_ack_ = last_ack_;
        if (seqno_ < last_ack_ + 1)
                seqno_ = last_ack_ + 1;
        if (rtt_active_ && pkt->seqno_ >= rtt_seq_) {
                rtt_active_ = 0;
                t_backoff_ = 1;
        }
}

void NewRenoTcpAgent::recv(Packet *pkt)
{
	if (pkt->type_ != PT_ACK) {
		fprintf(stderr,
			"ns: confiuration error: tcp received non-ack\n");
		exit(1);
	}

	if (pkt->flags_ & PF_ECN)
		quench(1);
	if (pkt->seqno_ > last_ack_) {
	    if (pkt->seqno_ >= recover_) {
		dupwnd_ = 0;
		newack(pkt);
                opencwnd();
	    }
	    else {
		/* received new ack for a packet sent during Fast
		 *  Recovery, but sender stays in Fast Recovery */
		dupwnd_ = 0;
		partialnewack(pkt);
		output(last_ack_ + 1, 0);
	    }
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
			if ( (highest_ack_ > recover_) ||
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

	if (dupacks_ == 0) 
                /*
                 * Maxburst is really only needed for the first
                 *   window of data on exiting Fast Recovery.
                 */
		send(0, 0, maxburst_);
	else if (dupacks_ > NUMDUPACKS - 1)
		send(0, 0, 2);
}

void NewRenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupwnd_ = 0;
		dupacks_ = 0;
		if (bug_fix_) recover_ = maxseq_;
		TcpAgent::timeout(tno);
	}
}
