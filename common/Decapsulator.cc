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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/Decapsulator.cc,v 1.3 2000/09/01 03:04:04 haoboy Exp $

#include "Decapsulator.h"
#include "ip.h"
#include "packet.h"
#include "encap.h"

static class DecapsulatorClass : public TclClass {
public:
	DecapsulatorClass() : TclClass("Agent/Decapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Decapsulator());
	}
} class_decapsulator;

Decapsulator::Decapsulator()  : Agent(PT_ENCAPSULATED)
{ 
}

Packet* const Decapsulator::decapPacket(Packet* const p) 
{
	hdr_cmn* ch= hdr_cmn::access(p);
	if (ch->ptype() == PT_ENCAPSULATED) {
		hdr_encap* eh= hdr_encap::access(p);
		return eh->decap();
	}
	return 0;
}
void Decapsulator::recv(Packet* p, Handler* h)
{
        Packet *decap_p= decapPacket(p);
	if (decap_p) {
		send(decap_p, h);
		Packet::free(p);
		return;
	}
	send(p, h);
}
