/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/agent.cc,v 1.16 1997/06/20 02:50:02 heideman Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <string.h>

#include "Tcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "flags.h"

static class AgentClass : public TclClass {
public:
	AgentClass() : TclClass("Agent") {} 
	TclObject* create(int argc, const char*const* argv) {
		return (new Agent(-1));
	}
} class_agent;

int Agent::uidcnt_;		/* running unique id */


Agent::Agent(int pkttype) : 
	addr_(-1), dst_(-1), size_(0), type_(pkttype), fid_(-1),
	prio_(-1), flags_(0)
{
	memset(pending_, 0, sizeof(pending_));
	// this is really an IP agent, so set up
	// for generating the appropriate IP fields...
	bind("addr_", (int*)&addr_);
	bind("dst_", (int*)&dst_);
	bind("fid_", (int*)&fid_);
	bind("prio_", (int*)&prio_);
	bind("flags_", (int*)&flags_);
	/*
	 * XXX Kevin replaced class_ with fid_.
	 * I don't understand this.  It's just a name
	 * change that has broke my scripts.  I added this
	 * extra bind as a workaround...  -Steve
	 */
	bind("class_", (int*)&fid_);

	bind("off_ip_", &off_ip_);
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
Packet* Agent::allocpkt() const
{
	Packet* p = Packet::alloc();
	hdr_cmn* th = (hdr_cmn*)p->access(off_cmn_);
	th->uid() = uidcnt_++;
	th->ptype() = type_;
	th->size() = size_;
	hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
	iph->src() = addr_;
	iph->dst() = dst_;
	iph->flowid() = fid_;
	iph->prio() = prio_;
	iph->ttl() = 32; /*XXX*/
	iph->iface() = -2;

	hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
	hf->ecn_ = 0;
	hf->ecn_to_echo_ = 0;
	hf->pri_ = 0;
	hf->usr1_ = 0;
	hf->usr2_ = 0;

	return (p);
}

/* allocate a packet and fill in all the generic fields and allocate
 * a buffer of n bytes for data
 */
Packet* Agent::allocpkt(int n) const
{
        Packet* p = allocpkt();

	if (n > 0)
	        p->allocdata(n);

	return(p);
}



