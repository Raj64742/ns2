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
//

#ifdef NS_DIFFUSION

#ifndef DIFFUSION_RTG
#define DIFFUSION_RTG

#include "difftimer.h"
#include "iodev.hh"
#include "events.hh"
#include "classifier-port.h"
#include "ll.h"
#include "ns-process.h"
#include "agent.h"
#include "trace.h"
#include "message.hh"

class DiffusionCoreAgent;
class DiffRoutingAgent;
extern DiffPacket DupPacket(DiffPacket pkt);

class LocalApp : public DiffusionIO {
public:
	LocalApp(DiffRoutingAgent *agent) { agent_ = agent;}
	DiffPacket recvPacket(int fd);
	void sendPacket(DiffPacket p, int len, int dst); 
protected:
	DiffRoutingAgent *agent_;
};

class LinkLayerAbs : public DiffusionIO {
public:
	LinkLayerAbs(DiffRoutingAgent *agent) { agent_ = agent;}
	DiffPacket recvPacket(int fd);
	void sendPacket(DiffPacket p, int len, int dst); 
protected:
	DiffRoutingAgent *agent_;
};
 
class DiffusionCoreEQ : public EventQueue {
public:
	DiffusionCoreEQ(DiffRoutingAgent *agent) { a_ = agent; }
	void eqAddAfter(int type, void *, int delay);
private:
	DiffRoutingAgent *a_;
};


class DiffusionData : public AppData {
private:
	Message *data_;
	int len_;
public:
	DiffusionData(Message *data, int len) : AppData(DIFFUSION_DATA), data_(0)
	{ 
		data_ = data;
		len_ = len;
	}
	~DiffusionData() { delete data_; }
	Message *data() {return data_;}
	int size() const { return len_; }
	AppData* copy() { 
		Message *newdata = CopyMessage(data_);
		DiffusionData *dup = new DiffusionData(newdata, len_);
		return dup; 
	} 
};


class DiffRoutingAgent : public Agent {
public:
	DiffRoutingAgent();
	int command(int argc, const char*const* argv);
	
	Packet* createNsPkt(Message *msg, int len, int dst);  
	void recv(Packet*, Handler*);
	void sendPacket(DiffPacket p, int len, int dst);
  
	DiffusionCoreAgent *getagent() { return agent_; }
	
	//trace support
	void trace (char *fmt,...);
	
	//timer functions
	CoreDiffEventHandler *getDiffTimer() { return difftimer_ ; }
	void diffTimeout(Event *de);

	PortClassifier *port_dmux() {return port_dmux_; }
private:
	int addr_;
	Trace *tracetarget_;
	
	//  diffusion core agent 
	DiffusionCoreAgent *agent_;
	
	// timer
	CoreDiffEventHandler *difftimer_;
	
	//port-dmux
	PortClassifier *port_dmux_;
	
}; 

#endif //diffrtg
#endif // NS

