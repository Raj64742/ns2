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
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-reno.cc,v 1.8.2.4 1997/04/29 06:25:39 padmanab Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

int RenoTcpAgent::window()
{
	//
	// reno: inflate the window by dupwnd_
	//	dupwnd_ will be non-zero during fast recovery,
	//	at which time it contains the number of dup acks
	//
	int win = int(cwnd()) + dupwnd_;
	if (win > int(wnd_))
		win = int(wnd_);
	return (win);
}

RenoTcpAgent::RenoTcpAgent() : TcpAgent(), dupwnd_(0)
{
}

void RenoTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		fprintf(stderr,
			"ns: confiuration error: tcp received non-ack\n");
		exit(1);
	}
#endif
	ts_peer_ = tcph->ts();

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
	recv_helper(pkt);
	if (tcph->seqno() > last_ack_) {
		dupwnd_ = 0;
		recv_newack_helper(pkt);
   	} else if (tcph->seqno() == last_ack_)  {
		if (++dupacks() == NUMDUPACKS) {
			/*
			 * Assume we dropped just one packet.
			 * Retransmit last ack + 1
			 * and try to resume the sequence.
			 */
                       /* The line below, for "bug_fix_" true, avoids
                        * problems with multiple fast retransmits after
			* a retransmit timeout.
                        */
			if ( !bug_fix_ || (highest_ack() > recover_) ||
			    ( recover_cause_ != 2)) {
				recover_cause_ = 1;
				recover_ = maxseq();
				closecwnd(1);
				reset_rtx_timer(1);
				output(last_ack_ + 1, TCP_REASON_DUPACK);
                        }
			dupwnd_ = NUMDUPACKS;
		} else if (dupacks() > NUMDUPACKS)
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

	if (dupacks() == 0 || dupacks() > NUMDUPACKS - 1)
		send(0, 0, maxburst_);
}

void RenoTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupwnd_ = 0;
		dupacks() = 0;
		if (bug_fix_) recover_ = maxseq();
		TcpAgent::timeout(tno);
	} else {
		send(1, TCP_REASON_TIMEOUT, maxburst_);
	}
}
