// Copyright (c) 2000 by the University of Southern California
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
// Ported from CMU/Monarch's code, appropriate copyright applies.  
/* -*- c++ -*- 
   routecache.h

   Interface all route caches must support

*/

#ifndef _routecache_h
#define _routecache_h

extern "C" {
#include <stdarg.h>
};

#include <object.h>
#include <trace.h>

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

  // *******************************************************

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


  //**********************************************************

  ID MAC_id, net_id;
  virtual void dump(FILE *out);

protected:
  Trace *logtarget;
};

RouteCache * makeRouteCache();
// return a ref to the route cache that should be used in this sim

#endif // _routecache_h
