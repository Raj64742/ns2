/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

/* -*- c++ -*- 
   routecache.h
   $Id: routecache.h,v 1.5 1999/04/22 18:53:49 haldar Exp $

   Interface all route caches must support

*/

#ifndef _routecache_h
#define _routecache_h

extern "C" {
#include <stdarg.h>
};

#include "object.h"
#include "trace.h"

#include "path.h"

#ifdef DSR_CACHE_STATS
class RouteCache;
#include <dsr/cache_stats.h>
#endif

class RouteCache : public TclObject {

public:
  RouteCache();
  ~RouteCache();

  virtual void noticeDeadLink(const ID&from, const ID& to, Time t) = 0;
  // the link from->to isn't working anymore, purge routes containing
  // it from the cache

  virtual void noticeRouteUsed(const Path& route, Time t, 
			       const ID& who_from) = 0;
  // tell the cache about a route we saw being used
  // if first tested is set, then we assume the first link was recently
  // known to work

  virtual void addRoute(const Path& route, Time t, const ID& who_from) = 0;
  // add this route to the cache (presumably we did a route request
  // to find this route and don't want to lose it)

  virtual bool findRoute(ID dest, Path& route, int for_use) = 0;
  // if there is a cached path from us to dest returns true and fills in
  // the route accordingly. returns false otherwise
  // if for_use, then we assume that the node really wants to keep 
  // the returned route

  virtual int command(int argc, const char*const* argv);
  void trace(char* fmt, ...);

///////////////////////////////////////////////////////////////////////////

  int pre_addRoute(const Path& route, Path &rt,
		   Time t, const ID& who_from);
  // returns 1 if the Route should be added to the cache, 0 otherwise.

  int pre_noticeRouteUsed(const Path& p, Path& stub,
			  Time t, const ID& who_from);
  // returns 1 if the Route should be added to the cache, 0 otherwise.

#ifdef DSR_CACHE_STATS
  MobiHandler mh;
  struct cache_stats stat;

  virtual void periodic_checkCache() = 0;
  int checkRoute_logall(Path *p, int action, int start);
#endif


///////////////////////////////////////////////////////////////////////////

  ID MAC_id, net_id;
  virtual void dump(FILE *out);

protected:
  Trace *logtarget;
};

RouteCache * makeRouteCache();
// return a ref to the route cache that should be used in this sim

#endif // _routecache_h
