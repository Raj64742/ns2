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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.cc,v 1.63 2001/06/20 02:23:54 xuanc Exp $
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/scheduler.cc,v 1.63 2001/06/20 02:23:54 xuanc Exp $ (LBL)";
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
scheduler_uid_t Scheduler::uid_ = 1;

// class AtEvent : public Event {
// public:
// 	char* proc_;
// };

Scheduler::Scheduler() : clock_(SCHED_START), halted_(0)
{
}

Scheduler::~Scheduler(){
	instance_ = NULL ;
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
	
	if (delay < 0) {
		// You probably don't want to do this
		// (it probably represents a bug in your simulation).
		fprintf(stderr, "warning: ns Scheduler::schedule: scheduling event\n\twith negative delay (%f) at time %f.\n", delay, clock_);
	}

	if (uid_ < 0) {
		fprintf(stderr, "Scheduler: UID space exhausted!\n");
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
	/*
	 * The order is significant: if halted_ is checked later,
	 * event p could be lost when the simulator resumes.
	 * Patch by Thomas Kaemer <Thomas.Kaemer@eas.iis.fhg.de>.
	 */
	while (!halted_ && (p = deque())) {
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
	if (t < clock_) {
		fprintf(stderr, "ns: scheduler going backwards in time from %f to %f.\n", clock_, t);
	}

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
	AtEvent() : proc_(0) {
	}
	~AtEvent() {
		if (proc_) delete [] proc_;
	}
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
	delete at;
}

void
Scheduler::reset()
{
	clock_ = SCHED_START;
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
			Event* p = lookup(STRTOUID(argv[2]));
      if (p != 0) {
				/*XXX make sure it really is an atevent*/
				cancel(p);
				AtEvent* ae = (AtEvent*)p;
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
			sprintf(tcl.buffer(), UID_PRINTF_FORMAT, e->uid_);
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
			sprintf(tcl.buffer(), UID_PRINTF_FORMAT, e->uid_);
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
		printf("t:%f uid: ", p->time_);
		printf(UID_PRINTF_FORMAT, p->uid_);
		printf(" handler: %p\n", p->handler_);
	}
}

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

Event* ListScheduler::lookup(scheduler_uid_t uid)
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

Heap::Heap(int size)
		: h_s_key(0), h_size(0), h_maxsize(size), h_iter(0)
{
	h_elems = new Heap_elem[h_maxsize];
	memset(h_elems, 0, h_maxsize*sizeof(Heap_elem));
	//for (unsigned int i = 0; i < h_maxsize; i++)
	//	h_elems[i].he_elem = 0;
}

Heap::~Heap()
{
	delete [] h_elems;
}

/*
 * int	heap_member(Heap *h, void *elem):		O(n) algorithm.
 *
 *	Returns index(elem \in h->he_elems[]) + 1,
 *			if elem \in h->he_elems[],
 *		0,	otherwise.
 *	External callers should just test for zero, or non-zero.
 *	heap_delete() uses this routine to find an element in the heap.
 */
int
Heap::heap_member(void* elem)
{
	unsigned int i;
	Heap::Heap_elem* he;
	for (i = 0, he = h_elems; i < h_size; i++, he++)
		if (he->he_elem == elem)
			return ++i;
	return 0;
}

/*
 * int	heap_delete(Heap *h, void *elem):		O(n) algorithm
 *
 *	Returns 1 for success, 0 otherwise.
 *
 * find elem in h->h_elems[] using heap_member()
 *
 * To actually remove the element from the tree, first swap it to the
 * root (similar to the procedure applied when inserting a new
 * element, but no key comparisons--just get it to the root).
 *
 * Then call heap_extract_min() to remove it & fix the tree.
 * 	This process is not the most efficient, but we do not
 *	particularily care about how fast heap_delete() is.
 *	Besides, heap_member() is already O(n), 
 *	and is the dominating cost.
 *
 * Actually remove the element by calling heap_extract_min().
 * 	The key that is now at the root is not necessarily the
 *	minimum, but heap_extract_min() does not care--it just
 *	removes the root.
 */
int
Heap::heap_delete(void* elem)
{
	int	i;
		if ((i = heap_member(elem)) == 0)
		return 0;
		for (--i; i; i = parent(i)) {
		swap(i, parent(i));
	}
	(void) heap_extract_min();
	return 1;
}

/*
 * void	heap_insert(Heap *h, heap_key_t *key, void *elem)
 *
 * Insert <key, elem> into heap h.
 * Adjust heap_size if we hit the limit.
 * 
 *	i := heap_size(h)
 *	heap_size := heap_size + 1
 *	while (i > 0 and key < h[Parent(i)])
 *	do	h[i] := h[Parent(i)]
 *		i := Parent(i)
 *	h[i] := key
 */
void
Heap::heap_insert(heap_key_t key, void* elem) 
{
	unsigned int	i, par;
	if (h_maxsize == h_size) {	/* Adjust heap_size */
		unsigned int osize = h_maxsize;
		Heap::Heap_elem *he_old = h_elems;
		h_maxsize *= 2;
		h_elems = new Heap::Heap_elem[h_maxsize];
		memcpy(h_elems, he_old, osize*sizeof(Heap::Heap_elem));
		delete []he_old;
	}

	i = h_size++;
	par = parent(i);
	while ((i > 0) && 
	       (KEY_LESS_THAN(key, h_s_key,
			      h_elems[par].he_key, h_elems[par].he_s_key))) {
		h_elems[i] = h_elems[par];
		i = par;
		par = parent(i);
	}
	h_elems[i].he_key  = key;
	h_elems[i].he_s_key= h_s_key++;
	h_elems[i].he_elem = elem;
	return;
};
		
/*
 * void *heap_extract_min(Heap *h)
 *
 *	Returns the smallest element in the heap, if it exists.
 *	NULL otherwise.
 *
 *	if heap_size[h] == 0
 *		return NULL
 *	min := h[0]
 *	heap_size[h] := heap_size[h] - 1   # C array indices start at 0
 *	h[0] := h[heap_size[h]]
 *	Heapify:
 *		i := 0
 *		while (i < heap_size[h])
 *		do	l = HEAP_LEFT(i)
 *			r = HEAP_RIGHT(i)
 *			if (r < heap_size[h])
 *				# right child exists =>
 *				#       left child exists
 *				then	if (h[l] < h[r])
 *						then	try := l
 *						else	try := r
 *				else
 *			if (l < heap_size[h])
 *						then	try := l
 *						else	try := i
 *			if (h[try] < h[i])
 *				then	HEAP_SWAP(h[i], h[try])
 *					i := try
 *				else	break
 *		done
 */
void*
Heap::heap_extract_min()
{
	void*	min;				  /* return value */
	unsigned int	i;
	unsigned int	l, r, x;
		if (h_size == 0)
		return 0;
		min = h_elems[0].he_elem;
	h_elems[0] = h_elems[--h_size];
// Heapify:
	i = 0;
	while (i < h_size) {
		l = left(i);
		r = right(i);
		if (r < h_size) {
			if (KEY_LESS_THAN(h_elems[l].he_key, h_elems[l].he_s_key,
					  h_elems[r].he_key, h_elems[r].he_s_key))
				x= l;
			else
				x= r;
		} else
			x = (l < h_size ? l : i);
		if ((x != i) && 
		    (KEY_LESS_THAN(h_elems[x].he_key, h_elems[x].he_s_key,
				   h_elems[i].he_key, h_elems[i].he_s_key))) {
			swap(i, x);
			i = x;
		} else {
			break;
		}
	}
	return min;
}


static class HeapSchedulerClass : public TclClass {
public:
	HeapSchedulerClass() : TclClass("Scheduler/Heap") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new HeapScheduler);
	}
} class_heap_sched;

