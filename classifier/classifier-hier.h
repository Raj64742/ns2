// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:52:53 haoboy>
//
// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Hierarchical classifier: a wrapper for hierarchical routing
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier-hier.h,v 1.5 2002/01/25 20:22:16 haldar Exp $

#include <assert.h>
#include "classifier.h"
#include "addr-params.h"
#include "route.h"

class HierClassifier : public Classifier {
public:
	HierClassifier() : Classifier() {
		clsfr_ = new Classifier*[AddrParamsClass::instance().hlevel()];
	}
	virtual ~HierClassifier() {
		// Deletion of contents (classifiers) is done in otcl
		delete []clsfr_;
	}
	inline virtual void recv(Packet *p, Handler *h) {
		clsfr_[0]->recv(p, h);
	}
	virtual int command(int argc, const char*const* argv);
	virtual void do_install(char *dst, NsObject *target);
	void set_table_size(int level, int csize);
private:
	Classifier **clsfr_;
};

