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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier-mcast.cc,v 1.13.2.5 1998/10/02 18:18:49 kannan Exp $";
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
        static const int HASHSIZE= 256;
	struct hashnode {
		int slot;
		nsaddr_t src;
		nsaddr_t dst;
		hashnode* next;
                int iif; // for RPF checking
	};
	int command(int argc, const char*const* argv);
	int classify(Packet *const p);
	int findslot();
	void set_hash(hashnode *ht[], nsaddr_t src, nsaddr_t dst, int slot, int iface);
	int hash(nsaddr_t src, nsaddr_t dst) const {
		u_int32_t s = src ^ dst;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s & 0xff);
	}
        void clearHash(hashnode* h[], int size);
        void clearAll();
	hashnode* ht_[HASHSIZE];
    	hashnode* ht_star_[HASHSIZE]; //for search by group only (not <s,g>)

	hashnode* const lookup(nsaddr_t src, nsaddr_t dst, int iface = ANY_IFACE.value()) const;
	hashnode* const lookup_star(nsaddr_t dst, int iface = ANY_IFACE.value()) const;
    
        void change_iface(nsaddr_t src, nsaddr_t dst, int oldiface, int newiface);
        void change_iface(nsaddr_t dst, int oldiface, int newiface);
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
	memset(ht_star_, 0, sizeof(ht_star_));
}

MCastClassifier::~MCastClassifier()
{
	clearAll();
}

void MCastClassifier::clearHash (hashnode* h[], int size) 
{
        for (int i = 0; i < size; ++i) {
                hashnode* p = h[i];
                while (p != 0) {
                        hashnode* n = p->next;
                        delete p;
                        p = n;
                }
        }
        memset(h, 0, sizeof(hashnode [size]));
}

void MCastClassifier::clearAll()
{
        clearHash(ht_, HASHSIZE);
	clearHash(ht_star_, HASHSIZE);
}

MCastClassifier::hashnode* const
MCastClassifier::lookup(nsaddr_t src, nsaddr_t dst, int iface = ANY_IFACE.value()) const
{
	int h = hash(src, dst);
	hashnode* p;
	for (p = ht_[h]; p != 0; p = p->next) {
		if (p->src == src && p->dst == dst)
			if (p->iif == iface ||
			    p->iif == UNKN_IFACE.value() ||
			    iface == ANY_IFACE.value())
				break;
	}
	return (p);
}

MCastClassifier::hashnode* const
MCastClassifier::lookup_star(nsaddr_t dst, int iface = ANY_IFACE.value()) const
{
	int h = hash(0, dst);
	hashnode* p;
	for (p = ht_star_[h]; p != 0; p = p->next) {
		if (p->dst == dst && p->iif == iface)
		       break;
	}
	return (p);
}

int MCastClassifier::classify(Packet *const pkt)
{
	hdr_cmn* h = (hdr_cmn*)pkt->access(off_cmn_);
	hdr_ip* ih = (hdr_ip*)pkt->access(off_ip_);

	nsaddr_t src = ih->src() >> 8; /*XXX*/
	nsaddr_t dst = ih->dst();

	int iface = h->iface();

	// first lookup (S,G) - entries
	//	cout << "classifying...";
	hashnode* p = lookup(src, dst, iface);
	if (p == 0)
	        p = lookup_star(dst, iface);
	if (p == 0) {
		//		cout << "couldn't classify by iface...";
		p = lookup(src, dst);
		if (p==0) p = lookup_star(dst);
		if (p == 0) {
			//			cout << "couldn't classify at all\n";
			/*
			 * Didn't find an entry.
			 * Call tcl exactly once to install one.
			 * If tcl doesn't come through then fail.
			 */
			Tcl::instance().evalf("%s new-group %u %u %d cache-miss", 
					      name(), src, dst, iface);
			return -1;
		}
		if (p->iif == ANY_IFACE.value() || iface == UNKN_IFACE.value())
			return p->slot;

		Tcl::instance().evalf("%s new-group %u %u %d wrong-iif", 
				      name(), src, dst, iface);
		return -1;
	}
	return p->slot;
}

int MCastClassifier::findslot()
{
	int i;
	for (i = 0; i < nslot_; ++i)
		if (slot_[i] == 0)
			break;
	return (i);
}

void MCastClassifier::set_hash(hashnode *ht[], nsaddr_t src, nsaddr_t dst, int slot, int iface)
{
	int h = hash(src, dst);
	hashnode* p = new hashnode;
	p->src = src;
	p->dst = dst;
	p->slot = slot;
        p->iif = iface;
	p->next = ht[h];
	ht[h] = p;
}

int MCastClassifier::command(int argc, const char*const* argv)
{
	/*
	 * $classifier set-hash $src $group $slot $iif
	 *      $iif can be:(1) iif
	 *                  (2) "*" - matches any interface
	 *                  (3) "?" - interface is unknown (usually this 
	 *                            means that the packet came from a local
	 *                            (for this node) agent.
	 */
	if (argc == 6) {
		if (strcmp(argv[1], "set-hash") == 0) {
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int slot = atoi(argv[4]);
                        int iface = (strcmp(argv[5], ANY_IFACE.name())==0) ? ANY_IFACE.value() 
				: (strcmp(argv[5], UNKN_IFACE.name())==0) ? UNKN_IFACE.value() 
				: atoi(argv[5]); 
			if (strcmp("*", argv[2]) == 0) {
			    // install a <*,G> entry
			    set_hash(ht_star_, 0, dst, slot, iface);
			} else {
			    //install a <S,G> entry
			    set_hash(ht_, src, dst, slot, iface);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "change-iface") == 0) {
			nsaddr_t src = strtol(argv[2], (char**)0, 0);
			nsaddr_t dst = strtol(argv[3], (char**)0, 0);
			int oldiface = atoi(argv[4]);
                        int newiface = atoi(argv[5]);
			if (strcmp("*", argv[2]) == 0) {
			    // change interface for a <*,G>
			    change_iface(dst, oldiface, newiface);
			} else {
			    // change interface for a <S,G>
			    change_iface(src, dst, oldiface, newiface);
			}
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

			// source specific entries have higher precedence
			hashnode *p = lookup(src, dst, iface); 
			if (p == 0)
				p= lookup_star(dst, iface); // if they aren't found, lookup <*,G>
			if ((p == 0) || (slot_[p->slot] == 0))
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

void MCastClassifier::change_iface(nsaddr_t src, nsaddr_t dst, int oldiface, int newiface)
{
	hashnode* p = lookup(src, dst, oldiface);
	if (p) p->iif = newiface;
}

void MCastClassifier::change_iface(nsaddr_t dst, int oldiface, int newiface)
{
	hashnode* p = lookup_star(dst, oldiface);
	if (p) p->iif = newiface;
}
