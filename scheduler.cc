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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/scheduler.cc,v 1.3 1997/03/11 00:41:20 kannan Exp $ (LBL)";
#endif

#include <stdlib.h>
#include "scheduler.h"

Scheduler* Scheduler::instance_;
int Scheduler::uid_;

Scheduler::Scheduler() : clock_(0.)
{
}

/*
 * Schedule an event delay time units into the future.
 * The event will be dispatched to the specified handler.
 * We use a relative time to avoid the problem of scheduling
 * something in the past.
 */
void Scheduler::schedule(Handler* h, Event* e, double delay)
{ 
	e->uid_ = uid_++;
	e->handler_ = h;
	double t = clock_ + delay;
	e->time_ = t;
	insert(e);
}

class AtEvent : public Event {
public:
	char* proc_;
};

class AtHandler : public Handler {
public:
	void handle(Event* event);
} at_handler;

void AtHandler::handle(Event* e)
{
	AtEvent* at = (AtEvent*)e;
	Tcl::instance().eval(at->proc_);
	delete at->proc_;
	delete at;
}

int Scheduler::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "run") == 0) {
			/* set global to 0 before calling object reset methods */
			reset();
			run();
			return (TCL_OK);
		} else if (strcmp(argv[1], "now") == 0) {
			sprintf(tcl.buffer(), "%.17g", clock());
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "at") == 0 ||
		    strcmp(argv[1], "cancel") == 0) {
			Event* p = lookup(atoi(argv[2]));
			if (p != 0) {
				/*XXX make sure it really is an atevent*/
				cancel(p);
				AtEvent* ae = (AtEvent*)p;
				delete ae->proc_;
				delete ae;
			}
		}
		return (TCL_OK);
	} else if (argc == 4) {
		if (strcmp(argv[1], "at") == 0) {
			double t = atof(argv[2]);
			const char* proc = argv[3];
	
			AtEvent* e = new AtEvent;
			int n = strlen(proc);
			e->proc_ = new char[n + 1];
			strcpy(e->proc_, proc);
			double delay = t - clock();
			if (delay < 0) {
				tcl.result("can't schedule command in past");
				return (TCL_ERROR);
			}
			schedule(&at_handler, e, delay);
			sprintf(tcl.buffer(), "%u", e->uid_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

/* XXX need to implement a calendar queue as well */

class ListScheduler : public Scheduler {
public:
	inline ListScheduler() : queue_(0) {}
	virtual void run();
	virtual void cancel(Event*);
	virtual void insert(Event*);
	virtual Event* lookup(int uid);
protected:
	Event* queue_;
};

static class ListSchedulerClass : public TclClass {
public:
	ListSchedulerClass() : TclClass("Scheduler/List") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new ListScheduler);
	}
} class_list_sched;

void ListScheduler::insert(Event* e)
{
	double t = e->time_;
	Event** p;
	for (p = &queue_; *p != 0; p = &(*p)->next_)
		if (t < (*p)->time_)
			break;
	e->next_ = *p;
	*p = e;
}

/*
 * Cancel an event.  It is an error to call this routine
 * when the event is not actually in the queue.  The caller
 * must free the event if necessary; this routine only removes
 * it from the scheduler queue.
 */
void ListScheduler::cancel(Event* e)
{
	Event** p;
	for (p = &queue_; *p != e; p = &(*p)->next_)
		if (*p == 0)
			abort();

	*p = (*p)->next_;
}

Event* ListScheduler::lookup(int uid)
{
	Event* p;
	for (p = queue_; p != 0; p = p->next_)
		if (p->uid_ == uid)
			break;
	return (p);
}

void ListScheduler::run()
{ 
	/*XXX*/
	instance_ = this;
	while (queue_ != 0) {
		Event* p = queue_;
		queue_ = p->next_;
		clock_ = p->time_;
		p->handler_->handle(p);
	}
}

#include "heap.h"

class HeapScheduler : public Scheduler {
public:
	inline HeapScheduler() { hp_ = new Heap; } 
	virtual void cancel(Event* p) {
		hp_->heap_delete((void*) p);
	}
	virtual void insert(Event* p) {
		hp_->heap_insert(p->time_, (void*) p);
	}
	virtual Event* lookup(int uid);
	virtual void run();
protected:
	Heap* hp_;
};

static class HeapSchedulerClass : public TclClass {
public:
	HeapSchedulerClass() : TclClass("Scheduler/Heap") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new HeapScheduler);
	}
} class_heap_sched;

Event* HeapScheduler::lookup(int uid)
{
        Event* p;
	
	for (p = (Event*) hp_->heap_iter_init(); p;
	     p = (Event*) hp_->heap_iter())
		if (p->uid_ == uid)
			break;
	return p;
}

void HeapScheduler::run()
{
	Event* p;
	instance_ = this;
	while ((p = (Event*) hp_->heap_extract_min()) != 0) {
		clock_ = p->time_;
		p->handler_->handle(p);
	}
}
