// 
// tools.cc      : Implements a few utility functions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: tools.cc,v 1.4 2001/12/13 22:29:11 haldar Exp $
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
int default_debug_level = DEBUG_DEFAULT;

char *application_id = NULL;

#ifdef BBN_LOGGER
Logger *log_general = NULL;

void InitMainLogger()
{
  // Init logger
  log_general = new Logger();

  // Configure and start logging
  log_general->initFromPropertiesFile(LOGGER_CONFIG_FILE);

  log_general->Go();
}
#endif // BBN_LOGGER

void getTime(struct timeval *tv)
{
#ifdef NS_DIFFUSION
  tv->tv_sec = (long)Scheduler::instance().clock();
  tv->tv_usec = 0; //using an arbitrary value as this is used as seed for random generators;
#else
  gettimeofday(tv, NULL);
#endif // NS_DIFFUSION
}

void getSeed(struct timeval *tv) 
{
#ifdef NS_DIFFUSION
  //double seed = Random::seed_heuristically();
  //srand(seed);
#else
  srand(tv->tv_usec);
#endif // NS_DIFFUSION
}

int getRand() 
{
#ifdef NS_DIFFUSION
  int num = Random::random();
  printf(".............num=%d.......\n", num);
  return num;
  //return (Random::random());
#else
  return (rand());
#endif // NS_DIFFUSION
}


void diffPrint(int msg_level, const char *fmt, ...)
{
#ifdef BBN_LOGGER
  char buf[LOGGER_BUFFER_SIZE];
  int retval;
#endif // BBN_LOGGER
  va_list ap;

  va_start(ap, fmt);

  if (global_debug_level >= msg_level){

#ifdef BBN_LOGGER
    // Log into BBN logger
    retval = vsnprintf(buf, LOGGER_BUFFER_SIZE, fmt, ap);
    if (log_general && application_id)
      log_general->LogMessage(application_id, buf);
#else
    // Print message
    vfprintf(stderr, fmt, ap);
    fflush(NULL);
#endif
  }

  va_end(ap);
}
