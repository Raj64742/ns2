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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"


class WWWTcpAgent : public TcpAgent {
 public:
	WWWTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
};

static class WWWTcpClass : public TclClass {
public:
	WWWTcpClass() : TclClass("Agent/TCP/WWW") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new WWWTcpAgent());
	}
} class_www;

WWWTcpAgent::WWWTcpAgent() 
{
}


/*
 * main reception path - should only see acks, otherwise the
 * network connections are misconfigured
 */
void WWWTcpAgent::recv(Packet *pkt, Handler*)
{
    char wrk[128];/*XXX*/
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip *iph = (hdr_ip*)pkt->access(off_ip_);
#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif

	/*XXX only if ecn_ true*/
	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
/*	if (tcph->seqno() == curseq_ - 1){
		sprintf(wrk, "%s finish", name());
		Tcl::instance().eval(wrk);
	}
*/	if (tcph->seqno() > last_ack_) {
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
	if (tcph->seqno() == curseq_ - 1){
		sprintf(wrk, "%s finish", name());
		Tcl::instance().eval(wrk);
	}
	/*
	 * Try to send more data.
	 */
	send(0, 0, maxburst_);
}

