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
// Diffusion-event handler class, Padma, nov 2001. 

#ifdef NS_DIFFUSION

#include "difftimer.h"
#include "diffagent.h"
#include "diffrtg.h"

DiffEvent::DiffEvent(d_handle hdl, void *payload, int time) {
	handle_ = hdl;
	payload_ = payload;
	GetTime(&tv_);
	TimevalAddusecs(&tv_, time*1000);
}

void DiffEventQueue::eqAddAfter(d_handle hdl, void *payload, int delay_msec) {
	DiffEvent* de;
	
	de = new DiffEvent(hdl, payload, delay_msec);
	DiffEventHandler *dh = timerHandler_;
	double delay = ((double)delay_msec)/1000;   //convert msec to sec
	
	(void)Scheduler::instance().schedule(dh, de, delay);
	scheduler_uid_t uid = ((Event *)de)->uid_;
	
	setUID(hdl, uid);
}


DiffEvent *DiffEventQueue::eqFindEvent(d_handle hdl) {
	scheduler_uid_t uid = getUID(hdl);
	if (uid != -1) {
		Event *p = Scheduler::instance().lookup(uid);
		if (p != 0) {
			return ((DiffEvent *)p);
		} else {
			fprintf(stderr, "Error: Can't find event in scheduler queue\n");
			return NULL;
		}
	} else {
		fprintf(stderr, "Error: Can't find event in uidmap!\n");
		return NULL;
	}
}

bool DiffEventQueue::eqRemove(d_handle hdl) {
	// first lookup UID
	scheduler_uid_t uid = getUID(hdl);
	if (uid != -1) {
		// next look for event in scheduler queue
		Event *p = Scheduler::instance().lookup(uid);
		if (p != 0) {
			// remove event from scheduler
			Scheduler::instance().cancel(p);
			TimerEntry* te = (TimerEntry *)((DiffEvent *)p)->payload();
			delete te;
			delete p;
			return 1;
		} else {
			fprintf(stderr, "Error: Can't find event in scheduler queue\n");
			return 0;
		}
	} else {
		fprintf(stderr, "Error: Can't find event in uidmap!\n");
		return 0;
	}
}

// sets value of UID and matching handle into uidmap
void DiffEventQueue::setUID(d_handle hdl, scheduler_uid_t uid) {
	MapEntry *me = new MapEntry;
	me->hdl_ = hdl;
	me->uid_ = uid;
	uidmap_.push_back(me);
}

// finds UID for matching handle; removes entry from uidmap and returns uid
scheduler_uid_t DiffEventQueue::getUID(d_handle hdl) {
	UIDMap_t::iterator itr;
	MapEntry* entry;
	for (itr = uidmap_.begin(); itr != uidmap_.end(); ++itr) {
		entry = *itr;
		if (entry->hdl_ == hdl) { // found handle
			// don't need this entry, take it out of list
			scheduler_uid_t uid = entry->uid_;
			itr = uidmap_.erase(itr);
			delete entry;
			return uid;
		}
	}
	return (-1);
}

// event handler that gets called at expiry time of event
void DiffEventHandler::handle(Event *e) {
	DiffEvent *de = (DiffEvent *)e;
	
	d_handle hdl = de->getHandle();
	queue_->getUID(hdl); // only removes entry from uidmap
	a_->diffTimeout(de);
}


#endif // NS
