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

/* routecache.cc
   $Id: routecache.cc,v 1.4 2001/05/21 19:27:33 haldar Exp $

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
      
      if(strcasecmp(argv[1], "log-target") == 0 ||\
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

