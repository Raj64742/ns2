/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * ctrMcast.cc
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
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mcast/ctrMcast.cc,v 1.9 2000/09/01 03:04:05 haoboy Exp $ (USC/ISI)";
#endif

#include "agent.h"
#include "ip.h"
#include "ctrMcast.h"

int hdr_CtrMcast::offset_;

static class CtrMcastHeaderClass : public PacketHeaderClass {
public:
	CtrMcastHeaderClass() : PacketHeaderClass("PacketHeader/CtrMcast",
						  sizeof(hdr_CtrMcast)) {
		bind_offset(&hdr_CtrMcast::offset_);
	} 
} class_CtrMcast_hdr;


class CtrMcastEncap : public Agent {
public:
	CtrMcastEncap() : Agent(PT_CtrMcast_Encap) {}
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler*);
};

class CtrMcastDecap : public Agent {
public:
	CtrMcastDecap() : Agent(PT_CtrMcast_Decap) {}
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler*);
};

static class CtrMcastEncapclass : public TclClass {
public:
	CtrMcastEncapclass() : TclClass("Agent/CtrMcast/Encap") {}
	TclObject* create(int, const char*const*) {
		return (new CtrMcastEncap);
	}
} class_CtrMcastEncap;

static class CtrMcastDecapclass : public TclClass {
public:
	CtrMcastDecapclass() : TclClass("Agent/CtrMcast/Decap") {}
	TclObject* create(int, const char*const*) {
		return (new CtrMcastDecap);
	}
} class_CtrMcastDecap;


int CtrMcastEncap::command(int argc, const char*const* argv)
{
	return Agent::command(argc, argv);
}

int CtrMcastDecap::command(int argc, const char*const* argv)
{
	return Agent::command(argc, argv);
}

void CtrMcastEncap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = hdr_CtrMcast::access(p);
	hdr_ip* ih = hdr_ip::access(p);

	ch->src() = ih->saddr();
	ch->group() = ih->daddr();
	ch->flowid() = ih->flowid();
	ih->saddr() = addr();
	ih->sport() = port();
	ih->daddr() = daddr();
	ih->dport() = dport();
	ih->flowid() = fid_;

	target_->recv(p);
}

void CtrMcastDecap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = hdr_CtrMcast::access(p);
	hdr_ip* ih = hdr_ip::access(p);

	ih->saddr() = ch->src();
	ih->daddr() = ch->group();
	ih->flowid() = ch->flowid();

	target_->recv(p);
}
