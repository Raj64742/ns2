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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.cc,v 1.43 1999/06/09 21:23:34 kfall Exp $
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.cc,v 1.43 1999/06/09 21:23:34 kfall Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <limits.h>
#include "config.h"
#include "scheduler.h"
#include "packet.h"

#ifdef MEMDEBUG_SIMULATIONS
#include "mem-trace.h"
#endif

Scheduler* Scheduler::instance_;
int Scheduler::uid_ = 1;

Scheduler::Scheduler() : clock_(SCHED_START), halted_(0)
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
	if (e->uid_ > 0) {
		printf("Scheduler: Event UID not valid!\n\n");
		abort();
	}
	e->uid_ = uid_++;
	e->handler_ = h;
	double t = clock_ + delay;
	e->time_ = t;
	insert(e);
}

void
Scheduler::run()
{
	instance_ = this;
	Event *p;
	while ((p = deque()) && !halted_) {
		dispatch(p);
	}
}

/*
 * dispatch a single simulator event by setting the system
 * virtul clock to the event's timestamp and calling its handler.
 * Note that the event may have side effects of placing other items
 * in the scheduling queue
 */

void
Scheduler::dispatch(Event* p, double t)
{
	if (t < clock_)
		fprintf(stderr, "ns: scheduler going backwards in time from %f to %f.\n", clock_, t);
	clock_ = t;
	p->uid_ = -p->uid_;	// being dispatched
	p->handler_->handle(p);	// dispatch
}

void
Scheduler::dispatch(Event* p)
{
	dispatch(p, p->time_);
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
	delete[] at->proc_;
	delete at;
}

int Scheduler::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (instance_ == 0)
		instance_ = this;
	if (argc == 2) {
		if (strcmp(argv[1], "run") == 0) {
			/* set global to 0 before calling object reset methods */
			reset();	// sets clock to zero
			run();
			return (TCL_OK);
		} else if (strcmp(argv[1], "now") == 0) {
			sprintf(tcl.buffer(), "%.17g", clock());
			tcl.result(tcl.buffer());
			return (TCL_OK);
		} else if (strcmp(argv[1], "resume") == 0) {
			halted_ = 0;
			run();
			return (TCL_OK);
		} else if (strcmp(argv[1], "halt") == 0) {
			halted_ = 1;
			return (TCL_OK);

		} else if (strcmp(argv[1], "clearMemTrace") == 0) {
#ifdef MEMDEBUG_SIMULATIONS
			extern MemTrace *globalMemTrace;
			if (globalMemTrace)
				globalMemTrace->diff("Sim.");
#endif
			return (TCL_OK);
		} else if (strcmp(argv[1], "is-running") == 0) {
			sprintf(tcl.buffer(), "%d", !halted_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "dumpq") == 0) {
			if (!halted_) {
				fprintf(stderr, "Scheduler: dumpq only allowed while halted\n");
				tcl.result("0");
				return (TCL_ERROR);
			}
			dumpq();
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
				delete[] ae->proc_;
				delete ae;
			}
		} else if (strcmp(argv[1], "at-now") == 0) {
			const char* proc = argv[2];

			// "at [$ns now]" may not work because of tcl's 
			// string number resolution
			AtEvent* e = new AtEvent;
			int n = strlen(proc);
			e->proc_ = new char[n + 1];
			strcpy(e->proc_, proc);
			schedule(&at_handler, e, 0);
			sprintf(tcl.buffer(), "%u", e->uid_);
			tcl.result(tcl.buffer());
		}
		return (TCL_OK);
	} else if (argc == 4) {
		if (strcmp(argv[1], "at") == 0) {
			/* t < 0 means relative time: delay = -t */
			double delay, t = atof(argv[2]);
			const char* proc = argv[3];

			AtEvent* e = new AtEvent;
			int n = strlen(proc);
			e->proc_ = new char[n + 1];
			strcpy(e->proc_, proc);
			delay = (t < 0) ? -t : t - clock();
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

void
Scheduler::dumpq()
{
	Event *p;

	printf("Contents of scheduler queue (events) [cur time: %f]---\n",
		clock());
	while ((p = deque()) != NULL) {
		printf("t:%f uid: %d handler: %p\n",
			p->time_, p->uid_, p->handler_);
	}
}

class ListScheduler : public Scheduler {
public:
	inline ListScheduler() : queue_(0) {}
	virtual void cancel(Event*);
	virtual void insert(Event*);
	virtual Event* deque();
	virtual Event* lookup(int uid);
protected:
	Event* queue_;
};

static class ListSchedulerClass : public TclClass {
public:
	ListSchedulerClass() : TclClass("Scheduler/List") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
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
	if (e->uid_ <= 0)	// event not in queue
		return;
	for (p = &queue_; *p != e; p = &(*p)->next_)
		if (*p == 0)
			abort();

	*p = (*p)->next_;
	e->uid_ = - e->uid_;
}

Event* ListScheduler::lookup(int uid)
{
	Event* e;
	for (e = queue_; e != 0; e = e->next_)
		if (e->uid_ == uid)
			break;
	return (e);
}


Event*
ListScheduler::deque()
{ 
	Event* e = queue_;
	if (e)
		queue_ = e->next_;
	return (e);
}

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
	virtual Event* lookup(int uid);
	virtual Event* deque();
protected:
	Heap* hp_;
};

static class HeapSchedulerClass : public TclClass {
public:
	HeapSchedulerClass() : TclClass("Scheduler/Heap") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new HeapScheduler);
	}
} class_heap_sched;

