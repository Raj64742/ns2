// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/Encapsulator.cc,v 1.5 2000/09/01 03:04:04 haoboy Exp $

#include "packet.h"
#include "ip.h"
#include "Encapsulator.h"
#include "encap.h"

static class EncapsulatorClass : public TclClass {
public:
	EncapsulatorClass() : TclClass("Agent/Encapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Encapsulator());
	}
} class_encapsulator;

Encapsulator::Encapsulator() : 
	Agent(PT_ENCAPSULATED), 
	d_target_(0)
{
	bind("status_", &status_);
	bind("overhead_", &overhead_);
}

int Encapsulator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "decap-target") == 0) {
			if (d_target_ != 0) tcl.result(d_target_->name());
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "decap-target") == 0) {
			d_target_ = (NsObject*)TclObject::lookup(argv[2]);
			// even if it's zero, it's OK, we'll just not send to such
			// a target then.
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void Encapsulator::recv(Packet* p, Handler* h)
{
	if (d_target_) {
		Packet *copy_p= p->copy();
		d_target_->recv(copy_p, h);
	}
	if (status_) {
		Packet* ep= allocpkt(); //sizeof(Packet*));
		hdr_encap* eh= hdr_encap::access(ep);
		eh->encap(p);
		//Packet** pp= (Packet**) encap_p->accessdata();
		//*pp= p;

		hdr_cmn* ch_e= hdr_cmn::access(ep);
		hdr_cmn* ch_p= hdr_cmn::access(p);
	  
		ch_e->ptype()= PT_ENCAPSULATED;
 		ch_e->size()= ch_p->size() + overhead_;
		ch_e->timestamp()= ch_p->timestamp();
		send(ep, h);
	}
	else send(p, h); //forward the packet as it is
}
