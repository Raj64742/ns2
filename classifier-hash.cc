/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-hash.cc,v 1.4 1997/06/03 21:33:38 kannan Exp $ (LBL)";
#endif

//
// a generalized classifier for mapping (src/dest/flowid) fields
// to a bucket.  "buckets_" worth of hash table entries are created
// at init time, and other entries in the same bucket are created when
// needed
//
//

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

/* class defs for HashClassifier (base), SrcDest, SrcDestFid HashClassifiers */

class HashClassifier : public Classifier {
public:
	HashClassifier(int nbuckets);
	~HashClassifier();
protected:
	struct hnode {
		int active;
		int slot;
		nsaddr_t src, dst;
		int fid;
		hnode *next;
	};
	virtual const hnode* lookup(Packet*) = 0;
	virtual void set_hash(nsaddr_t src, nsaddr_t dst, int fid, int slot) = 0;
	int command(int argc, const char*const* argv);
	int newflow(Packet*);
	void insert(int, nsaddr_t, nsaddr_t, int, int);
	void reset();
	int classify(Packet *const p) {
		const hnode *hn = lookup(p);
		if (hn != NULL)
			return (hn->slot);
		return (newflow(p));
	}
	nsaddr_t mask_;
	int shift_;
	int buckets_;
	hnode* htab_;
};

class SrcDestFidHashClassifier : public HashClassifier {
public:
	SrcDestFidHashClassifier(int nbuck) : HashClassifier(nbuck) {}
protected:
	int command(int argc, const char*const* argv);
	const hnode* lookup(Packet *);
	int hash(u_int32_t s, u_int32_t d, int f) {
		s = s ^ d ^ f;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s % buckets_);
	}
	int compare(hnode *hn, Packet *p) {
		hdr_ip* h = (hdr_ip*)p->access(off_ip_);
		return (hn->active && hn->src == h->src() &&
			hn->dst == ((h->dst() >> shift_) & mask_)
			&& hn->fid == h->flowid());
	}
	void set_hash(nsaddr_t src, nsaddr_t dst, int fid, int slot) {
		int buck = hash(src, dst, fid);
		insert(buck, src, dst, fid, slot);
	}
};

class SrcDestHashClassifier : public HashClassifier {
public:
	SrcDestHashClassifier(int nb) : HashClassifier(nb) {}
protected:
	int command(int argc, const char*const* argv);
	const hnode* lookup(Packet *);
	int hash(u_int32_t s, u_int32_t d) {
		s = s ^ d;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s % buckets_);
	}
	int compare(hnode *hn, Packet *p) {
		hdr_ip* h = (hdr_ip*)p->access(off_ip_);
		return (hn->active && hn->src == h->src() &&
			hn->dst == ((h->dst() >> shift_) & mask_));
	}
	void set_hash(nsaddr_t src, nsaddr_t dst, int fid, int slot) {
		int buck = hash(src, dst);
		insert(buck, src, dst, fid, slot);
	}
};

class FidHashClassifier : public HashClassifier {
public:
	FidHashClassifier(int nb) : HashClassifier(nb) {}
protected:
	int command(int argc, const char*const* argv);
	const hnode* lookup(Packet *);
	int hash(int fid) {
		return (fid % buckets_);
	}
	int compare(hnode *hn, Packet *p) {
		hdr_ip* h = (hdr_ip*)p->access(off_ip_);
		return (hn->active && hn->fid == h->flowid());
	}
	void set_hash(nsaddr_t src, nsaddr_t dst, int fid, int slot) {
		int buck = hash(fid);
		insert(buck, src, dst, fid, slot);
	}
};

/**************  TCL linkage ****************/
static class SrcDestHashClassifierClass : public TclClass {
public:
	SrcDestHashClassifierClass() : TclClass("Classifier/Hash/SrcDest") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc < 2) {
			fprintf(stderr, "SrcDestHashClassifier ctor requires buckets arg\n");
			exit(1);
		}
		int buckets = atoi(argv[1]);
		return (new SrcDestHashClassifier(buckets));
	}
} class_hash_srcdest_classifier;

static class FidHashClassifierClass : public TclClass {
public:
	FidHashClassifierClass() : TclClass("Classifier/Hash/Fid") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc < 5) {
			fprintf(stderr, "FidHashClassifier ctor requires buckets arg\n");
			exit(1);
		}
		int buckets = atoi(argv[4]);
		return (new FidHashClassifier(buckets));
	}
} class_hash_fid_classifier;

static class SrcDestFidHashClassifierClass : public TclClass {
public:
	SrcDestFidHashClassifierClass() :
		TclClass("Classifier/Hash/SrcDestFid") {}

