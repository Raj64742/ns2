/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.h,v 1.24 2002/07/18 23:09:53 yuri Exp $ (LBL)
 */

#ifndef ns_scheduler_h
#define ns_scheduler_h

#include "config.h"

// Make use of 64 bit integers if available.
#ifdef HAVE_INT64
typedef int64_t scheduler_uid_t;
#define UID_PRINTF_FORMAT STRTOI64_FMTSTR
#define STRTOUID(S) STRTOI64((S), NULL, 0)
#else
typedef int scheduler_uid_t;
#define UID_PRINTF_FORMAT "%d"
#define STRTOUID(S) atoi((S))
#endif


class Handler;

class Event {
public:
	Event* next_;		/* event list */
	Event* prev_;
	Handler* handler_;	/* handler to call when event ready */
	double time_;		/* time at which event is ready */
	scheduler_uid_t uid_;	/* unique ID */
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
#define CALENDAR_ALPHA  2.0    /* used for resizing bucket width incase of overflow */

class Scheduler : public TclObject {
public:
	static Scheduler& instance() {
		return (*instance_);		// general access to scheduler
	}
	void schedule(Handler*, Event*, double delay);	// sched later event
	virtual void run();			// execute the simulator
	virtual void cancel(Event*) = 0;	// cancel event
	virtual void insert(Event*) = 0;	// schedule event
	virtual Event* lookup(scheduler_uid_t uid) = 0;	// look for event
	virtual Event* deque() = 0;		// next event (removes from q)
	inline double clock() const {		// simulator virtual time
		return (clock_);
	}
	virtual void sync() {};
	virtual double start() {		// start time
		return SCHED_START;
	}
	virtual void reset();
protected:
	void dumpq();	// for debug: remove + print remaining events
	void dispatch(Event*);	// execute an event
	void dispatch(Event*, double);	// exec event, set clock_
	Scheduler();
	virtual ~Scheduler();
	int command(int argc, const char*const* argv);
	double clock_;
	int halted_;
	static Scheduler* instance_;
	static scheduler_uid_t uid_;
};

class ListScheduler : public Scheduler {
public:
	inline ListScheduler() : queue_(0) {}
	virtual void cancel(Event*);
	virtual void insert(Event*);
	virtual Event* deque();
	virtual Event* lookup(scheduler_uid_t uid);
protected:
	Event* queue_;
};

#include "heap.h"

class HeapScheduler : public Scheduler {
public:
	inline HeapScheduler() { hp_ = new Heap; } 
	virtual void cancel(Event* e) {
		if (e->uid_ <= 0)
			return;
		e->uid_ = - e->uid_;
		hp_->heap_delete((void*) e);
	}
	virtual void insert(Event* e) {
		hp_->heap_insert(e->time_, (void*) e);
	}
	virtual Event* lookup(scheduler_uid_t uid);
	virtual Event* deque();
protected:
	Heap* hp_;
};

class CalendarScheduler : public Scheduler {
public:
	CalendarScheduler();
	virtual ~CalendarScheduler();
	virtual void cancel(Event*);
	virtual void insert(Event*);
	virtual Event* lookup(scheduler_uid_t uid);
	virtual Event* deque();

protected:
	double width_;
	double diff0_, diff1_, diff2_; /* wrap-around checks */

	int stat_qsize_;		/* # of distinct priorities in queue*/
	int nbuckets_;
	int lastbucket_;
	int top_threshold_;
	int bot_threshold_;

	struct Bucket {
		Event *list_;
		int    count_;
	} *buckets_;
		
	int qsize_;

	virtual void reinit(int nbuck, double bwidth, double start);
	virtual void resize(int newsize, double start);
	virtual double newwidth();

private:
	virtual void insert2(Event*);
	double cal_clock_;  // same as clock in sims, may be different in RT-scheduling.

};

#endif
