/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
/*
 * prune.cc
 * Copyright (C) 1997 by USC/ISI
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/prune.cc,v 1.5 1998/06/26 02:20:24 gnguyen Exp $ (LBL)";
#endif

#include "agent.h"
#include "random.h"
#include "prune.h"
#include "ip.h"

class PruneAgent : public Agent {
public:
	PruneAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);
protected:
	int off_prune_;
};


static class PruneHeaderClass : public PacketHeaderClass {
public:
	PruneHeaderClass() : PacketHeaderClass("PacketHeader/Prune",
					       sizeof(hdr_prune)) {}
} class_prunehdr;

static class PruneClass : public TclClass {
public:
	PruneClass() : TclClass("Agent/Mcast/Prune") {}
	TclObject* create(int, const char*const*) {
		return (new PruneAgent());
	}
} class_prune;


PruneAgent::PruneAgent() : Agent(PT_GRAFT)
{
	bind("packetSize_", &size_);
	bind("off_prune_", &off_prune_);
}

void PruneAgent::recv(Packet* pkt, Handler*)
{
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
