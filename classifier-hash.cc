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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-hash.cc,v 1.12 1997/08/08 01:24:33 gnguyen Exp $ (LBL)";
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
	virtual int compare(hnode*, nsaddr_t, nsaddr_t, int) = 0;
	hnode* lookup(Packet*);
	hnode* lookup(nsaddr_t, nsaddr_t, int);
	hnode* lookup(int, nsaddr_t, nsaddr_t, int);
	virtual int find_hash(nsaddr_t src, nsaddr_t dst, int fid) = 0;
	int command(int argc, const char*const* argv);
	int newflow(Packet*);
	void insert(int, nsaddr_t, nsaddr_t, int, int);
	void reset();
	int classify(Packet *const p) {

		hnode *hn = lookup(p);
		if (hn != NULL)
			return (hn->slot);
		else if (default_ >= 0)
			return (default_);
		return (newflow(p));
	}
	nsaddr_t mask_;
	int shift_;
	int default_;
	int buckets_;
	hnode* htab_;
};

class SrcDestFidHashClassifier : public HashClassifier {
public:
	SrcDestFidHashClassifier(int nbuck) : HashClassifier(nbuck) {}
protected:
	int command(int argc, const char*const* argv);
	int hash(u_int32_t s, u_int32_t d, int f) {
		s = s ^ d ^ f;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s % buckets_);
	}
	int compare(hnode *hn, nsaddr_t src, nsaddr_t dst, int fid) {
		return (hn->active && hn->src == src &&
			hn->dst == ((dst >> shift_) & mask_)
			&& hn->fid == fid);
	}
	int find_hash(nsaddr_t src, nsaddr_t dst, int fid) {
		int buck = hash(src, dst, fid);
		return (buck);
	}
};

class SrcDestHashClassifier : public HashClassifier {
public:
	SrcDestHashClassifier(int nb) : HashClassifier(nb) {}
protected:
	int command(int argc, const char*const* argv);
	int hash(u_int32_t s, u_int32_t d) {
		s = s ^ d;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s % buckets_);
	}
	int compare(hnode *hn, nsaddr_t src, nsaddr_t dst, int) {
		return (hn->active && hn->src == src &&
			hn->dst == ((dst >> shift_) & mask_));
	}
	int find_hash(nsaddr_t src, nsaddr_t dst, int) {
		int buck = hash(src, dst);
		return (buck);
	}
};

class FidHashClassifier : public HashClassifier {
public:
	FidHashClassifier(int nb) : HashClassifier(nb) {}
protected:
	int command(int argc, const char*const* argv);
	int hash(int fid) {
		return (fid % buckets_);
	}
	int compare(hnode *hn, nsaddr_t, nsaddr_t, int fid) {
		return (hn->active && hn->fid == fid);
	}
	int find_hash(nsaddr_t, nsaddr_t, int fid) {
		return(hash(fid));
	}
};

/**************  TCL linkage ****************/
static class SrcDestHashClassifierClass : public TclClass {
public:
	SrcDestHashClassifierClass() : TclClass("Classifier/Hash/SrcDest") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc < 5) {
			fprintf(stderr, "SrcDestHashClassifier ctor requires buckets arg\n");
			abort();
		}
		int buckets = atoi(argv[4]);
		return (new SrcDestHashClassifier(buckets));
	}
} class_hash_srcdest_classifier;

static class FidHashClassifierClass : public TclClass {
public:
	FidHashClassifierClass() : TclClass("Classifier/Hash/Fid") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc < 5) {
			fprintf(stderr, "FidHashClassifier ctor requires buckets arg\n");
			abort();
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
		if (argc < 5) {
			fprintf(stderr, "SrcDstFidHashClassifier ctor requires buckets arg\n");
			abort();
		}
		int buckets = atoi(argv[4]);
		return (new SrcDestFidHashClassifier(buckets));
	}
} class_hash_srcdestfid_classifier;

/****************** HashClassifier Methods ************/

HashClassifier::HashClassifier(int b) : mask_(~0), shift_(0),
	default_(-1), buckets_(b), htab_(NULL)
{ 
	// shift and mask operations on dest address
	bind("mask_", (int*)&mask_);
	bind("shift_", &shift_);
	bind("default_", &default_);
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
	Tcl& tcl = Tcl::instance();
        /*
         * $classifier set-hash $hashbucket src dst fid $slot
         */

	if (argc == 7) {
		if (strcmp(argv[1], "set-hash") == 0) {
			nsaddr_t src = atoi(argv[3]);
			nsaddr_t dst = atoi(argv[4]);
			int fid = atoi(argv[5]);
			int slot = atoi(argv[6]);

			int buck;
			if (strcmp(argv[2], "auto") == 0)
				buck = find_hash(src, dst, fid);
			else
				buck = atoi(argv[2]);

//printf("classifier-hash(%s), set-hash [%d/%d/%d] (buck:%d)(slot:%d)[%s]\n",
//name(), src, dst, fid, buck, slot, slot_[slot]->name());

			insert(buck, src, dst, fid, slot);
			return (TCL_OK);
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "lookup") == 0) {
			nsaddr_t src = atoi(argv[3]);
			nsaddr_t dst = atoi(argv[4]);
			int fid = atoi(argv[5]);
			int buck;
			if (strcmp(argv[2], "auto") == 0)
				buck = find_hash(src, dst, fid);
			else
				buck = atoi(argv[2]);
			hnode* hn = lookup(buck, src, dst, fid);
			if (hn && slot_[hn->slot]) {
				tcl.resultf("%s",
					slot_[hn->slot]->name());
				return (TCL_OK);
			}
			tcl.resultf("");
			return (TCL_OK);
		}
	} else if (argc == 5) {
		/* $classifier del-hash src dst fid */
		if (strcmp(argv[1], "del-hash") == 0) {
			nsaddr_t src = atoi(argv[2]);
			nsaddr_t dst = atoi(argv[3]);
			int fid = atoi(argv[4]);
			hnode* hn = lookup(src, dst, fid);

//printf("classifier-hash(%s), deleting [%d/%d/%d] (node:%p)(slot:%d)\n",
//name(), src, dst, fid, hn, hn->slot);

			if (hn != NULL) {
				hn->active = 0;
				tcl.resultf("%u", hn->slot);
				return (TCL_OK);
			}
			return (TCL_ERROR);
		}
	}
        return (Classifier::command(argc, argv));
}


int
HashClassifier::newflow(Packet* pkt)
{
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);

	int buck = find_hash(h->src(), h->dst(), h->flowid());
	Tcl::instance().evalf("%s unknown-flow %u %u %u %u",
		name(), h->src(), h->dst(), h->flowid(), buck);
	hnode* hn = lookup(pkt);
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
HashClassifier::hnode*
HashClassifier::lookup(int buck, nsaddr_t src, nsaddr_t dst, int fid)
{
	HashClassifier::hnode* hn;
	hn = &htab_[buck];
	while (hn != NULL) {
		if (compare(hn, src, dst, fid))
			break;
		hn = hn->next;
	}
	return (hn);
}

HashClassifier::hnode*
HashClassifier::lookup(nsaddr_t src, nsaddr_t dst, int fid)
{
	int bucknum = find_hash(src, ((dst >> shift_) & mask_), fid);
	return (lookup(bucknum, src, dst, fid));
}

HashClassifier::hnode*
HashClassifier::lookup(Packet *pkt)
{
	hdr_ip* h = (hdr_ip*)pkt->access(off_ip_);
	return (lookup(h->src(), h->dst(), h->flowid()));
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