Event* HeapScheduler::lookup(int uid)
{
	Event* e;
	
	for (e = (Event*) hp_->heap_iter_init(); e;
	     e = (Event*) hp_->heap_iter())
		if (e->uid_ == uid)
			break;
	return e;
}

Event*
HeapScheduler::deque()
{
	return ((Event*) hp_->heap_extract_min());
}

/*
 * Calendar queue scheduler contributed by
 * David Wetherall <djw@juniper.lcs.mit.edu>
 * March 18, 1997.
 *
 * A calendar queue basically hashes events into buckets based on
 * arrival time.
 */

class CalendarScheduler : public Scheduler {
public:
	CalendarScheduler();
	virtual ~CalendarScheduler();
	virtual void cancel(Event*);
	virtual void insert(Event*);
	virtual Event* lookup(int uid);
	virtual Event* deque();

protected:
	int resizeenabled_;
	double width_;
	double oneonwidth_;
	double buckettop_;
	double last_clock_;
	int nbuckets_;
	int buckbits_;
	int lastbucket_;
	int top_threshold_;
	int bot_threshold_;
	Event** buckets_;
	int qsize_;
	double max_;

	virtual void reinit(int nbuck, double bwidth, double start);
	virtual void resize(int newsize);
	virtual double newwidth();

private:
	virtual void insert2(Event*);

};

static class CalendarSchedulerClass : public TclClass {
public:
	CalendarSchedulerClass() : TclClass("Scheduler/Calendar") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new CalendarScheduler);
	}
} class_calendar_sched;

CalendarScheduler::CalendarScheduler() {
	reinit(2, 1.0, 0.0);
	resizeenabled_ = 1;
	max_ = 0.0;
}

CalendarScheduler::~CalendarScheduler() {
	delete [] buckets_;
	qsize_ = 0;
}

void CalendarScheduler::insert(Event* e)
{

	/* (e->time_ * oneonwidth) needs to be less than the
	 * largest integer on the machine for the mapping
	 * of time to bucket to work properly.  if it is not
	 * we force a resize() where we reset the width.
	 */
	if (e->time_ > max_) {
		max_ = e->time_;
		if (e->time_ * oneonwidth_ > ULONG_MAX) {
			resize(nbuckets_);
		}
	}
	// bucket number and address
	int i = (int)(((long)(e->time_ * oneonwidth_)) & buckbits_); 
	Event** p = buckets_ + i;

	// insert event in stable time sorted order
	while ((*p != NULL) && (e->time_ >= (*p)->time_))
		p = &(*p)->next_;

	e->next_ = *p;
	*p = e;

	if (++qsize_ > top_threshold_)
		resize(2*nbuckets_);
}

