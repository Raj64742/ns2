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
#include "parentnode.h"

class BcastAddressClassifier;

static class LanNodeClass : public TclClass {
public:
	LanNodeClass() : TclClass("LanNode") {}
	TclObject* create(int, const char*const*) {
                return (new LanNode);
        }
} class_lan_node;

static class AbsLanNodeClass : public TclClass {
public:
	AbsLanNodeClass() : TclClass("AbsLanNode") {}
	TclObject* create(int, const char*const*) {
                return (new AbsLanNode);
        }
} class_abslan_node;

static class BroadcastNodeClass : public TclClass {
public:
	BroadcastNodeClass() : TclClass("Node/Broadcast") {}
	TclObject* create(int, const char*const*) {
                return (new BroadcastNode);
        }
} class_broadcast_node;

int LanNode::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = atoi(argv[2]);
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

int AbsLanNode::command(int argc, const char*const* argv) {
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

int BroadcastNode::command(int argc, const char*const* argv) {
  Tcl& tcl = Tcl::instance();
  if (argc == 3) {
    if (strcmp(argv[1], "addr") == 0) {
      address_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } 
    else if (strcmp(argv[1], "nodeid") == 0) {
      nodeid_ = Address::instance().str2addr(argv[2]);
      return TCL_OK;
    } 
    else if (strcmp(argv[1], "attach-classifier") == 0) {
      classifier_ = (BcastAddressClassifier*)(TclObject::lookup(argv[2]));
      if (classifier_ == NULL) {
	tcl.add_errorf("Wrong object name %s",argv[2]);
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return ParentNode::command(argc,argv);
}

void BroadcastNode::add_route(char *dst, NsObject *target) {
  classifier_->do_install(dst,target);
}

void BroadcastNode::delete_route(char *dst, NsObject *nullagent) {
  classifier_->do_install(dst, nullagent); 
}
