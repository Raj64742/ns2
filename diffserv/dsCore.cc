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
 * core.cc
 */

/*
 * Integrated into ns (Nov 01, 2000) by Xuan Chen (xuanc@isi.edu) 
 * rename to dsCore.cc
 */

#include "dsCore.h"


/*------------------------------------------------------------------------------
class coreClass 
------------------------------------------------------------------------------*/
static class coreClass : public TclClass {
public:
	coreClass() : TclClass("Queue/dsRED/core") {}
	TclObject* create(int, const char*const*) {
		return (new coreQueue);
	}
} class_core;


/*------------------------------------------------------------------------------
coreQueue() Constructor.
------------------------------------------------------------------------------*/
coreQueue::coreQueue() {

}



/*------------------------------------------------------------------------------
int command(int argc, const char*const* argv) 
    Commands from the ns file are interpreted through this interface.
------------------------------------------------------------------------------*/
int coreQueue::command(int argc, const char*const* argv) {


	return(dsREDQueue::command(argc, argv));
}