Event*
CalendarScheduler::deque()
{
	if (qsize_ == 0)
		return NULL;
	int i = lastbucket_;

	// check for an event this `year'
	do {
		Event* e = buckets_[i];
		if ((e != NULL) && (e->time_ < buckettop_)) {
			buckets_[i] = e->next_;
			lastbucket_ = i;
			last_clock_ = e->time_;
			if (--qsize_ < bot_threshold_)
				resize(nbuckets_/2);
			return e;
		} else {
			if (++i == nbuckets_)
				i = 0;
			buckettop_ += width_;
		}
	} while (i != lastbucket_);

	// or direct search for the minimum event
	int pos = 0;
	Event* min;
	do {
		min = buckets_[pos++];
	} while (min == NULL);
	pos--;

	int k;
	for (k = pos+1; k < nbuckets_; k++) {
		Event* e = buckets_[k];
		if ((e != NULL) && (e->time_ < min->time_)) {
			min = e; pos = k;
		}
	}

	// adjust year and resume
	lastbucket_ = pos;
	last_clock_ = min->time_;
	unsigned long n = (unsigned long)(min->time_ * oneonwidth_);
	buckettop_ = (n + 1) * width_ + 0.5 * width_;

	return deque();
}

void CalendarScheduler::reinit(int nbuck, double bwidth, double start)
{
	buckets_ = new Event*[nbuck];
	memset(buckets_, 0, sizeof(Event*)*nbuck);
	width_ = bwidth;
	oneonwidth_ = 1.0 / width_;
	nbuckets_ = nbuck;
	buckbits_ = nbuck-1;
	qsize_ = 0;
	last_clock_ = start;
	long n = (long)(start * oneonwidth_);
	lastbucket_ = n % nbuck;
	buckettop_ = (n + 1) * width_ + 0.5 * width_;
	bot_threshold_ = nbuck / 2 - 2;
	top_threshold_ = 2 * nbuck;
}

void CalendarScheduler::resize(int newsize)
{
	if (!resizeenabled_) return;
	resizeenabled_ = 0;
	double bwidth = newwidth();
	Event** oldb = buckets_;
	int oldn = nbuckets_;

	// copy events to new buckets
	reinit(newsize, bwidth, clock_);
	int i;
	for (i = oldn-1; i >= 0; i--) {
		Event* e = oldb[i];
		while (e != NULL) {
			Event* en = e->next_;
			insert(e);
			e = en;
		}
	}
	delete [] oldb;
	resizeenabled_ = 1;
}

#define MIN_WIDTH (1.0e-6)
#define MAX_HOLD  25

double
CalendarScheduler::newwidth()
{
	static Event* hold[MAX_HOLD];
	int nsamples;

	if (qsize_ < 2) return 1.0;
	if (qsize_ < 5) nsamples = qsize_;
	else nsamples = 5 + qsize_ / 10;
	if (nsamples > MAX_HOLD) nsamples = MAX_HOLD;

	// dequue and requeue sample events to gauge width
	double olp = clock_;
	double olt = buckettop_;
	int olb = lastbucket_;
	for (int i = 0; i < nsamples; i++) hold[i] = deque();
	// insert in the inverse order and using insert2 to take care of same-time events.
	for (int j = nsamples-1; j >= 0; j--) insert2(hold[j]);
	clock_ = olp;
	buckettop_ = olt;
	lastbucket_ = olb;

	// try to work out average cluster separation
	double asep = (hold[nsamples-1]->time_ - hold[0]->time_) / (nsamples-1);
	double asep2 = 0.0;
	double min = (clock_ + 1.0) * MIN_WIDTH; 
	int count = 0;

	for (int k = 1; k < nsamples; k++) {
		double diff = hold[k]->time_ - hold[k-1]->time_;
		if (diff < 2.0*asep) { asep2 += diff; count++; }
	}

	// but don't let things get too small for numerical stability
	double nw = count ? 3.0*(asep2/count) : asep;
	if (nw < min) nw = min;

	/* need to make sure that time_/width_ can be represented as
	 * an int.  see the comment at the start of insert().
	 */
	if (max_/nw > ULONG_MAX) {
		nw = max_/ULONG_MAX;
	}
	return nw;
}

/*
 * Cancel an event.  It is an error to call this routine
 * when the event is not actually in the queue.  The caller
 * must free the event if necessary; this routine only removes
 * it from the scheduler queue.
 */
