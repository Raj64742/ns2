/*
 * Copyright (c) 1990-1994 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory.
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

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/agent.cc,v 1.3 1997/01/26 22:32:24 mccanne Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <string.h>

#include "Tcl.h"
#include "agent.h"
#include "packet.h"

static class NullAgentClass : public TclClass {
public:
	NullAgentClass() : TclClass("agent/null") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Agent(-1));
	}
} class_null_agent;

Agent::Agent(int pkttype) : 
	type_(pkttype), dst_(-1), addr_(-1), seqno_(0), size_(0), class_(0)
{
	memset(pending_, 0, sizeof(pending_));
	/*
	 * XXX warning: we don't use "class" here because it conflicts
	 * with otcl's member class variable
	 */
	bind("cls", &class_);
	bind("addr", (int*)&addr_);
	bind("dst", (int*)&dst_);
	bind("seqno", &seqno_);
}

Agent::~Agent()
{
}

void Agent::sched(double delay, int tno)
{
	if (pending_[tno])
		/*XXX timers botched*/
		abort();

	pending_[tno] = 1;
	Scheduler& s = Scheduler::instance();
	s.schedule(this, &timer_[tno], delay);
}

void Agent::timeout(int)
{
}

void Agent::handle(Event* e)
{
	/*XXX use a subclass here*/
	if (e >= &timer_[0] && e < &timer_[NTIMER]) {
		int tno = e - &timer_[0];
		pending_[tno] = 0;
		timeout(tno);
	} else
		/* otherwise, can only be a packet */
		recv((Packet*)e);
}

void Agent::recv(Packet* p, Handler*)
{
	/*
	 * didn't expect packet (or we're a null agent?)
	 */
	Packet::free(p);
}

/*
 * allocate a packet and fill in all the generic fields
 */
Packet* Agent::allocpkt(int seqno) const
{
	Packet* p = Packet::alloc();
	p->type_ = type_;
	p->flags_ = 0;
	p->class_ = class_;
	p->src_ = addr_;
	p->dst_ = dst_;
	p->size_ = size_;
	p->seqno_ = seqno;
	return (p);
}
