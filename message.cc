/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
/*
 * Copyright (c) 1994-1997 Regents of the University of California.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/message.cc,v 1.11 1998/06/26 02:20:20 gnguyen Exp $ (LBL)";
#endif

#include "agent.h"
#include "random.h"
#include "message.h"

static class MessageHeaderClass : public PacketHeaderClass {
public:
	MessageHeaderClass() : PacketHeaderClass("PacketHeader/Message",
						 sizeof(hdr_msg)) {}
} class_msghdr;

class MessageAgent : public Agent {
 public:
	MessageAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);
protected:
	int off_msg_;
};

static class MessageClass : public TclClass {
public:
	MessageClass() : TclClass("Agent/Message") {}
	TclObject* create(int, const char*const*) {
		return (new MessageAgent());
	}
} class_message;

MessageAgent::MessageAgent() : Agent(PT_MESSAGE)
{
	bind("packetSize_", &size_);
	bind("off_msg_", &off_msg_);
}

void MessageAgent::recv(Packet* pkt, Handler*)
{
	hdr_msg* mh = (hdr_msg*)pkt->access(off_msg_);
	char wrk[128];/*XXX*/
	sprintf(wrk, "%s recv {%s}", name(), mh->msg());

	Tcl& tcl = Tcl::instance();
	tcl.eval(wrk);

	Packet::free(pkt);
}

/*
 * $proc handler $handler
 * $proc send $msg
 */
int MessageAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "send") == 0) {
			Packet* pkt = allocpkt();
			hdr_msg* mh = (hdr_msg*)pkt->access(off_msg_);
			const char* s = argv[2];
			int n = strlen(s);
			if (n >= mh->maxmsg()) {
				tcl.result("message too big");
				Packet::free(pkt);
				return (TCL_ERROR);
			}
			strcpy(mh->msg(), s);
			send(pkt, 0);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
