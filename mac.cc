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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/mac.cc,v 1.27 1998/08/13 00:12:41 gnguyen Exp $ (UCB)";
#endif

#include "classifier.h"
#include "channel.h"
#include "mac.h"

int hdr_mac::offset_;

static class MacHeaderClass : public PacketHeaderClass {
public:
	MacHeaderClass() : PacketHeaderClass("PacketHeader/Mac",
					     sizeof(hdr_mac)) {
		bind_offset(&hdr_mac::offset_);
	}
	void export_offsets() {
		field_offset("macSA_", OFFSET(hdr_mac, macSA_));
		field_offset("macDA_", OFFSET(hdr_mac, macDA_));
	}
} class_hdr_mac;

static class MacClass : public TclClass {
public:
	MacClass() : TclClass("Mac") {}
	TclObject* create(int, const char*const*) {
		return (new Mac);
	}
} class_mac;


void
MacHandlerResume::handle(Event*)
{
	mac_->resume();
}

void
MacHandlerSend::handle(Event* e)
{
	mac_->send((Packet*)e);
}


Mac::Mac() : Connector(), hlen_(0), state_(MAC_IDLE), channel_(0), callback_(0), hRes_(this), hSend_(this), macList_(0)
{
	bind_time("delay_", &delay_);
	bind_bw("bandwidth_", &bandwidth_);
	bind("hlen_", &hlen_);
	bind("addr_", &addr_);
}


int Mac::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "channel") == 0) {
			channel_ = (Channel*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "classifier") == 0) {
			mcl_ = (Classifier*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "maclist") == 0) {
			macList_ = (Mac*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "channel") == 0) {
			tcl.resultf("%s", channel_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "classifier") == 0) {
			tcl.resultf("%s", mcl_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "maclist") == 0) {
			tcl.resultf("%s", macList_->name());
			return (TCL_OK);
		}
	}
	return Connector::command(argc, argv);
}


void Mac::recv(Packet* p, Handler* h)
{
	// if h is NULL, packet comes from the lower layer, ie. MAC classifier
	if (h == 0) {
		state(MAC_IDLE);
		Scheduler::instance().schedule(target_, p, delay_);
		return;
	}
	callback_ = h;
	hdr_mac* mh = hdr_mac::access(p);
	mh->set(MF_DATA, addr_);
	state(MAC_SEND);
	send(p);
}


void Mac::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p);
	channel_->send(p, txt);
	s.schedule(&hRes_, &intr_, txt);
}


void Mac::resume(Packet* p)
{
	if (p != 0)
		drop(p);
	state(MAC_IDLE);
	callback_->handle(&intr_);
}


Mac* Mac::getPeerMac(Packet* p)
{
	return (Mac*) mcl_->slot(hdr_mac::access(p)->macDA());
}
