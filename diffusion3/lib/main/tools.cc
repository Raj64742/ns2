// 
// tools.cc      : Implements a few utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.cc,v 1.5 2002/09/16 17:57:29 haldar Exp $
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

#include "tools.hh"

#ifdef NS_DIFFUSION
#include "scheduler.h"
#include "random.h"
#endif // NS_DIFFUSION

int global_debug_level = DEBUG_DEFAULT;

void GetTime(struct timeval *tv)
{
#ifdef NS_DIFFUSION
  double time;
  int sec, usec;

  time = Scheduler::instance().clock();
  sec = (int) (time * 1000000) / 1000000;
  usec = (int) (time * 1000000) % 1000000;
  tv->tv_sec = sec;
  tv->tv_usec = usec;
  //  printf("tv->sec = %d, tv->usec = %d\n", tv->tv_sec, tv->tv_usec);
#else
  gettimeofday(tv, NULL);
#endif // NS_DIFFUSION
}

void SetSeed(struct timeval *tv) 
{
#ifdef NS_DIFFUSION
  // Don't need to do anything since NS's RNG is seeded using
  // otcl proc ns-random <seed>
#else
  srand(tv->tv_usec);
#endif // NS_DIFFUSION
}

int GetRand()
{
#ifdef NS_DIFFUSION
  return (Random::random());
#else
  return (rand());
#endif // NS_DIFFUSION
}

void DiffPrint(int msg_debug_level, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  if (global_debug_level >= msg_debug_level){
    // Print message
    vfprintf(stderr, fmt, ap);
    fflush(NULL);
  }

  va_end(ap);
}
