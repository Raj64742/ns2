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

#ifndef ns_parentnode_h
#define ns_parentnode_h

#include "address.h"
#include "classifier-addr.h"
#include "rtmodule.h"

class NsObject;

/* Class ParentNode : from which all node types including Node, LanNode etc evolve */

class ParentNode : public TclObject {
public:
	ParentNode() : nodeid_(-1), address_(-1) {} 
	/*virtual int command(int argc, const char*const* argv) {}*/
	virtual inline int address() { return address_;}
	virtual inline int nodeid() { return nodeid_;}
	virtual void add_route (char *, NsObject *) {}
	virtual void delete_route (char *, NsObject *) {}
	virtual void set_table_size(int nn) {}
	virtual void set_table_size(int lev, int nn) {}
protected:
  int nodeid_;
  int address_;
};


/* LanNode: Lan implementation as a virtual node: 
   LanNode mimics a real
   node and uses an address (id) from Node's address space */

class LanNode : public ParentNode {
public:
  LanNode() : ParentNode() {} 
  virtual int command(int argc, const char*const* argv);
};


/* AbsLanNode:
   It create an abstract LAN.
   An abstract lan is one in which the complex CSMA/CD 
   contention mechanism is replaced by a simple DropTail 
   queue mechanism. */

class AbsLanNode : public ParentNode {
public:
  AbsLanNode() : ParentNode() {} 
  virtual int command(int argc, const char*const* argv);
};


/* Node/Broadcast: used for sun's implementation of MIP;
   why is this version of MIP used when (CMU's) wireless 
   version is also available?? doesn't make sense to have both;
*/

class BroadcastNode : public ParentNode {
public:
	BroadcastNode() : ParentNode(), classifier_(NULL) {}
	virtual int command(int argc, const char*const* argv);
	virtual void add_route (char *dst, NsObject *target);
	virtual void delete_route (char *dst, NsObject *nullagent);
	//virtual void set_table_size(int nn) {}
	//virtual void set_table_size(int lev, int nn) {}
private:
  BcastAddressClassifier *classifier_;
};
  
#endif /* ---ns_lannode_h */

