/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 *
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/ll.cc,v 1.23 1998/08/05 01:58:42 gnguyen Exp $ (UCB)";
#endif

#include "errmodel.h"
#include "mac.h"
#include "ll.h"

int hdr_ll::offset_;

static class LLHeaderClass : public PacketHeaderClass {
public:
	LLHeaderClass()	: PacketHeaderClass("PacketHeader/LL",
					    sizeof(hdr_ll)) {
		offset(&hdr_ll::offset_);
	}
} class_hdr_ll;


static class LLClass : public TclClass {
public:
	LLClass() : TclClass("LL") {}
	TclObject* create(int, const char*const*) {
		return (new LL);
	}
} class_ll;


LL::LL() : seqno_(0), ackno_(0), macDA_(0), ifq_(0), sendtarget_(0), recvtarget_(0)
{
	bind("macDA_", &macDA_);
}

int LL::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "ifq") == 0) {
			ifq_ = (Queue*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "sendtarget") == 0) {
			sendtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "recvtarget") == 0) {
			recvtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}

	else if (argc == 2) {
		if (strcmp(argv[1], "ifq") == 0) {
			tcl.resultf("%s", ifq_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "sendtarget") == 0) {
			tcl.resultf("%s", sendtarget_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "recvtarget") == 0) {
			tcl.resultf("%s", recvtarget_->name());
			return (TCL_OK);
		}
	}
	return LinkDelay::command(argc, argv);
}


void LL::recv(Packet* p, Handler* h)
{
	if (h == 0) {		// from MAC classifier
		if (recvtarget_)
			recvfrom(p);
		else
			drop(p);
	}
	else {
		hdr_ll::access(p)->lltype() = LL_DATA;
		sendto(p, h);
	}
}


Packet* LL::sendto(Packet* p, Handler* h)
{	
	hdr_ll::access(p)->seqno_ = ++seqno_;
	hdr_mac::access(p)->macDA_ = macDA_;
	sendtarget_->recv(p);
	if (h) {
		Scheduler& s = Scheduler::instance();
		s.schedule(h, &intr_, txtime(p));
	}
	return p;
}


Packet* LL::recvfrom(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	if (hdr_cmn::access(p)->error() != 0)
		drop(p);
	else
		s.schedule(recvtarget_, p, delay_);
	return p;
}
