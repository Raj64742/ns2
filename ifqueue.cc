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

#include "ifqueue.h"


static class IFQueueClass : public TclClass {
public:
	IFQueueClass() : TclClass("IFQueue") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new IFQueue);
	}
} class_ifqueue;


IFQueue::IFQueue() : Queue(), mac_(0), hRs_(*this)
{
}


int
IFQueue::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "mac") == 0) {
			mac_ = (Mac*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}


void
IFQueue::recv(Packet* p, Handler* h)
{
	q_.enque(p);
	if (!blocked_) {
		p = deque();
		if (p) {
			blocked_ = 1;
			mac_->recv(p, &qh_);
		}
	}
}


void
IFQueue::enque(Packet* p)
{
	q_.enque(p);
}


Packet*
IFQueue::deque()
{
	// target_ = tq_.deque();
	return q_.deque();
}


void
IFQueue::resume()
{
	Packet* p = q_.deque();
	NsObject* target = 0;	// tq_.deque();
	if (p)
		mac_->recv(p, &qh_);
	else
		blocked_ = 0;
	
}


void
IFQueue::reset()
{
	while (Packet* p = q_.deque())
		drop(p);
//	while (tq_.deque());
}


void
IFQueueHandlerRs::handle(Event* e)
{
	ifq_.resume();
}
