/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * classifier-mcast.h
 *
 * Copyright (C) 1999 by USC/ISI
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

#ifndef ns_classifier_mcast_h
#define ns_classifier_mcast_h

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class MCastClassifier : public Classifier {
public:
	MCastClassifier();
	~MCastClassifier();
	static const char STARSYM[]; //"source" field for shared trees
protected:
	virtual int command(int argc, const char*const* argv);
	virtual int classify(Packet *p);
	int findslot();
	enum {HASHSIZE = 256};
	struct hashnode {
		int slot;
		nsaddr_t src;
		nsaddr_t dst;
		hashnode* next;
		int iif; // for RPF checking
	};
	int hash(nsaddr_t src, nsaddr_t dst) const {
		u_int32_t s = src ^ dst;
		s ^= s >> 16;
		s ^= s >> 8;
		return (s & 0xff);
	}
	hashnode* ht_[HASHSIZE];
	hashnode* ht_star_[HASHSIZE]; // for search by group only (not <s,g>)

	void set_hash(hashnode* ht[], nsaddr_t src, nsaddr_t dst,
		      int slot, int iface);
	void clearAll();
	void clearHash(hashnode* h[], int size);
	hashnode* lookup(nsaddr_t src, nsaddr_t dst,
			 int iface = iface_literal::ANY_IFACE) const;
	hashnode* lookup_star(nsaddr_t dst,
			      int iface = iface_literal::ANY_IFACE) const;
	void change_iface(nsaddr_t src, nsaddr_t dst,
			  int oldiface, int newiface);
	void change_iface(nsaddr_t dst,
			  int oldiface, int newiface);
};
#endif
