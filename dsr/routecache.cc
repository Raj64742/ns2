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
/* routecache.cc

   handles routes
*/

extern "C" {
#include <stdio.h>
#include <stdarg.h>
};

#undef DEBUG

#include "path.h"
#include "routecache.h"
#include <god.h>

static const int verbose_debug = 0;

/*===============================================================
  OTcl definition
----------------------------------------------------------------*/
static class RouteCacheClass : public TclClass {
public:
        RouteCacheClass() : TclClass("RouteCache") {}
        TclObject* create(int, const char*const*) {
                return makeRouteCache();
        }
} class_RouteCache;

/*===============================================================
 Constructors
----------------------------------------------------------------*/
RouteCache::RouteCache():
#ifdef DSR_CACHE_STATS
	mh(this),
#endif
	MAC_id(invalid_addr), net_id(invalid_addr)
{
#ifdef DSR_CACHE_STATS
	stat.reset();
#endif
}

RouteCache::~RouteCache()
{
}

int
RouteCache::command(int argc, const char*const* argv)
{
  if(argc == 2)
    {
      if(strcasecmp(argv[1], "startdsr") == 0)
	{
#ifdef DSR_CACHE_STATS
	  mh.start();
#endif
	  return TCL_OK;
	}
    }
  else if(argc == 3) 
    {    
      if (strcasecmp(argv[1], "ip-addr") == 0) {
	net_id = ID(atoi(argv[2]), ::IP);
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "mac-addr") == 0) {
	MAC_id = ID(atoi(argv[2]), ::MAC);
      return TCL_OK;
      }
      
      /* must be a looker up */
      TclObject *obj = (TclObject*) TclObject::lookup(argv[2]);
      if(obj == 0)
	return TCL_ERROR;
      
      if(strcasecmp(argv[1], "log-target") == 0 || 
	 strcasecmp(argv[1], "tracetarget") == 0) {
	logtarget = (Trace*) obj;
	return TCL_OK;
      }

      return TCL_ERROR;
    }
  
  return TclObject::command(argc, argv);
}

void
RouteCache::trace(char* fmt, ...)
{
  va_list ap;
  
  if (!logtarget) return;

  va_start(ap, fmt);
  vsprintf(logtarget->pt_->buffer(), fmt, ap);
  logtarget->pt_->dump();
  va_end(ap);
}


void
RouteCache::dump(FILE *out)
{
  fprintf(out, "Route cache dump unimplemented\n");
  fflush(out);
}

///////////////////////////////////////////////////////////////////////////

extern bool cache_ignore_hints;
extern bool cache_use_overheard_routes;

#define STOP_PROCESSING 0
#define CONT_PROCESSING 1

int
RouteCache::pre_addRoute(const Path& route, Path& rt,
			 Time t, const ID& who_from)
{
  assert(!(net_id == invalid_addr));
  
  if (route.length() < 2)
    return STOP_PROCESSING; // we laugh in your face

  if(verbose_debug)
    trace("SRC %.9f _%s_ adding rt %s from %s",
	  Scheduler::instance().clock(), net_id.dump(),
	  route.dump(), who_from.dump());

  if (route[0] != net_id && route[0] != MAC_id) 
    {
      fprintf(stderr,"%.9f _%s_ adding bad route to cache %s %s\n",
	      t, net_id.dump(), who_from.dump(), route.dump());
      return STOP_PROCESSING;
    }
	     
  rt = (Path&) route;	// cast away const Path&
  rt.owner() = who_from;

  Link_Type kind = LT_TESTED;
  for (int c = 0; c < rt.length(); c++)
    {
      rt[c].log_stat = LS_UNLOGGED;
      if (rt[c] == who_from) kind = LT_UNTESTED; // remaining ids came from $
      rt[c].link_type = kind;
      rt[c].t = t;
    }

  return CONT_PROCESSING;
}

int
RouteCache::pre_noticeRouteUsed(const Path& p, Path& stub,
				Time t, const ID& who_from)
{
  int c;
  bool first_tested = true;

  if (p.length() < 2)
	  return STOP_PROCESSING;
  if (cache_ignore_hints == true)
	  return STOP_PROCESSING;

  for (c = 0; c < p.length() ; c++) {
	  if (p[c] == net_id || p[c] == MAC_id) break;
  }

  if (c == p.length() - 1)
	  return STOP_PROCESSING; // path contains only us

  if (c == p.length()) { // we aren't in the path...
	  if (cache_use_overheard_routes) {
		  // assume a link from us to the person who
		  // transmitted the packet
		  if (p.index() == 0) {
			   /* must be a route request */
			  return STOP_PROCESSING;
		  }

		  stub.reset();
		  stub.appendToPath(net_id);
		  int i = p.index() - 1;
		  for ( ; i < p.length() && !stub.full() ; i++) {
			  stub.appendToPath(p[i]);
		  }
		  // link to xmiter might be unidirectional
		  first_tested = false;
	  }
	  else {
		  return STOP_PROCESSING;
	  }
  }
  else { // we are in the path, extract the subpath
	  CopyIntoPath(stub, p, c, p.length() - 1);
  }

  Link_Type kind = LT_TESTED;
  for (c = 0; c < stub.length(); c++) {
	  stub[c].log_stat = LS_UNLOGGED;

	   // remaining ids came from $
	  if (stub[c] == who_from)
		  kind = LT_UNTESTED;
	  stub[c].link_type = kind;
	  stub[c].t = t;
  }
  if (first_tested == false)
	  stub[0].link_type = LT_UNTESTED;

  return CONT_PROCESSING;
}

#undef STOP_PROCESSING
#undef CONT_PROCESSING

//////////////////////////////////////////////////////////////////////

#ifdef DSR_CACHE_STATS

void
MobiHandler::handle(Event *) {
        cache->periodic_checkCache();
        Scheduler::instance().schedule(this, &intr, interval);
}

int
RouteCache::checkRoute_logall(Path *p, int action, int start)
{
  int c;
  int subroute_bad_count = 0;

  if(p->length() == 0)
    return 0;
  assert(p->length() >= 2);

  assert(action == ACTION_DEAD_LINK ||
         action == ACTION_EVICT ||
         action == ACTION_FIND_ROUTE);

  for (c = start; c < p->length() - 1; c++)
    {
      if (God::instance()->hops((*p)[c].getNSAddr_t(), (*p)[c+1].getNSAddr_t()) != 1)
	{
          trace("SRC %.9f _%s_ %s [%d %d] %s->%s dead %d %.9f",
                Scheduler::instance().clock(), net_id.dump(),
                action_name[action], p->length(), c,
                (*p)[c].dump(), (*p)[c+1].dump(),
                (*p)[c].link_type, (*p)[c].t);

          if(subroute_bad_count == 0)
            subroute_bad_count = p->length() - c - 1;
	}
    }
  return subroute_bad_count;
}

#endif /* DSR_CACHE_STAT */

