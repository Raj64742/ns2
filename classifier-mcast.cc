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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-mcast.cc,v 1.1.1.1 1996/12/19 03:22:44 mccanne Exp $";
#endif

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "classifier.h"

class MCastClassifier : public Classifier {
public:
	MCastClassifier();
	~MCastClassifier();
protected:
	int command(int argc, const char*const* argv);
	int classify(const Packet* p);
	int findslot();
	void set_hash(nsaddr_t src, nsaddr_t dst, int slot);
	int hash(nsaddr_t src, nsaddr_t dst) const {
		u_int32_t s = src ^ dst;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s & 0xff);
	}
	struct hashnode {
		int slot;
		nsaddr_t src;
		nsaddr_t dst;
		hashnode* next;
	};
	hashnode* ht_[256];
	const hashnode* lookup(nsaddr_t src, nsaddr_t dst) const;
};

static class MCastClassifierClass : public TclClass {
public:
	MCastClassifierClass() : TclClass("classifier/mcast") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new MCastClassifier());
	}
} class_mcast_classifier;

MCastClassifier::MCastClassifier()
{
	memset(ht_, 0, sizeof(ht_));
}

MCastClassifier::~MCastClassifier()
{
	for (int i = 0; i < 256; ++i) {
		hashnode* p = ht_[i];
		while (p != 0) {
			hashnode* n = p->next;
			delete p;
			p = n;
		}
	}
}

const MCastClassifier::hashnode*
MCastClassifier::lookup(nsaddr_t src, nsaddr_t dst) const
{
	int h = hash(src, dst);
	for (const hashnode* p = ht_[h]; p != 0; p = p->next) {
		if (p->src == src && p->dst == dst)
			break;
	}
	return (p);
}

int MCastClassifier::classify(const Packet* pkt)
{
	nsaddr_t src = pkt->src_ >> 8;/*XXX*/
	nsaddr_t dst = pkt->dst_;
	const hashnode* p = lookup(src, dst);
	if (p == 0) {
		/*
		 * Didn't find an entry.
		 * Call tcl exactly once to install one.
		 * If tcl doesn't come through then fail.
		 */
		Tcl::instance().evalf("%s new-group %u %u", name(), src, dst);
		p = lookup(src, dst);
		if (p == 0)
			return (-1);
	}

	return (p->slot);
}

int MCastClassifier::findslot()
{
	for (int i = 0; i < nslot_; ++i)
		if (slot_[i] == 0)
			break;
	return (i);
}

void MCastClassifier::set_hash(nsaddr_t src, nsaddr_t dst, int slot)
{
	int h = hash(src, dst);
	hashnode* p = new hashnode;
	p->src = src;
	p->dst = dst;
	p->slot = slot;
	p->next = ht_[h];
	ht_[h] = p;
}

int MCastClassifier::command(int argc, const char*const* argv)
{
	/*
	 * $classifier set-hash $src $group $slot
	 */
	if (argc == 5) {
		if (strcmp(argv[1], "set-hash") == 0) {
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int slot = atoi(argv[4]);
			set_hash(src, dst, slot);
			return (TCL_OK);
		}
	}
	return (Classifier::command(argc, argv));
}

