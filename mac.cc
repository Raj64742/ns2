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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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

#include "mac.h"


static class MacClass : public TclClass {
public:
	MacClass() : TclClass("Mac/Base") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Mac);
	}
} class_mac;


Mac::Mac() : BiConnector(), channel_(0), callback_(0), mh_(*this), macList_(0)
{
	bind_bw("bandwidth_", &bandwidth_);
}


int
Mac::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "channel") == 0) {
			channel_ = (Channel*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "channel") == 0) {
			tcl.resultf("%s", channel_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "set-maclist") == 0) { // circular list
			macList_ = (Mac *) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return BiConnector::command(argc, argv);
}


void
Mac::recv(Packet* p, Handler* h)
{
	callback_ = h;
	send(p);
}


void
Mac::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p);
	channel_->send(p, p->target(), txt);
	s.schedule(callback_, &intr_, txt);
}


void
MacHandler::handle(Event* e)
{
	mac_.send((Packet*)e);
}
