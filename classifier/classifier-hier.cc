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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier-hier.cc,v 1.2 2000/09/15 20:31:56 haoboy Exp $

#include <assert.h>
#include "classifier.h"
#include "addr-params.h"

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
private:
	Classifier **clsfr_;
};

int HierClassifier::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "classifier") == 0) {
			// XXX Caller must guarantee that the supplied level
			// is within range, i.e., 0 < n <= level_
			tcl.resultf("%s", clsfr_[atoi(argv[2])-1]->name());
			return TCL_OK;
		} else if (strcmp(argv[1], "defaulttarget") == 0) {
			NsObject *o = (NsObject *)TclObject::lookup(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->set_default_target(o);
			return (TCL_OK);
		} else if (strcmp(argv[1], "clear") == 0) {
			int slot = atoi(argv[2]);
			for (int i = 0; 
			     i < AddrParamsClass::instance().hlevel(); i++)
				clsfr_[i]->clear(slot);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "add-classifier") == 0) {
			int n = atoi(argv[2]) - 1;
			Classifier *c=(Classifier*)TclObject::lookup(argv[3]);
			clsfr_[n] = c;
			return (TCL_OK);
		}
	}
	return Classifier::command(argc, argv);
}


static class HierClassifierClass : public TclClass {
public:
	HierClassifierClass() : TclClass("Classifier/Hier") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new HierClassifier());
	}
} class_hier_classifier;
