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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/prune.cc,v 1.1 1997/06/28 03:25:59 polly Exp $ (LBL)";
#endif

#include "agent.h"
#include "Tcl.h"
#include "packet.h"
#include "random.h"
#include "prune.h"
#include "ip.h"

static class PruneHeaderClass : public PacketHeaderClass {
public:
        PruneHeaderClass() : PacketHeaderClass("PacketHeader/Prune",
					     sizeof(hdr_prune)) {}
} class_prunehdr;

class PruneAgent : public Agent {
 public:
	PruneAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);
protected:
	int off_prune_;
};

static class PruneClass : public TclClass {
public:
	PruneClass() : TclClass("Agent/Mcast/Prune") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new PruneAgent());
	}
} class_prune;

PruneAgent::PruneAgent() : Agent(PT_GRAFT)
{
	Tcl& tcl = Tcl::instance();
	bind("packetSize_", &size_);
	bind("off_prune_", &off_prune_);
}

void PruneAgent::recv(Packet* pkt, Handler*)
{
        hdr_ip* ih = (hdr_ip*)pkt->access(off_ip_);
	hdr_prune* ph = (hdr_prune*)pkt->access(off_prune_);
	Tcl::instance().evalf("%s handle %s %d %d %d", name(), ph->type(), ph->from(), ph->src(), ph->group());
	Packet::free(pkt);
}

/*
 * $proc handler $handler
 * $proc send $type $src $group
 */
int PruneAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 6) {
		if (strcmp(argv[1], "send") == 0) {
			Packet* pkt = allocpkt();
			hdr_prune* ph = (hdr_prune*)pkt->access(off_prune_);
			const char* s = argv[2];
			int n = strlen(s);
			if (n >= ph->maxtype()) {
				tcl.result("message type too big");
				Packet::free(pkt);
				return (TCL_ERROR);
			}
			strcpy(ph->type(), s);
			ph->from() = atoi(argv[3]);
			ph->src() = atoi(argv[4]);
			ph->group() = atoi(argv[5]);
			send(pkt, 0);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
