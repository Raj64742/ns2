/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-hash.cc,v 1.23 1999/09/18 03:34:46 heideman Exp $ (LBL)";
#endif

//
// a generalized classifier for mapping (src/dest/flowid) fields
// to a bucket.  "buckets_" worth of hash table entries are created
// at init time, and other entries in the same bucket are created when
// needed
//
//

extern "C" {
#include <tcl.h>
}

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

/* class defs for HashClassifier (base), SrcDest, SrcDestFid HashClassifiers */
class HashClassifier : public Classifier {
public:
	HashClassifier(int keylen) : default_(-1), keylen_(keylen) {
		// shift + mask picked up from underlying Classifier object
		bind("default_", &default_);
		Tcl_InitHashTable(&ht_, keylen);
	}		
	~HashClassifier() {
		Tcl_DeleteHashTable(&ht_);
	};
	int classify(const Packet * p) {
		int slot= lookup(p);
		if (slot >= 0 && slot <=maxslot_)
			return (slot);
		else if (default_ >= 0)
			return (default_);
		return (unknown(p));
	}
	virtual int lookup(Packet* p) {
		hdr_ip* h = hdr_ip::access(p);
		return get_hash(h->saddr(), h->daddr(), h->flowid());
	}
	virtual int unknown(Packet* p) {
		hdr_ip* h = hdr_ip::access(p);
		Tcl::instance().evalf("%s unknown-flow %u %u %u",
				      name(), h->saddr(), h->daddr(), h->flowid());
		return lookup(p);
	};

protected:
	union hkey {
		struct {
			int fid;
		} Fid;
		struct {
			nsaddr_t dst;
		} Dst;
		struct {
			nsaddr_t src, dst;
		} SrcDst;
		struct {
			nsaddr_t src, dst;
			int fid;
		} SrcDstFid;
	};
		
	int lookup(nsaddr_t src, nsaddr_t dst, int fid) {
		return get_hash(src, dst, fid);
	}
	int newflow(Packet* pkt) {
		hdr_ip* h = hdr_ip::access(pkt);
		Tcl::instance().evalf("%s unknown-flow %u %u %u",
				      name(), h->saddr(), h->daddr(), h->flowid());
		return lookup(pkt);
	};
	void reset() {
		Tcl_DeleteHashTable(&ht_);
		Tcl_InitHashTable(&ht_, keylen_);
	}

	virtual const char* hashkey(nsaddr_t, nsaddr_t, int)=0; 

	int set_hash(nsaddr_t src, nsaddr_t dst, int fid, int slot) {
		int newEntry;
		Tcl_HashEntry *ep= Tcl_CreateHashEntry(&ht_, hashkey(src, dst, fid), &newEntry);
		if (ep) {
			Tcl_SetHashValue(ep, slot);
			return slot;
		}
		return -1;
	}
	int get_hash(nsaddr_t src, nsaddr_t dst, int fid) {
		Tcl_HashEntry *ep= Tcl_FindHashEntry(&ht_, hashkey(src, dst, fid));
		if (ep)
			return (int)Tcl_GetHashValue(ep);
		return -1;
	}
	
	int command(int argc, const char*const* argv);


	int default_;
	Tcl_HashTable ht_;
	hkey buf_;
	int keylen_;
};

/****************** HashClassifier Methods ************/
int HashClassifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	/*
	 * $classifier set-hash $hashbucket src dst fid $slot
	 */

	if (argc == 7) {
		if (strcmp(argv[1], "set-hash") == 0) {
			//xxx: argv[2] is ignored for now
			nsaddr_t src = atoi(argv[3]);
			nsaddr_t dst = atoi(argv[4]);
			int fid = atoi(argv[5]);
			int slot = atoi(argv[6]);
			if (0 > set_hash(src, dst, fid, slot))
				return TCL_ERROR;
			return TCL_OK;
		}
	} else if (argc == 6) {
		/* $classifier lookup $hashbuck $src $dst $fid */
		if (strcmp(argv[1], "lookup") == 0) {
			nsaddr_t src = atoi(argv[3]);
			nsaddr_t dst = atoi(argv[4]);
			int fid = atoi(argv[5]);
			int slot= get_hash(src, dst, fid);
			if (slot>=0 && slot <=maxslot_) {
				tcl.resultf("%s", slot_[slot]->name());
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

			Tcl_HashEntry *ep= Tcl_FindHashEntry(&ht_, hashkey(src, dst, fid));
			if (ep) {
				int slot= (int)Tcl_GetHashValue(ep);
				Tcl_DeleteHashEntry(ep);
				tcl.resultf("%u", slot);
				return (TCL_OK);
			}
			return (TCL_ERROR);
		}
	}
	return (Classifier::command(argc, argv));
}


class SrcDestFidHashClassifier : public HashClassifier {
public:
	SrcDestFidHashClassifier() : HashClassifier(3) {
	}
protected:
	const char* hashkey(nsaddr_t src, nsaddr_t dst, int fid) {
		buf_.SrcDstFid.src= mshift(src);
		buf_.SrcDstFid.dst= mshift(dst);
		buf_.SrcDstFid.fid= fid;
		return (const char*) &buf_;
	}
};

class SrcDestHashClassifier : public HashClassifier {
public:
	SrcDestHashClassifier() : HashClassifier(2) {
	}
protected:
	const char*  hashkey(nsaddr_t src, nsaddr_t dst, int) {
		buf_.SrcDst.src= mshift(src);
		buf_.SrcDst.dst= mshift(dst);
		return (const char*) &buf_;
	}
};

class FidHashClassifier : public HashClassifier {
public:
	FidHashClassifier() : HashClassifier(TCL_ONE_WORD_KEYS) {
	}
protected:
	const char* hashkey(nsaddr_t, nsaddr_t, int fid) {
		return (const char*) fid;
	}
};

class DestHashClassifier : public HashClassifier {
public:
	DestHashClassifier() : HashClassifier(TCL_ONE_WORD_KEYS) {}
protected:
	const char* hashkey(nsaddr_t, nsaddr_t dst, int) {
		return (const char*) mshift(dst);
	}
};

/**************  TCL linkage ****************/
static class SrcDestHashClassifierClass : public TclClass {
public:
	SrcDestHashClassifierClass() : TclClass("Classifier/Hash/SrcDest") {}
	TclObject* create(int, const char*const*) {
		return new SrcDestHashClassifier;
	}
} class_hash_srcdest_classifier;

static class FidHashClassifierClass : public TclClass {
public:
	FidHashClassifierClass() : TclClass("Classifier/Hash/Fid") {}
	TclObject* create(int, const char*const*) {
		return new FidHashClassifier;
	}
} class_hash_fid_classifier;

static class DestHashClassifierClass : public TclClass {
public:
	DestHashClassifierClass() : TclClass("Classifier/Hash/Dest") {}
	TclObject* create(int, const char*const*) {
		return new DestHashClassifier;
	}
} class_hash_dest_classifier;

static class SrcDestFidHashClassifierClass : public TclClass {
public:
	SrcDestFidHashClassifierClass() : TclClass("Classifier/Hash/SrcDestFid") {}
	TclObject* create(int, const char*const*) {
		return new SrcDestFidHashClassifier;
	}
} class_hash_srcdestfid_classifier;







