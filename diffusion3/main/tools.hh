// 
// tools.hh      : Other utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.hh,v 1.3 2001/12/11 23:21:45 haldar Exp $
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
// tools.hh contains OS abstractions to make it easy to use 
// diffusion in different environments (i.e. in simulations,
// where time is virtualized, and in embeddedd apps where
// error logging happens in some non-trivial way).
//

#ifndef tools_hh
#define tools_hh

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.hh"

extern int global_debug_level;
extern char *application_id;

#ifdef BBN_LOGGER
#include "Logger.h"

void InitMainLogger();
#endif // BBN_LOGGER

void getSeed(struct timeval *tv);
void getTime(struct timeval *tv);
int getRand();
void diffPrint(int msg_level, const char *fmt, ...);

#endif // tools_hh
