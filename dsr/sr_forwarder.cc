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
   
   sr_forwarder.cc
   source route forwarder

   */


#include <object.h>
#include <packet.h>
#include <ip.h>
#include <mac.h>
#include "hdr_sr.h"
#include "dsr_proto.h"
#include "routecache.h"
#include "requesttable.h"
#include "sr_forwarder.h"

/*===========================================================================
  static classes for OTcl
---------------------------------------------------------------------------*/
static class SRForwarderClass : public TclClass {
public:
  SRForwarderClass() : TclClass("SRForwarder") {}
  TclObject* create(int, const char*const*) {
    return (new SRForwarder);
  }
} class_srforwarder;


/*===========================================================================
  Member functions
---------------------------------------------------------------------------*/
SRForwarder::SRForwarder()
{
  target_ = 0;
  route_agent_ = 0;
  tracetarget = 0;
  
  bind("off_sr_", &off_sr_);
  bind("off_ll_", &off_ll_);
  bind("off_mac_", &off_mac_);
  bind("off_ip_", &off_ip_);

}

int
SRForwarder::command(int argc, const char*const* argv)
{
  Tcl& tcl = Tcl::instance();

  if (argc == 2) 
    {
      if (strcmp(argv[1], "target") == 0) {
	if (target_ != 0)
	  tcl.result(target_->name());
	return (TCL_OK);
      }
      if (strcmp(argv[1], "routeagent") == 0) {
	if (route_agent_ != 0)
	  tcl.result(route_agent_->name());
	return (TCL_OK);
      }
    }
  else if(argc == 3) 
    {
      TclObject *obj;
      
      if( (obj = TclObject::lookup(argv[2])) == 0) {
	fprintf(stderr, "SRForwarder: %s lookup of %s failed\n", argv[1],
		argv[2]);
	return TCL_ERROR;
      }

      if (strcasecmp(argv[1], "tracetarget") == 0) 
	{
	  tracetarget = (Trace*) obj;
	  return TCL_OK;
	}
      else if (strcasecmp(argv[1], "routecache") == 0) 
	{
	  routecache_ = (RouteCache*) obj;
	  return TCL_OK;
	}
      else if (strcasecmp(argv[1], "target") == 0) 
	{
	  target_ = (NsObject*) obj;
	  return TCL_OK;
	}
      else if (strcasecmp(argv[1], "routeagent") == 0) 
	{
	  route_agent_ = (DSRProto*) obj;
	  return TCL_OK;
	}
    }
  
  return NsObject::command(argc, argv);
}

void
SRForwarder::handlePktWithoutSR(Packet *p)
{
  hdr_sr *srh = hdr_sr::access(p);
  hdr_ip *iph = hdr_ip::access(p);
  //hdr_cmn *cmh =  hdr_cmn::access(p);
  Path path;

  /* check route cache for source route for this packet */
  //if (routecache_->findRoute(ID(iph->daddr()>>8,::IP),path))
  if (routecache_->findRoute(ID(iph->daddr(),::IP),path,0))
    {
      srh->init();
      path.fillSR(srh);
      sprintf(tracetarget->pt_->buffer(),
	      "$hit %g originating 0x%x -> 0x%x %s", 
	      Scheduler::instance().clock(),
	      iph->saddr(), iph->daddr(), srh->dump());
      tracetarget->pt_->dump();
      this->recv(p,0);		// send back to forwarder to send this out
      return;
    }
  
  /* if not found, give packet to routing protocol */
  sprintf(tracetarget->pt_->buffer(),
	  "dfu %g srforwarder: %s no route to 0x%x", 
	  Scheduler::instance().clock(),
	  routecache_->net_id.dump(), iph->daddr());
  tracetarget->pt_->dump();

  route_agent_->noRouteForPacket(p);
}

void
SRForwarder::recv(Packet* p, Handler* callback)
{
  hdr_sr *srh =  (hdr_sr*)p->access(off_sr_);

  assert(tracetarget != 0);

  if (!srh->valid())
    {
      handlePktWithoutSR(p);
      return;
    }

  /* forward as per source route */
  hdr_ip *iph = hdr_ip::access(p);
  hdr_cmn *cmnh =  hdr_cmn::access(p);
  
  if (srh->route_request())
    { /* just broadcast */
      cmnh->next_hop() = MAC_BROADCAST;
      cmnh->addr_type() = NS_AF_ILINK;
      target_->recv(p, (Handler*)0);
      return;	  
    }
  
  /* it must be a normal data packet with a source route */
  if (tracetarget)
    {
      sprintf(tracetarget->pt_->buffer(),
	      "F %g 0x%x -> 0x%x via 0x%x %s", 
	      Scheduler::instance().clock(),
	      iph->saddr(), iph->daddr(), srh->get_next_addr(), srh->dump());
      tracetarget->pt_->dump();
    }
  
  cmnh->next_hop() = srh->get_next_addr();
  cmnh->addr_type() = NS_AF_INET;
  srh->cur_addr() = srh->cur_addr() + 1;
  target_->recv(p, (Handler*)0);
  return;
}
