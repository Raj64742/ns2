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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ctrMcast.cc,v 1.6.2.1 1998/07/15 18:34:11 kannan Exp $ (USC/ISI)";
#endif

#include "agent.h"
#include "ip.h"
#include "ctrMcast.h"

class CtrMcastEncap : public Agent {
public:
	CtrMcastEncap() : Agent(PT_CtrMcast_Encap) { 
		bind("off_CtrMcast_", &off_CtrMcast_);
	}
	void recv(Packet* p, Handler*);
protected:
	int off_CtrMcast_;
};

class CtrMcastDecap : public Agent {
public:
	CtrMcastDecap() : Agent(PT_CtrMcast_Decap) { 
		bind("off_CtrMcast_", &off_CtrMcast_);
	}
	void recv(Packet* p, Handler*);
protected:
	int off_CtrMcast_;
};

static class CtrMcastHeaderClass : public PacketHeaderClass {
public:
	CtrMcastHeaderClass() : PacketHeaderClass("PacketHeader/CtrMcast",
						  sizeof(hdr_CtrMcast)) { } 
} class_CtrMcast_hdr;

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


void CtrMcastEncap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = (hdr_CtrMcast*)p->access(off_CtrMcast_);
	hdr_ip* ih = (hdr_ip*)p->access(off_ip_);

	ch->src() = ih->src();
	ch->group() = ih->dst();
	ch->flowid() = ih->flowid();
	ih->src() = addr_;
	ih->dst() = dst_;
	ih->flowid() = fid_;

	target_->recv(p);
}

void CtrMcastDecap::recv(Packet* p, Handler*)
{
	hdr_CtrMcast* ch = (hdr_CtrMcast*)p->access(off_CtrMcast_);
	hdr_ip* ih = (hdr_ip*)p->access(off_ip_);

	ih->src() = ch->src();
	ih->dst() = ch->group();
	ih->flowid() = ch->flowid();

	target_->recv(p);
}
