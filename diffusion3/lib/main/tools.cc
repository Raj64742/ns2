// 
// tools.cc      : Implements a few utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: tools.cc,v 1.12 2004/01/09 00:15:24 haldar Exp $
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
#include "rng.h"
#include "random.h"
#endif // NS_DIFFUSION

int global_debug_level = DEBUG_DEFAULT;

void GetTime(struct timeval *tv)
{
#ifdef NS_DIFFUSION
  double time;
  long sec, usec;

  time = Scheduler::instance().clock();
  sec = time;
  usec = (time - sec) * 1000000;
  tv->tv_sec = sec;
  tv->tv_usec = usec;
  DiffPrint(DEBUG_NEVER, "tv->sec = %ld, tv->usec = %ld\n", tv->tv_sec, tv->tv_usec);
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

void DiffPrintWithTime(int msg_debug_level, const char *fmt, ...)
{
  struct timeval tv;
  va_list ap;

  va_start(ap, fmt);

  if (global_debug_level >= msg_debug_level){
    // Get time
    GetTime(&tv);

    // Print Time
    fprintf(stderr, "%d.%06d : ", tv.tv_sec, tv.tv_usec);
    // Print message
    vfprintf(stderr, fmt, ap);
    fflush(NULL);
  }

  va_end(ap);
}
