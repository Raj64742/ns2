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

   dsr_proto.h
   the DSR routing protocol agent
   */


#ifndef _dsr_proto_h
#define _dsr_proto_h

#include <packet.h>
#include <object.h>
#include <agent.h>

#include "path.h"
#include "routecache.h"

class DSRProto : public Agent {
public:
  DSRProto();
  void recv(Packet*, Handler* callback = 0);
  int command(int argc, const char*const* argv);
  void noRouteForPacket(Packet *p);

private:
  Trace *tracetarget;
  RouteCache *routecache;
  void testinit();
  ID net_id;
  ID mac_id;
};

#endif //_dsr_proto_h
