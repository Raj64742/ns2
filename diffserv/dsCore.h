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
 * core.h
 */

/* 
 * Integrated into ns on Nov 01, 2000, by Xuan Chen (xuanc@isi.edu)  
 * Rename as dsCore.h
 *
 */

#ifndef DS_CORE_H
#define DS_CORE_H
#include "dsred.h"


/*------------------------------------------------------------------------------
class coreQueue 
    This class specifies the characteristics for the core router.
------------------------------------------------------------------------------*/
class coreQueue : public dsREDQueue {
public:	
	coreQueue();
	int command(int argc, const char*const* argv);

protected:

};

#endif

