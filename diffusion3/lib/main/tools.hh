// 
// tools.hh      : Other utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.hh,v 1.8 2002/09/16 17:57:29 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

//
// This file contains OS abstractions to make it easy to use diffusion
// in different environments (i.e. in simulations, where time is
// virtualized, and in embeddedd apps where error logging happens in
// some non-trivial way).
//
// This file defines the various debug levels and a global debug level
// variable (global_debug_level) that can be set according to how much
// debug information one would like to get when using DiffPrint (see
// below for further details).

#ifndef _TOOLS_HH_
#define _TOOLS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Defines the various debug levels
#define DEBUG_NEVER            11
#define DEBUG_LOTS_DETAILS     10
#define DEBUG_MORE_DETAILS      8
#define DEBUG_DETAILS           6
#define DEBUG_SOME_DETAILS      4
#define DEBUG_NO_DETAILS        3
#define DEBUG_IMPORTANT         2
#define DEBUG_ALWAYS            1

// Defines the default debug level
#ifdef NS_DIFFUSION
#define DEBUG_DEFAULT           0
#else
#define DEBUG_DEFAULT           1
#endif // NS_DIFFUSION

extern int global_debug_level;

// SetSeed sets the random number generator's seed with the timeval
// structure given in tv (which is not changed)
void SetSeed(struct timeval *tv);

// GetTime returns a timeval structure with the current system time
void GetTime(struct timeval *tv);

// GetRand returns a random number between 0 and RAND_MAX
int GetRand();

// DiffPrint can be used to print messages. In addition to take the
// message to be printed (using the variable argument list format,
// just like fprintf), DiffPrint also requires a debug level for this
// particular message. This is is compared to the global debug level
// and if it is below the current global debug level, the message is
// sent to the logging device (usually set to stderr).
void DiffPrint(int msg_debug_level, const char *fmt, ...);

#endif // !_TOOLS_HH_
