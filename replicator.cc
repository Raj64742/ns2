/*
 * Copyright (c) 1996 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/replicator.cc,v 1.4 1997/02/27 04:39:00 kfall Exp $";
#endif

#include "classifier.h"
#include "packet.h"
#include "ip.h"

/*
 * A replicator is not really a packet classifier but
 * we simply find convenience in leveraging its slot table.
 * (this object used to implement fan-out on a multicast
 * router as well as broadcast LANs)
 */
class Replicator : public Classifier {
public:
	Replicator();
	void recv(Packet*, Handler* h = 0);
	virtual int classify(Packet* const) {};
protected:
	int ignore_;
};

static class ReplicatorClass : public TclClass {
public:
	ReplicatorClass() : TclClass("Classifier/Replicator") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Replicator());
	}
} class_replicator;

Replicator::Replicator() : ignore_(0)
{
	bind("ignore", &ignore_);
}

void Replicator::recv(Packet* p, Handler*)
{
	IPHeader *iph = IPHeader::access(p->bits());
	if (maxslot_ < 0) {
		if (!ignore_)
			Tcl::instance().evalf("%s drop %u %u", name(), 
				iph->src(), iph->dst());
		Packet::free(p);
		return;
	}
	for (int i = 0; i < maxslot_; ++i) {
		NsObject* o = slot_[i];
		if (o != 0)
			o->recv(p->copy());
	}
	/* we know that maxslot is non-null */
	slot_[maxslot_]->recv(p);
}
