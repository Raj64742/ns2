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
 */

#include "ll.h"
#include "mac.h"


static class LLHeaderClass : public PacketHeaderClass {
public:
	LLHeaderClass()
		: PacketHeaderClass("PacketHeader/LL", sizeof(hdr_ll)) {}
} class_llhdr;

static class LLClass : public TclClass {
public:
	LLClass() : TclClass("LL") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new LL);
	}
} class_ll;


LL::LL() : BiConnector(), delay_(0), seqno_(0), peerLL_(0), mac_(0)
{
	bind_time("delay_", &delay_);
	bind("off_ll_", &off_ll_);
	bind("off_mac_", &off_mac_);
}


int
LL::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "peerLL") == 0) {
			peerLL_ = (LL*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mac") == 0) {
			mac_ = (Mac*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}

	else if (argc == 2) {
		if (strcmp(argv[1], "peerLL") == 0) {
			tcl.resultf("%s", peerLL_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mac") == 0) {
			tcl.resultf("%s", mac_->name());
			return (TCL_OK);
		}
	}
	return BiConnector::command(argc, argv);
}


void
LL::recv(Packet* p, Handler* h)
{
	Scheduler& s = Scheduler::instance();
	hdr_ll *llh = (hdr_ll*)p->access(off_ll_);
	if (h == 0) {		// from MAC classifier
		if (llh->error())
			drop(p);
		else
			s.schedule(recvtarget_, p, delay_);
		return;
	}
	llh->seqno() = ++seqno_;
	llh->error() = 0;
	((hdr_mac*)p->access(off_mac_))->macDA() = peerLL_->mac()->label();

	s.schedule(sendtarget_, p, delay_);
	s.schedule(h, &intr_, 0.000001); // XXX
}
