/*
 * Copyright (c) 1990, 1997 Regents of the University of California.
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
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-newreno.cc,v 1.7 1997/05/21 21:42:41 tomh Exp $ (LBL)";
#endif

//
// newreno-tcp: a revised reno TCP source, without sack
//
// first cut, Nov 1995; not yet debugged
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"

class NewRenoTcpAgent : public TcpAgent {
 public:
	NewRenoTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt);
	virtual void timeout(int tno);
 protected:
	u_int dupwnd_;
	int newreno_changes_;	/* 0 for fixing unnecessary fast retransmits */
					/* 1 for additional code from Allman, */
					/* to implement other algorithms from */
					/* Hoe's paper */
	void partialnewack(Packet *pkt);
};

static class NewRenoTcpClass : public TclClass {
public:
	NewRenoTcpClass() : TclClass("Agent/TCP/Newreno") {}
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

NewRenoTcpAgent::NewRenoTcpAgent() : dupwnd_(0), newreno_changes_(0)
{
	bind("newreno_changes_", &newreno_changes_);
}

/* 
 * Process a packet that acks previously unacknowleges data, but
 * does not take us out of Fast Retransmit.
 */
void NewRenoTcpAgent::partialnewack(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
        newtimer(pkt);
#ifdef notyet
        if (pkt->seqno_ == stp->maxpkts && stp->maxpkts > 0)
                stp->endtime = (float) realtime();
#endif
        last_ack_ = tcph->seqno();
        highest_ack_ = last_ack_;
        if (t_seqno_ < last_ack_ + 1)
                t_seqno_ = last_ack_ + 1;
        if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
                rtt_active_ = 0;
                t_backoff_ = 1;
        }
}

void NewRenoTcpAgent::recv(Packet *pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);

	static int acked = 0;
	int new_ssthresh;
	static double ack2, ack3, basertt;

	/* Use first packet to calculate the RTT  --contributed by Allman */

	if (++acked == 1) 
		basertt = Scheduler::instance().clock() - firstsent_;

	/* Estimate ssthresh based on the calculated RTT and the estimated
		bandwidth (using ACKs 2 and 3).  */

	else if (acked == 2)
		ack2 = Scheduler::instance().clock();
	else if (acked == 3) {
		ack3 = Scheduler::instance().clock();
		new_ssthresh = int((basertt * (size_ / (ack3 - ack2))) / size_);
		if (newreno_changes_ > 0 && new_ssthresh < ssthresh_)
			ssthresh_ = new_ssthresh;
	}

#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		fprintf(stderr,
			"ns: confiuration error: tcp received non-ack\n");
		exit(1);
	}
#endif

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
	if (tcph->seqno() > last_ack_) {
	    if (tcph->seqno() >= recover_) {
		dupwnd_ = 0;
		newack(pkt);
                opencwnd();
	    } else {
		/* received new ack for a packet sent during Fast
		 *  Recovery, but sender stays in Fast Recovery */
		dupwnd_ = 0;
		partialnewack(pkt);
		output(last_ack_ + 1, 0);
	    }
   	} else if (tcph->seqno() == last_ack_)  {
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
				output(last_ack_ + 1, TCP_REASON_DUPACK);
                        }
			dupwnd_ = NUMDUPACKS;
		} else if (dupacks_ > NUMDUPACKS) {
			++dupwnd_;
			/* For every two duplicate ACKs we receive (in the
			 * "fast retransmit phase"), send one entirely new
			 * data packet "to keep the flywheel going".  --Allman
			 */
			if (newreno_changes_ > 0 && (dupacks_ % 2) == 1)
				output (t_seqno_++,0);

		}
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
	else if (dupacks_ > NUMDUPACKS - 1 && newreno_changes_ == 0)
		send(0, 0, 2);
}

void NewRenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupwnd_ = 0;
		dupacks_ = 0;
		if (bug_fix_) recover_ = maxseq_;
		TcpAgent::timeout(tno);
	} else {
		send(1, TCP_REASON_TIMEOUT);
	}
}
