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

// DiffAppAgent - Wrapper Class for diffusion transport agent DR, ported from SCADDS's directed diffusion software. --Padma, nov 2001.  

#ifdef NS_DIFFUSION

#ifndef NS_DIFFAGENT
#define NS_DIFFAGENT

#include <stdlib.h>
#include "agent.h"
#include "tclcl.h"
#include "events.hh"
#include "dr.hh"
#include "difftimer.h"

class DiffAppAgent;

class NsLocal : public DiffusionIO {
public:
	NsLocal(DiffAppAgent *agent) { agent_ = agent;}
	~NsLocal() {};
	DiffPacket recvPacket(int fd);
	void sendPacket(DiffPacket p, int len, int dst);
protected:
	DiffAppAgent *agent_;
};


class DiffEvent : public Event {
private:
	struct timeval tv_;	
	int type_;
	void* payload_;
public:
	DiffEvent(int type, void *payload, int time);
	~DiffEvent() { }
	int gettime() {
		return ((tv_.tv_sec) + ((tv_.tv_usec)/1000000));
	}
	int type() { return type_; }
	void* payload() { return payload_; }
};

class DiffEventQueue : public EventQueue { 
public: 
	DiffEventQueue(DiffAppAgent *a) { a_ = a; } 
	void eqAddAfter(int type, void *, int delay_msec); 
private: 
	DiffAppAgent *a_;
}; 



class DiffAppAgent : public Agent {
public:
  
	DiffAppAgent();
	int command(int argc, const char*const* argv);
	
	Packet* createNsPkt(Message *msg, int len);	
	void recv(Packet*, Handler*);
	void sendPacket(DiffPacket msg, int len, int dst);
	
	NR *dr() {return dr_; }
  
	//timer functions
	DiffEventHandler *getDiffTimer() { return difftimer_ ; }
	void diffTimeout(Event *e);	
	
protected:

	// diffusion transport agent or DR
	NR *dr_;
	
	//timers
	DiffEventHandler *difftimer_;
  
};

#endif //diffagent
#endif // NS
