/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.h,v 1.13 1998/06/26 02:28:22 gnguyen Exp $ (LBL)
 */

#ifndef ns_scheduler_h
#define ns_scheduler_h

#include <tclcl.h>

class Handler;

class Event {
public:
	Event* next_;		/* event list */
	Handler* handler_;	/* handler to call when event ready */
	double time_;		/* time at which event is ready */
	int uid_;		/* unique ID */
	Event() : time_(0), uid_(0) {}
};

/*
 * The base class for all event handlers.  When an event's scheduled
 * time arrives, it is passed to handle which must consume it.
 * i.e., if it needs to be freed it, it must be freed by the handler.
 */
class Handler {
 public:
	virtual void handle(Event* event) = 0;
};

#define	SCHED_START	0.0	/* start time (secs) */

class Scheduler : public TclObject {
public:
	static Scheduler& instance() {
		return (*instance_);		// general access to scheduler
	}
	void schedule(Handler*, Event*, double delay);	// sched later event
	void rc_schedule(Handler*, Event*, double delay);
	virtual void run();			// execute the simulator
	virtual void cancel(Event*) = 0;	// cancel event
	virtual void insert(Event*) = 0;	// schedule event
	virtual Event* lookup(int uid) = 0;	// look for event
	virtual Event* deque() = 0;		// next event (removes from q)
	inline double clock() const {		// simulator virtual time
		return (clock_);
	}
	virtual void sync() {};
	virtual double start() {		// start time
		return SCHED_START;
	}
	inline void reset() { clock_ = SCHED_START; }/*XXX*/
protected:
	void dumpq();	// for debug: remove + print remaining events
	void dispatch(Event*);	// execute an event
	void dispatch(Event*, double);	// exec event, set clock_
	Scheduler();
	int command(int argc, const char*const* argv);
	double clock_;
	int halted_;
	static Scheduler* instance_;
	static int uid_;
};
#endif
