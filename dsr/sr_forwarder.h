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
   
   sr_forwarder.h
   source route forwarder

   */

#ifndef _sr_forwarder_
#define _sr_forwarder_

#include <object.h>
#include <trace.h>

#include "dsr_proto.h"
#include "requesttable.h"
#include "routecache.h"

/* a source route forwarding object takes packets and if it has a SR
routing header it forwards the packets into target_ according to the
header.  Otherwise, it gives the packet to the noroute_ object in
hopes that it knows what to do with it. */

class SRForwarder : public NsObject {
public:
  SRForwarder();

protected:
  virtual int command(int argc, const char*const* argv);
  virtual void recv(Packet*, Handler* callback = 0);

  NsObject* target_;		/* where to send forwarded pkts */
  DSRProto* route_agent_;	        /* where to send unforwardable pkts */
  RouteCache* routecache_;	/* the routecache */

private:
  void handlePktWithoutSR(Packet *p);
  Trace *tracetarget;
  int off_mac_;
  int off_ll_;
  int off_ip_;
  int off_sr_;
};
#endif //_sr_forwarder_
