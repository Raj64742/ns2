// 
// tools.hh      : Other utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.hh,v 1.2 2001/11/20 22:28:17 haldar Exp $
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

#ifdef NS_DIFFUSION

#ifndef tools_hh
#define tools_hh

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.hh"

void getTime(struct timeval *tmv);
void getSeed(struct timeval tv);
void diffPrint(int msg_level, char *msg);

#endif // tools_hh
#endif // NS