	TclObject* create(int argc, const char*const* argv) {
		if (argc < 2) {
			fprintf(stderr, "SrcDstFidHashClassifier ctor requires buckets arg\n");
			exit(1);
		}
		int buckets = atoi(argv[1]);
		return (new SrcDestFidHashClassifier(buckets));
	}
} class_hash_srcdestfid_classifier;

/****************** HashClassifier Methods ************/

HashClassifier::HashClassifier(int b) : mask_(~0), shift_(0),
	buckets_(b), htab_(NULL)
{ 
	// shift and mask operations on dest address
	bind("mask_", (int*)&mask_);
	bind("shift_", &shift_);
	// number of buckets in hashtable
	htab_ = new hnode[buckets_];
	if (htab_ != NULL)
		memset(htab_, '\0', sizeof(hnode) * buckets_);
	else
		fprintf(stderr, "HashClassifier: out of memory\n");
}

HashClassifier::~HashClassifier()
{
	register i;
	hnode *p;
	hnode *n;
	for (i = 0; i < buckets_; i++) {
		p = htab_[i].next;
		while (p != NULL) {
			n = p;
			p = p->next;
			delete n;
		}
	}
	delete htab_;
}
int HashClassifier::command(int argc, const char*const* argv)
{
        /*
         * $classifier set-hash $src $dst $fid $slot
         */

	if (argc < 5) 
		return (Classifier::command(argc, argv));

	if (strcmp(argv[1], "set-hash") == 0) {
		if (strcmp(argv[2], "fid") == 0) {
			NsObject* cl = (NsObject*)TclObject::lookup(argv[3]);
			int fid = atoi(argv[4]);
			set_hash(0, 0, fid, fid);
			install(fid, cl);	// Classifier::install()
			return (TCL_OK);
		}
	}
        return (Classifier::command(argc, argv));
}


int
HashClassifier::newflow(Packet* pkt)
{
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);
	Tcl::instance().evalf("%s unknown-flow %u %u %u",
		name(), h->src(), h->dst(), h->flowid());
	const hnode* hn = lookup(pkt);
	if (hn == NULL)
		return (-1);
	return(hn->slot);
}

void
HashClassifier::reset()
{
	hnode *h;
	register i;
	for (i = 0; i < buckets_; i++) {
		htab_[i].active = 0;
		h = htab_[i].next;
		while (h != NULL) {
			delete h;
			h = h->next;
		}
		htab_[i].next = NULL;
	}
}

void
HashClassifier::insert(int buck, nsaddr_t src, nsaddr_t dst, int fid, int slot)
{
	hnode *p;
	if (htab_[buck].active) {
		p = new hnode;
		p->next = htab_[buck].next;
		htab_[buck].next = p;
	} else
		p = &htab_[buck];
		
	p->active = 1;
	p->src = src;
	p->dst = dst;
	p->fid = fid;
	p->slot = slot;
}

/****************** Specific Lookup Methods **********************/

const HashClassifier::hnode*
SrcDestHashClassifier::lookup(Packet *pkt)
{
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);
	HashClassifier::hnode* hn;
	int bucknum = hash(h->src(), ((h->dst() >> shift_) & mask_));
	hn = &htab_[bucknum];
	while (hn != NULL) {
		if (compare(hn, pkt))
			break;
		hn = hn->next;
	}
	return (hn);
}

const HashClassifier::hnode*
SrcDestFidHashClassifier::lookup(Packet *pkt)
{
	HashClassifier::hnode* hn;
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);
	int bucknum = hash(h->src(), ((h->dst() >> shift_) & mask_), h->flowid());
	hn = &htab_[bucknum];
	while (hn != NULL) {
		if (compare(hn, pkt))
			break;
		hn = hn->next;
	}
	return (hn);
}

const HashClassifier::hnode*
FidHashClassifier::lookup(Packet *pkt)
{
	HashClassifier::hnode* hn;
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);
	int bucknum = hash(h->flowid());
	hn = &htab_[bucknum];
	while (hn != NULL) {
		if (compare(hn, pkt))
			break;
		hn = hn->next;
	}
	return (hn);
}

int SrcDestHashClassifier::command(int argc, const char*const* argv)
{
	return (HashClassifier::command(argc, argv));
}

int SrcDestFidHashClassifier::command(int argc, const char*const* argv)
{
	return (HashClassifier::command(argc, argv));
}

int FidHashClassifier::command(int argc, const char*const* argv)
{
	return (HashClassifier::command(argc, argv));
}
