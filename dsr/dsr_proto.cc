
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

   dsr_proto.cc
   the DSR routing protocol agent
   
   */


#include <packet.h>
#include <object.h>
#include <agent.h>
#include <trace.h>
#include <ip.h>
#include "routecache.h"
#include "dsr_proto.h"

static class DSRProtoClass : public TclClass {
public:
        DSRProtoClass() : TclClass("Agent/DSRProto") {}
        TclObject* create(int, const char*const*) {
                return (new DSRProto);
        }
} class_DSRProto;


DSRProto::DSRProto() : Agent(PT_DSR)
{
  // dst_ = (IP_BROADCAST << 8) | RT_PORT;
  dst_.addr_ = IP_BROADCAST;
  dst_.port_ = RT_PORT;
}


void
DSRProto::recv(Packet* p, Handler* callback)
{
  
}

void
DSRProto::testinit()
{
  //struct hdr_sr hsr;
  hdr_sr hsr;
  
  if (net_id == ID(1,::IP))
    {
      printf("adding route to 1\n");
      hsr.init();
      hsr.append_addr( 1<<8, NS_AF_INET );
      hsr.append_addr( 2<<8, NS_AF_INET );
      hsr.append_addr( 3<<8, NS_AF_INET );
      hsr.append_addr( 4<<8, NS_AF_INET );
      
      routecache->addRoute(Path(hsr.addrs(), hsr.num_addrs()), 0.0, ID(1,::IP));
    }
  
  if (net_id == ID(3,::IP))
    {
      printf("adding route to 3\n");
      hsr.init();
      hsr.append_addr( 3<<8, NS_AF_INET );
      hsr.append_addr( 2<<8, NS_AF_INET );
      hsr.append_addr( 1<<8, NS_AF_INET );
      
      routecache->addRoute(Path(hsr.addrs(), hsr.num_addrs()), 0.0, ID(3,::IP));
    }
}


int
DSRProto::command(int argc, const char*const* argv)
{
  if(argc == 2) 
    {
      if (strcasecmp(argv[1], "testinit") == 0)
	{
	  testinit();
	  return TCL_OK;
	}
    }

  if(argc == 3) 
    {
      if(strcasecmp(argv[1], "tracetarget") == 0) {
	tracetarget = (Trace*) TclObject::lookup(argv[2]);
	if(tracetarget == 0)
	  return TCL_ERROR;
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "routecache") == 0) {
	routecache = (RouteCache*) TclObject::lookup(argv[2]);
	if(routecache == 0)
	  return TCL_ERROR;
	return TCL_OK;
      }
      else if (strcasecmp(argv[1], "ip-addr") == 0) {
	net_id = ID(atoi(argv[2]), ::IP);
	return TCL_OK;
      }
      else if(strcasecmp(argv[1], "mac-addr") == 0) {
	mac_id = ID(atoi(argv[2]), ::MAC);
      return TCL_OK;

      }
    }
  return Agent::command(argc, argv);
}

void 
DSRProto::noRouteForPacket(Packet *p)
{

}
