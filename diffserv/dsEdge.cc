/*
 * Copyright (c) 2000, Nortel Networks.
 * All rights reserved.
 *
 * License is granted to copy, to use, to make and to use derivative
 * works for research and evaluation purposes.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 *
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 *
 * edge.cc
 *
 */

/* 
 * Integrated into ns main distribution by Xuan Chen (xuanc@isi.edu)
 * 1. rename to dsEdge.cc
 * 2. Reorganized to use the new policy style.
 */

#include "dsEdge.h"
#include "dsPolicy.h"
#include "packet.h"
#include "tcp.h"
#include "random.h"


/*------------------------------------------------------------------------------
class edgeClass 
------------------------------------------------------------------------------*/
static class edgeClass : public TclClass {
public:
	edgeClass() : TclClass("Queue/dsRED/edge") {}
	TclObject* create(int, const char*const*) {
		return (new edgeQueue);
	}
} class_edge;


/*------------------------------------------------------------------------------
edgeQueue() Constructor.
------------------------------------------------------------------------------*/
edgeQueue::edgeQueue() {
  policy = NULL;
}


/*------------------------------------------------------------------------------
void enque(Packet* pkt) 
Post: The incoming packet pointed to by pkt is marked with an appropriate code
  point (as handled by the marking method) and enqueued in the physical and 
  virtual queue corresponding to that code point (as specified in the PHB 
  Table).
Uses: Methods Policy::mark(), lookupPHBTable(), and redQueue::enque(). 
------------------------------------------------------------------------------*/
void edgeQueue::enque(Packet* pkt) {
	int codePt;

	// Mark the packet with the specified priority:
	//printf("before ,mark\n");
	codePt = policy->mark(pkt);
	//	printf("after ,mark\n");
	dsREDQueue::enque(pkt);
}


/*------------------------------------------------------------------------------
int command(int argc, const char*const* argv) 
    Commands from the ns file are interpreted through this interface.
------------------------------------------------------------------------------*/
int edgeQueue::command(int argc, const char*const* argv) {
  if (strcmp(argv[1], "addPolicyEntry") == 0) {
    // Now, edge router needs to decide what kind of policy it wants.
    if (!policy) {
      if (strcmp(argv[4], "Dumb") == 0)
	policy = new DumbPolicy;
      else if (strcmp(argv[4], "TSW2CM") == 0)
	policy = new TSW2CMPolicy;
      else if (strcmp(argv[4], "TSW3CM") == 0)
	policy = new TSW3CMPolicy;
      else if (strcmp(argv[4], "TokenBucket") == 0)
	  policy = new TBPolicy;
      else if (strcmp(argv[4], "srTCM") == 0)
	    policy = new SRTCMPolicy;
      else if (strcmp(argv[4], "trTCM") == 0)
	      policy = new TRTCMPolicy;
      else if (strcmp(argv[4], "FW") == 0)
	policy = new FWPolicy;
      else {
	printf("No applicable policy specified, exit!!!");
	exit(-1);
      }
    };

    policy->addPolicyEntry(argc, argv);
    return(TCL_OK);
  }
  if (strcmp(argv[1], "addPolicerEntry") == 0) {
    if (policy)
      policy->addPolicerEntry(argc, argv);
    return(TCL_OK);
  }
  if (strcmp(argv[1], "getCBucket") == 0) {
    Tcl& tcl = Tcl::instance();
    if (policy)
      tcl.resultf("%f", policy->getCBucket(argv));
    return(TCL_OK);
  }
  if (strcmp(argv[1], "printPolicyTable") == 0) {
    if (policy)
      policy->printPolicyTable();
    return(TCL_OK);
  }
  if (strcmp(argv[1], "printPolicerTable") == 0) {
    if (policy)
      policy->printPolicerTable();
    return(TCL_OK);
  }
  
  return(dsREDQueue::command(argc, argv));
};


