// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:53:32 haoboy>
//
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
// DiffAppAgent - Wrapper Class for diffusion transport agent DR, ported from SCADDS's directed diffusion software. --Padma, nov 2001.  

#ifdef NS_DIFFUSION

#include "diffagent.h"
#include "diffrtg.h"


static class DiffAppAgentClass : public TclClass {
public:
  DiffAppAgentClass() : TclClass("Agent/DiffusionApp") {}
  TclObject* create(int , const char*const* ) {
    return(new DiffAppAgent());
  }
} class_diffusion_app_agent;


void NsLocal::sendPacket(DiffPacket p, int len, int dst) {
  agent_->sendPacket(p, len, dst);
}

DiffPacket NsLocal::recvPacket(int fd) {
  DiffPacket p;
  fprintf(stderr, "This function should not get called; call DiffAppAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}


DiffAppAgent::DiffAppAgent() : Agent(PT_DIFF) {
  dr_ = NR::create_ns_NR(DEFAULT_DIFFUSION_PORT, this);
}


int DiffAppAgent::command(int argc, const char*const* argv) {
  //Tcl& tcl = Tcl::instance();
  
  if (argc == 3) {
	  if (strcmp(argv[1], "node") == 0) {
		  MobileNode *node = (MobileNode *)TclObject::lookup(argv[2]);
		  ((DiffusionRouting *)dr_)->getNode(node);
		  return TCL_OK;
	  } 
	  if (strcmp(argv[1], "agent-id") == 0) {
		  int id = atoi(argv[2]);
		  ((DiffusionRouting *)dr_)->getAgentId(id);
		  return TCL_OK;
	  }
  }
  return Agent::command(argc, argv);
}



void DiffAppAgent::recv(Packet* p, Handler* h) 
{
	Message *msg;
	DiffusionData *diffdata;
	
	diffdata = (DiffusionData *)(p->userdata());
	msg = diffdata->data();
	
	DiffusionRouting *dr = (DiffusionRouting*)dr_;
	dr->recvMessage(msg);
	
	Packet::free(p);
	
}

void
DiffAppAgent::initpkt(Packet* p, Message* msg, int len)
{
	hdr_cmn* ch = HDR_CMN(p);
	hdr_ip* iph = HDR_IP(p);
	AppData *diffdata;
		
	diffdata  = new DiffusionData(msg, len);
	p->setdata(diffdata);
	
	// initialize pkt
	ch->uid() = msg->pkt_num_; /* copy pkt_num from diffusion msg */
	ch->ptype() = type_;
	ch->size() = size_;
	ch->timestamp() = Scheduler::instance().clock();
	ch->iface() = UNKN_IFACE.value(); // from packet.h (agent is local)
	ch->direction() = hdr_cmn::NONE;
	ch->error() = 0;	/* pkt not corrupt to start with */

	iph->saddr() = addr();
	iph->sport() = ((DiffusionRouting*)dr_)->getAgentId();
	iph->daddr() = addr();
	iph->flowid() = fid_;
	iph->prio() = prio_;
	iph->ttl() = defttl_;
	
	hdr_flags* hf = hdr_flags::access(p);
	hf->ecn_capable_ = 0;
	hf->ecn_ = 0;
	hf->eln_ = 0;
	hf->ecn_to_echo_ = 0;
	hf->fs_ = 0;
	hf->no_ts_ = 0;
	hf->pri_ = 0;
	hf->cong_action_ = 0;
	
}

Packet* 
DiffAppAgent::createNsPkt(Message *msg, int len) 
{
	Packet *p;
  
	p =  Packet::alloc();
	initpkt(p, msg, len);
	return (p);
}


void DiffAppAgent::sendPacket(DiffPacket dp, int len, int dst) {
  Packet *p;
  hdr_ip *iph;
  Message *msg;

  msg = (Message *)dp;
  p = createNsPkt(msg, len); 
  iph = HDR_IP(p);
  iph->dport() = dst;

  // schedule for realistic delay : set to 0 sec for now
  (void)Scheduler::instance().schedule(target_, p, 0.000001);

}


#endif // NS



