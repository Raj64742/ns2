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
 */

/*
 * edge.h
 */

#ifndef edge_h
#define edge_h

#include "dsred.h"
#include "dsPolicy.h"


/*------------------------------------------------------------------------------
    This class specifies the characteristics of an edge router in the Diffserv 
architecture.  Its position in the class hierarchy is shown in "dsred.h", the
header file for its parent class, dsREDQueue.
------------------------------------------------------------------------------*/
class edgeQueue : public dsREDQueue {
public:	
	edgeQueue();
	int command(int argc, const char*const* argv);	// interface to ns scripts

protected:
	Policy policy;
	void enque(Packet *pkt);	// edge mechanism to enque a packet
};

#endif