void CalendarScheduler::cancel(Event* e)
{
	int i = (int)(((long)(e->time_ * oneonwidth_)) & buckbits_);

	if (e->uid_ <= 0)	// event not in queue
		return;
	for (Event** p = buckets_ + i; (*p) != NULL; p = &(*p)->next_)
		if ((*p) == e) {
			(*p) = (*p)->next_;
			e->uid_ = - e->uid_;
			qsize_--;
			return;
		}
	abort();
}

Event* CalendarScheduler::lookup(int uid)
{
	for (int i = 0; i < nbuckets_; i++)
		for (Event* p = buckets_[i]; p != NULL; p = p->next_)
			if (p->uid_== uid) return p;
	return NULL;
}

void CalendarScheduler::insert2(Event* e)
{
	// Same as insert, but for inserts e *before* any same-time-events, if
	//   there should be any.  Since it is used only by CalendarScheduler::newwidth(),
	//   some important checks present in insert() need not be performed.

	// bucket number and address
	int i = (int)(((long)(e->time_ * oneonwidth_)) & buckbits_); 
	Event** p = buckets_ + i;

	// insert event in stable time sorted order
	while ((*p != NULL) && (e->time_ > (*p)->time_)) // > instead of >=!
		p = &(*p)->next_;

	e->next_ = *p;
	*p = e;
	++qsize_;
}

#ifndef WIN32
#include <sys/time.h>
#endif

/*
 * Really should instance the list/calendar/heap discipline
 * inside a RealTimeScheduler or VirtualTimeScheduler
 */

#ifdef notyet
class RealTimeScheduler : public CalendarScheduler {
#endif

class RealTimeScheduler : public ListScheduler {
public:
	RealTimeScheduler();
	virtual void run();
	double start() const { return start_; }
protected:
	void sync() { clock_ = tod(); }
	int rwait(double);	// sleep
	double tod();
	double slop_;	// allowed drift between real-time and virt time
	double start_;	// starting time
};

static class RealTimeSchedulerClass : public TclClass {
public:
	RealTimeSchedulerClass() : TclClass("Scheduler/RealTime") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new RealTimeScheduler);
	}
} class_realtime_sched;

RealTimeScheduler::RealTimeScheduler() : start_(0.0)
{
	start_ = tod();
	bind("maxslop_", &slop_);
}

double
RealTimeScheduler::tod()
{
	timeval tv;
	gettimeofday(&tv, 0);
	double s = tv.tv_sec;
	s += (1e-6 * tv.tv_usec);
	return (s - start_);
}

// XXX not used?
// static void nullTimer(ClientData)
// {
// }

void RealTimeScheduler::run()
{ 
	Event *p;
	double now;

	/*XXX*/
	instance_ = this;

	while (!halted_) {
		now = tod();
		//if ((clock_ - now) > slop_) {
		if ((now - clock_) > slop_) {
			fprintf(stderr,
			"RealTimeScheduler: warning: slop %f exceeded limit %f [now:%f, clock_:%f\n",
				now - clock_, slop_, now, clock_);
		}

		//
		// first handle any "old events"
		//

		while ((p = deque()) != NULL && (p->time_ <= now)) {
			dispatch(p);
		}

		//
		// now handle a "future event", if there is one
		//
		if (p != NULL) {
			int rval = rwait(p->time_);
			if (rval < 0) {
				fprintf(stderr, "RTScheduler: wait problem\n");
				abort();
			} else if (rval == 0) {
				//
				// proper time to dispatch sim event... do so
				//
				dispatch(p, clock_);
			} else {
				//
				// there was a simulator event which fired, and
				// may have added something to the queue, which
				// could cause our event p to not be the next,
				// so put p back into the event queue and cont
				//
				insert(p);
			}
			continue;
		}

		//
		// no sim events to handle at all, check with tcl
		//
		sync();
		Tcl_DoOneEvent(TCL_DONT_WAIT);
	}

	return;	// we reach here only if halted
}

/*
 * wait until the specified amount has elapsed, or a tcl event has happened,
 * whichever comes first.  Return 1 if a tcl event happened, 0 if the
 * deadline has been reached, or -1 on error (shouldn't happen).
 */

int
RealTimeScheduler::rwait(double deadline)
{
	while (1) {
		sync();
		if (Tcl_DoOneEvent(TCL_DONT_WAIT) == 1)
			return (1);
		if (deadline <= tod())
			return 0;
	}
	return -1;
}
