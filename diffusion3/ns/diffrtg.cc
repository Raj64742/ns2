// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:52:53 haoboy>
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
// Diffusion Routing Agent - a wrapper class for core diffusion agent, ported from SCADDS's directed diffusion software. --Padma, nov 2001.

#ifdef NS_DIFFUSION

#include "diffrtg.h"
#include "address.h"
#include "scheduler.h"
#include "diffagent.h"


static class DiffRoutingAgentClass : public TclClass {
public:
	DiffRoutingAgentClass() : TclClass("Agent/DiffusionRouting") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc == 5)
			return(new DiffRoutingAgent(atoi(argv[4])));
		
		fprintf(stderr, "Insufficient number of args for creating DiffRtgAgent");
		return (NULL);
	}
} class_diffusion_routing_agent;


void LocalApp::sendPacket(DiffPacket msg, int len, int dst) {
  agent_->sendPacket(msg, len, dst); 
}


DiffPacket LocalApp::recvPacket(int fd) {
  DiffPacket p;
  
  fprintf(stderr, "This function should not get called; call DiffRoutingAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}


void LinkLayerAbs::sendPacket(DiffPacket dp, int len, int dst) {
  Packet *p;
  Handler *h;
  hdr_cmn *ch;
  hdr_ip *iph;
  Message *msg;
  
  msg = (Message *)dp;
  p = agent_->createNsPkt(msg, len, dst); 
  iph = HDR_IP(p);
  ch = HDR_CMN(p);
  iph->saddr() = agent_->addr();
  iph->sport() = agent_->port();    //RT_PORT;
  iph->daddr() = msg->next_hop_;  // Use diffusion next_hop_
  iph->dport() = agent_->port();    //RT_PORT;
  ch->next_hop_ = msg->next_hop_;  // populate nexthop in cmn hdr
  agent_->send(p, h);

}


DiffPacket LinkLayerAbs::recvPacket(int fd) {
  DiffPacket p;
  
  fprintf(stderr, "This function should not get called; call DiffRoutingAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}



DiffRoutingAgent::DiffRoutingAgent(int nodeid) : Agent(PT_DIFF) {
	agent_ = new DiffusionCoreAgent(this, nodeid);

}


void DiffRoutingAgent::sendPacket(DiffPacket dp, int len, int dst) {
	Packet *p;
	hdr_ip *iph;
	Message *msg;

	msg = (Message *)dp;
	p = createNsPkt(msg, len, dst); 
	iph = HDR_IP(p);
	iph->dport() = dst;
	
	// schedule for a realistic delay : 0 sec for now
	(void)Scheduler::instance().schedule(port_dmux(), p, 0.000001);
	
}

void
DiffRoutingAgent::initpkt(Packet* p, Message* msg, int len)
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
	iph->sport() = port(); // RT_PORT
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
DiffRoutingAgent::createNsPkt(Message *msg, int len, int dst) {
	Packet *p;
	
	p = Packet::alloc();
	initpkt(p, msg, len);
	return p;
}

void DiffRoutingAgent::recv(Packet *p, Handler *h) {
	Message *msg;
	DiffusionData *diffdata;
	
	diffdata = (DiffusionData *)(p->userdata());
	msg = diffdata->data();
	
	agent_->recvMessage(msg);
	
	//delete msg;
	Packet::free(p);
}

int DiffRoutingAgent::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcasecmp(argv[1], "start")==0) {
			//start();
			//eq = new DiffusionCoreEQ(this);
			//eq->eq_new();
			// Add timers to the eventQueue
			//eq->eq_addAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
			//eq->eq_addAfter(FILTER_TIMER, NULL, FILTER_DELAY);
			
			return TCL_OK;
		}
		
	}
	else if (argc == 3) {
		if (strcasecmp(argv[1], "addr") == 0) {
			addr_ = (Address::instance().str2addr(argv[2]));
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "stop-time")==0) {
			// add stop-event which when fired dumps statistical data
			// at end of ns simulation
			TimerCallback *callback;
			callback = new DiffusionStopTimer(agent_);
			agent_->timers_manager_->addTimer(atoi(argv[2])*1000, callback);
			return TCL_OK;
		}
		TclObject *obj;
		if ((obj = TclObject::lookup (argv[2])) == 0) {
			fprintf(stderr, "_Diffusion Node_ %s lookup of %s failed\n", argv[1], argv[2]);
			return TCL_ERROR;
		}
		if (strcasecmp(argv[1], "port-dmux") == 0) {
			port_dmux_ = (PortClassifier *)obj;
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "add-ll") == 0) {
			target_ = (LL*)obj;
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "tracetarget") == 0) {
			tracetarget_ = (Trace *)obj;
			return TCL_OK;
		}
	}
	return Agent::command(argc, argv);
}


void DiffRoutingAgent::trace(char *fmt, ...) {
	va_list ap;
	if (!tracetarget_)
		return;
	
	va_start (ap, fmt);
	vsprintf (tracetarget_->pt_->buffer (), fmt, ap);
	tracetarget_->pt_->dump ();
	va_end (ap);
}
	

#endif // NS
