/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * classifier-mpath.cc
 *
 * Copyright (C) 1997 by USC/ISI
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/classifier-mpath.cc,v 1.9 2001/12/20 00:15:34 haldar Exp $ (USC/ISI)";
#endif

#include "classifier.h"

class MultiPathForwarder : public Classifier {
public:
	MultiPathForwarder() : ns_(0) {} 
	virtual int classify(Packet*) {
		int cl;
		int fail = ns_;
		do {
			cl = ns_++;
			ns_ %= (maxslot_ + 1);
		} while (slot_[cl] == 0 && ns_ != fail);
		return cl;
	}
private:
	int ns_;
};

static class MultiPathClass : public TclClass {
public:
	MultiPathClass() : TclClass("Classifier/MultiPath") {} 
	TclObject* create(int, const char*const*) {
		return (new MultiPathForwarder());
	}
} class_multipath;
