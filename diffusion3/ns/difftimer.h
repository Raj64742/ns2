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
// Diffusion-event handler object, Padma, nov 2001.

#ifdef NS_DIFFUSION

#ifndef diffevent_handler_h
#define diffevent_handler_h


#include <list>
#include "scheduler.h"
#include "timers.hh"


class MapEntry;
class DiffEventQueue;

typedef long d_handle;
typedef list<MapEntry *> UIDMap_t;


class MapEntry {
public:
	d_handle hdl_;
	scheduler_uid_t uid_;
};



/* This handler class keeps track of diffusion specific events. 
   It schedules multiple events and irrespective of the event types, 
   always calls back the callback function of its agent.
*/

class DiffEvent : public Event {
private:
	struct timeval tv_;	
	d_handle handle_;
	void* payload_;
public:
	DiffEvent(d_handle hdl, void *payload, int time);
	~DiffEvent() { }
	int gettime() {
		return ((tv_.tv_sec) + ((tv_.tv_usec)/1000000));
	}
	d_handle getHandle() { return handle_; }
	void* payload() { return payload_; }
};


class DiffEventHandler : public Handler {
public:
	DiffEventHandler(TimerManager *agent, DiffEventQueue *deq) { 
		a_ = agent; 
		queue_ = deq;
	}
	void handle(Event *);

private:
	TimerManager *a_;
	DiffEventQueue *queue_;
};


class DiffEventQueue : public EventQueue { 
public: 
	DiffEventQueue(TimerManager* a) { 
		a_ = a; 
		timerHandler_ = new DiffEventHandler(a_, this);
	} 
	~DiffEventQueue() { delete timerHandler_; }
	// queue functions
	void eqAddAfter(d_handle hdl, void *payload, int delay_msec);
	DiffEvent *eqFindEvent(d_handle hdl);
	bool eqRemove(d_handle hdl);
	
	// mapping functions
	void setUID(d_handle hdl, scheduler_uid_t uid);
	scheduler_uid_t getUID(d_handle hdl);
	
private: 
	TimerManager *a_;
	DiffEventHandler *timerHandler_;
	
	// map for diffusion handle and ns scheduler uid
	UIDMap_t uidmap_;
}; 





#endif //diffusion_timer_h
#endif // NS
