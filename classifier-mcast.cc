/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-mcast.cc,v 1.14 1998/08/12 23:40:59 gnguyen Exp $";
#endif

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class MCastClassifier : public Classifier {
public:
	MCastClassifier();
	~MCastClassifier();
protected:
	int command(int argc, const char*const* argv);
	int classify(Packet *const p);
	int findslot();
	void set_hash(nsaddr_t src, nsaddr_t dst, int slot, int iface);
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
		int iif; // for RPF checking
	};
	void clearAll();
	hashnode* ht_[256];
	const hashnode* lookup(nsaddr_t src, nsaddr_t dst) const;
	hashnode* lookupiface(nsaddr_t src, nsaddr_t dst, int iface) const;
	void change_iface(nsaddr_t src, nsaddr_t dst, int oldiface, int newiface);
};

static class MCastClassifierClass : public TclClass {
public:
	MCastClassifierClass() : TclClass("Classifier/Multicast") {}
	TclObject* create(int, const char*const*) {
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

void MCastClassifier::clearAll()
{
	for (int i = 0; i < 256; ++i) {
		hashnode* p = ht_[i];
		while (p != 0) {
			hashnode* n = p->next;
			delete p;
			p = n;
		}
	}
	memset(ht_, 0, sizeof(ht_));
}

const MCastClassifier::hashnode*
MCastClassifier::lookup(nsaddr_t src, nsaddr_t dst) const
{
	int h = hash(src, dst);
	const hashnode* p;
	for (p = ht_[h]; p != 0; p = p->next) {
		if (p->src == src && p->dst == dst)
			break;
	}
	return (p);
}

int MCastClassifier::classify(Packet *const pkt)
{
	hdr_cmn* h = hdr_cmn::access(pkt);
	hdr_ip* ih = hdr_ip::access(pkt);

	nsaddr_t src = ih->src() >> 8; /*XXX*/
	nsaddr_t dst = ih->dst();

	int iface = h->iface();

	const hashnode* p = lookupiface(src, dst, iface);
	if (p == 0) {
		p = lookup(src, dst);
		/*
		 * Didn't find an entry.
		 * Call tcl exactly once to install one.
		 * If tcl doesn't come through then fail.
		 */
		if (p == 0) {
			Tcl::instance().evalf("%s new-group %u %u %d %s", 
					      name(), src, dst, iface, "CACHE_MISS");
			return (-1);
		}
		if ( (p->iif != -1) && (p->iif != iface) ) {
			Tcl::instance().evalf("%s new-group %u %u %d %s", 
					      name(), src, dst, iface, "WRONG_IIF");
			//printf ("wrong_iff: %s %u %u %d\n", name(), src, dst, iface);
			return (-1);
		}
	}
	return (p->slot);
}

int MCastClassifier::findslot()
{
	int i;
	for (i = 0; i < nslot_; ++i)
		if (slot_[i] == 0)
			break;
	return (i);
}

void MCastClassifier::set_hash(nsaddr_t src, nsaddr_t dst, int slot, int iface)
{
	int h = hash(src, dst);
	hashnode* p = new hashnode;
	p->src = src;
	p->dst = dst;
	p->slot = slot;
	p->iif = iface;
	p->next = ht_[h];
	ht_[h] = p;
}

int MCastClassifier::command(int argc, const char*const* argv)
{
	/*
	 * $classifier set-hash $src $group $slot
	 */
	if (argc == 6) {
		if (strcmp(argv[1], "set-hash") == 0) {
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int slot = atoi(argv[4]);
			int iface = atoi(argv[5]);
			set_hash(src, dst, slot, iface);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "change-iface") == 0) {
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int oldiface = atoi(argv[4]);
			int newiface = atoi(argv[5]);
			change_iface(src, dst, oldiface, newiface);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		/*
		 * $classifier lookup-iface $src $group $iface
		 */
		if (strcmp(argv[1], "lookup-iface") == 0) {
			Tcl &tcl = Tcl::instance();
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int iface = atoi(argv[4]);
			hashnode *p = lookupiface(src, dst, iface);
			if ((p == NULL) || (slot_[p->slot] == 0))
				tcl.resultf("");
			else 
				tcl.resultf("%s", slot_[p->slot]->name());
			return (TCL_OK);
		}
	}
	if (argc == 2) {
		if (strcmp(argv[1], "clearAll") == 0) {
			clearAll();
			return (TCL_OK);
		}
	}
	return (Classifier::command(argc, argv));
}


/* interface look up for the interface code*/

MCastClassifier::hashnode*
MCastClassifier::lookupiface(nsaddr_t src, nsaddr_t dst, int iface) const
{
	int h = hash(src, dst);
	hashnode* p;
	for (p = ht_[h]; p != 0; p = p->next) {
		if (p->src == src && p->dst == dst && p->iif == iface)
			break;
	}
	return (p);
}

void MCastClassifier::change_iface(nsaddr_t src, nsaddr_t dst, int oldiface, int newiface)
{

	hashnode* p = lookupiface(src, dst, oldiface);
	p->iif = newiface;
}
