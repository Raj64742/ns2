// 
// tools.cc      : Implements a few utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.cc,v 1.2 2001/11/20 22:28:17 haldar Exp $
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

#include "tools.hh"

#ifdef NS_DIFFUSION
#include "scheduler.h"
#include "random.h"
#endif

int global_debug_level = DEFAULT_DEBUG_LEVEL;

void getTime(struct timeval *tmv)
{
#ifdef NS_DIFFUSION
  tmv->tv_sec = (long)Scheduler::instance().clock();
  tmv->tv_usec = 0; //using an arbitrary value as this is used as seed for random generators;
#else
  gettimeofday(tmv, NULL);
#endif //ns_diffusion

}

void getSeed(struct timeval tv) 
{
#ifdef NS_DIFFUSION
  double seed = Random::seed_heuristically();
  srand(seed);
#else
  srand(tv.tv_usec);
#endif
}

void diffPrint(int msg_level, char *msg)
{
  if (global_debug_level >= msg_level){
    // Print message
    fprintf(stderr, msg);
    fflush(stderr);
  }
}
#endif // NS
