/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 1997 by the University of Southern California
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
// utilities.h 
//      Miscellaneous useful definitions, including debugging routines.
// 
// Author:
//   Mohit Talwar (mohit@catarina.usc.edu)
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/rap/utilities.h,v 1.4 1999/06/15 20:06:48 salehi Exp $

#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include <memory.h>

// Constants...

#ifndef NAME_MAX
// Maximum length of a file name
// In case that it's not defined in limits.h
#define NAME_MAX 14
#endif

// Definitions...

#define and &&
#define or ||
#define is ==

// Types...

typedef void (*VoidFunctionPtr)(int arg); 

// Headers...

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "random.h"
#include "raplist.h"

// Globals...

// Functions...

extern FILE * DebugEnable(unsigned int nodeid);
// Print debug message if flag is enabled
extern void Debug (int debugFlag, FILE *log, char* format, ...); 

#endif // UTILITIES_H
