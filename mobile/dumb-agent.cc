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
// dumb-agent.cc

#include "dumb-agent.h"

static class DumbAgentClass : public TclClass {
public:
  DumbAgentClass() : TclClass("Agent/DumbAgent") {}
  TclObject* create(int, const char*const*) {
    return (new DumbAgent());
  }
} class_DumbAgent;

DumbAgent::DumbAgent() : Agent(PT_PING) {}


int DumbAgent::command(int argc, const char*const* argv)
{
  if (argc == 3) {
    if (strcmp(argv[1], "port-dmux") == 0) {
      dmux_ = (PortClassifier *)TclObject::lookup (argv[2]);
      if (dmux_ == 0) {
	fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		 argv[2]);
	return TCL_ERROR;
      }
      return TCL_OK;
    }
    else if (strcmp(argv[1], "tracetarget") == 0) {
      tracetarget_ = (Trace *)TclObject::lookup (argv[2]);
      if (tracetarget_ == 0) {
	fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		 argv[2]);
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return Agent::command(argc, argv);
}
 

void DumbAgent::recv(Packet *p, Handler *h=0) 
{
  
  hdr_cmn *ch = HDR_CMN(p);
  hdr_ip *iph = HDR_IP(p);
  
  if (ch->direction() == hdr_cmn::UP) { // in-coming pkt
    if ((u_int32_t)iph->daddr() == IP_BROADCAST) {
      printf("Recvd brdcast pkt\n");
      dmux_->recv(p, 0);
    
    } else {
      // this agent recvs pkts destined to it only
      // doesnot support multi-hop scenarios
      assert(iph->daddr() == here_.addr_);
      printf("Recvd unicast pkt\n");
      dmux_->recv(p, 0);
    }
    
  } else { // out-going pkt
    target_->recv(p, (Handler*)0);
  }

}
 
void DumbAgent::trace(char *fmt, ...) 
{
  
  va_list ap;
  
  if (!tracetarget_)
    return;
  
  va_start (ap, fmt);
  vsprintf (tracetarget_->pt_->buffer (), fmt, ap);
  tracetarget_->pt_->dump ();
  va_end (ap);
}
