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



DiffEvent::DiffEvent(int type, void *payload, int time) {
  type_ = type;
  payload_ = payload;
  getTime(&tv_);
  timeval_addusecs(&tv_, time*1000);
}

void DiffEventQueue::eqAddAfter(int type, void *payload, int delay_msec) {
  DiffEvent* de;
	
  de = new DiffEvent(type, payload, delay_msec);
  DiffEventHandler *dh = a_->getDiffTimer();
  double delay = delay_msec/1000;   //convert msec to sec
	
  (void)Scheduler::instance().schedule(dh, de, delay);
}


DiffAppAgent::DiffAppAgent() : Agent(PT_DIFF) {
  dr_ = NR::create_ns_NR(DEFAULT_DIFFUSION_PORT, this);
  difftimer_ = new DiffEventHandler(this);
}

void DiffAppAgent::diffTimeout(Event *de) {
  DiffEvent *e = (DiffEvent *)de;
  
  switch (e->type()) {
	  
  case INTEREST_TIMER:
	  
	  //pthread_mutex_lock(drMtx);
	  ((DiffusionRouting *)dr_)->interestTimeout((HandleEntry *)(e->payload()));
	  //pthread_mutex_unlock(drMtx);
	  
	  delete e;
	  
	  break;
	  
  case FILTER_KEEPALIVE_TIMER:
	  
	  //pthread_mutex_lock(drMtx);
	  ((DiffusionRouting *)dr_)->filterKeepaliveTimeout((FilterEntry *) (e->payload()));
	  //pthread_mutex_unlock(drMtx);
	  
	  delete e;
	  
	  break;
	  
  case APPLICATION_TIMER:
	  
	  ((DiffusionRouting *)dr_)->applicationTimeout((TimerEntry *) (e->payload()));
	  
	  delete e;
	  
	  break;
	  
  default:
	  // no error for unknown type of timer ??
	  break;
	  
  }
}


int DiffAppAgent::command(int argc, const char*const* argv) {
  //Tcl& tcl = Tcl::instance();
  
  if (argc == 3) {
	  if (strcmp(argv[1], "agent-id") == 0) {
		  int id = atoi(argv[2]);
		  ((DiffusionRouting *)dr_)->get_agentid(id);
		  return TCL_OK;
	  }
  }
  return Agent::command(argc, argv);
}



void DiffAppAgent::recv(Packet* p, Handler* h) {
  Message *msg;
  DiffusionData *diffdata;

  diffdata = (DiffusionData *)(p->userdata());
  msg = diffdata->data();
  
  DiffusionRouting *dr = (DiffusionRouting*)dr_;
  dr->recvMessage(msg);
  
  //delete msg;
  Packet::free(p);
  
  
}


Packet* DiffAppAgent::createNsPkt(Message *msg, int len) {
  Packet *p;
  AppData *diffdata;
  
  p = allocpkt();
  diffdata  = new DiffusionData(msg, len);
  p->setdata(diffdata);
  return p;
}


void DiffAppAgent::sendPacket(DiffPacket dp, int len, int dst) {
  Packet *p;
  hdr_ip *iph;
  Message *msg;

  msg = (Message *)dp;
  p = createNsPkt(msg, len); 
  iph = HDR_IP(p);
  iph->saddr() = addr();
  iph->sport() = ((DiffusionRouting*)dr_)->get_agentid();
  iph->daddr() = addr();
  iph->dport() = dst;

  // schedule for realistic delay : set to 0 sec for now
  (void)Scheduler::instance().schedule(target_, p, 1);

}


#endif // NS



