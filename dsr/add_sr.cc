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

/* 
   add_sr.cc
   add a compiled constant source route to a packet.
   for testing purposes
   Ported from CMU/Monarch's code, appropriate copyright applies.  
   */

#include <packet.h>
#include <ip.h>
#include "hdr_sr.h"

#include <connector.h>

class AddSR : public Connector {
public:
  void recv(Packet*, Handler* callback = 0);
  AddSR();

private:
  int off_ip_;
  int off_sr_;
  int off_ll_;
  int off_mac_;
};

static class AddSRClass : public TclClass {
public:
  AddSRClass() : TclClass("Connector/AddSR") {}
  TclObject* create(int, const char*const*) {
    return (new AddSR);
  }
} class_addsr;

AddSR::AddSR()
{
  bind("off_sr_", &off_sr_);
  bind("off_ll_", &off_ll_);
  bind("off_mac_", &off_mac_);
  bind("off_ip_", &off_ip_);
}

void
AddSR::recv(Packet* p, Handler* callback)
{
  hdr_sr *srh =  hdr_sr::access(p);

  srh->route_request() = 0;
  srh->num_addrs() = 0;
  srh->cur_addr() = 0;
  srh->valid() = 1;
  srh->append_addr( 1<<8, NS_AF_INET );
  srh->append_addr( 2<<8, NS_AF_INET );
  srh->append_addr( 3<<8, NS_AF_INET );
  printf(" added sr %s\n",srh->dump());
  send(p,0);
}