Event* HeapScheduler::lookup(scheduler_uid_t uid)
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
 *
 * See R.Brown. "Calendar queues: A fast O(1) priority queue implementation 
 *  for the simulation event set problem." 
 *  Comm. of ACM, 31(10):1220-1227, Oct 1988
 */

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
	 * we need to re-calculate the width.
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
		resize(nbuckets_<<1);
}

Event*
CalendarScheduler::deque()
{
	if (qsize_ == 0)
		return NULL;
	for (;;) {
		int i = lastbucket_;
		
		// check for an event this `year'
		do {
			Event* e = buckets_[i];
			if ((e != NULL) && (e->time_ < buckettop_)) {
				buckets_[i] = e->next_;
				lastbucket_ = i;
				last_clock_ = e->time_;
				if (--qsize_ < bot_threshold_)
					resize(nbuckets_>>1);
				return e;
			} else {
				if (++i == nbuckets_) {
					i = 0;
				/* Brad Karp, karp@eecs.harvard.edu:
				   at the end of each year, eliminate roundoff
				   error in buckettop_ */
					buckettop_ = prevtop_ + nbuckets_ * width_;
					prevtop_ = buckettop_;
					
				} else {
					buckettop_+= width_;
				}
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
		buckettop_ = width_*(n+1.5);
		prevtop_ = buckettop_ - lastbucket_ * width_;
	}
	//return deque();
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
	lastbucket_ = n & buckbits_;
	buckettop_ = width_*(n+1.5);
	prevtop_ = buckettop_ - lastbucket_ * width_;
	bot_threshold_ = (nbuck >> 1) - 2;
	top_threshold_ = (nbuck << 1);
}

void CalendarScheduler::resize(int newsize)
{
	if (!resizeenabled_) return;

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
	double olbt = prevtop_;

	resizeenabled_ = 0;
	for (int i = 0; i < nsamples; i++) hold[i] = deque();
	// insert in the inverse order and using insert2 to take care of same-time events.
	for (int j = nsamples-1; j >= 0; j--) insert2(hold[j]);
	resizeenabled_ = 1;

	clock_ = olp;
	buckettop_ = olt;
	prevtop_ = olbt;
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
 *
 * xxx: we may cancel the last event and invalidate the value of max_
 * xxx: thus using wider buckets than really necessary
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

Event* CalendarScheduler::lookup(scheduler_uid_t uid)
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
	virtual void reset();
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

void
RealTimeScheduler::reset()
{
	clock_ = SCHED_START;
	start_ = tod();
}

void RealTimeScheduler::run()
{ 
	Event *p;
	double now;

	/*XXX*/
	instance_ = this;

	while (!halted_) {
		now = tod();
		if ((now - clock_) > slop_) {
			fprintf(stderr,
			"RealTimeScheduler: warning: slop %f exceeded limit %f [now:%f, clock_:%f]\n",
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
