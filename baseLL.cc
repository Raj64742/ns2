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

#include "baseLL.h"
#include "ifqueue.h"


static class BaseLLClass : public TclClass {
public:
	BaseLLClass() : TclClass("LL/Base") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new BaseLL);
	}
} class_baseLL;


BaseLL::BaseLL() : LinkDelay(), ifq_(0), em_(0)
{
}


int
BaseLL::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "ifqueue") == 0) {
			ifq_ = (IFQueue*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "error") == 0) {
			em_ = (ErrorModel*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(srgv[1], "sendtarget") == 0) {
			sendtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(srgv[1], "recvtarget") == 0) {
			recvtarget_ = (NsObject*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return LinkDelay::command(argc, argv);
}


void
BaseLL::recv(Packet* p, Handler* h)
{
	if (em_ && em_->corrupt(p)) {
		p->error(1);
	}
	p->target_ = sendtarget;  /* peer link layer */
	ifq_->recv(p, h);         /* send it down to the interface queue */
}

void
BaseLL::handle(Event* e)    
{
	Scheduler& s = Scheduler::instance();
	((Packet*)e)->target_ = recvtarget;  /* the classifier on this node */
	s.schedule(recvtarget, e, 0);
}

