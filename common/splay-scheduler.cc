/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2002 University of Southern California/Information
 * Sciences Institute.  All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/splay-scheduler.cc,v 1.2 2002/07/16 22:36:20 yuri Exp $
 */
#ifndef lint
static const char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/splay-scheduler.cc,v 1.2 2002/07/16 22:36:20 yuri Exp $ (LBL)";
#endif
/**
 *
 * Scheduler based on a splay tree.
 *
 * Daniel D. Sleator and Robert E. Tarjan. Self-adjusting binary
 * search trees. Journal of the ACM, 32(3):652--686, 1985. 118.
 *
 * Basic idea of this scheduler: it organizes the event queue into a
 * binary search tree.  For every insert and deque operation,
 * "splaying" is performed aimed at shortening the search path for
 * "similar" priorities (time_).  This should give O(log(N)) amortized
 * time for basic operations.
 *
 * Implementation notes: Event::next_ and Event::prev_ are used as
 * right and left pointers.  insert() and deque() use the "top-down"
 * splaying algorithm taken almost verbatim from the paper and in some
 * cases optimized for particular operations.  lookup() is extremely
 * inefficient, and should be avoided whenever possible.  cancel()
 * would be better off if we had a pointer to the parent, then there
 * wouldn't be any need to search for it (and use Event::uid_ to
 * resolve conflicts when same-priority events are both on the left
 * and right).  cancel() does not perform any splaying, while it
 * perhaps should.
 *
 * Memory used by this scheduler is O(1) (not counting the events).
 *
 **/
#include <scheduler.h>
#include <assert.h>

class SplayScheduler : public Scheduler 
{
public:
	SplayScheduler() : root_(0), qsize_(0) {}
	virtual void insert(Event *);
	virtual Event *deque();
	virtual void cancel(Event *);
	virtual Event *lookup(scheduler_uid_t);
	void validate() { assert(validate(root_) == qsize_); };
    
protected:
	Event *&left(Event *e)  { return e->prev_; }
	Event *&right(Event *e) { return e->next_; } 
	Event *uid_lookup(Event *);

	Event			*root_;
	scheduler_uid_t 	lookup_uid_;
	int 			qsize_;
private:
	int validate(Event *);
};

static class SplaySchedulerClass : public TclClass 
{
public:
        SplaySchedulerClass() : TclClass("Scheduler/Splay") {}
        TclObject* create(int /* argc */, const char*const* /* argv */) {
                return (new SplayScheduler);
        }
} class_splay_sched;

#define ROTATE_RIGHT(t, x)		\
    do {				\
    	left(t) = right(x);	 	\
    	right(x) = (t);			\
    	(t) = (x);			\
    } while(0)

#define ROTATE_LEFT(t, x)		\
    do {				\
    	right(t)  = left(x);	 	\
    	left(x) = (t);			\
    	(t) = (x);			\
    } while(0)
#define LINK_LEFT(l, t)			\
    do {				\
	right(l) = (t);			\
	(l) = (t);			\
	(t) = right(t);			\
    } while (0)
#define LINK_RIGHT(r, t)		\
    do {				\
	left(r) = (t);			\
	(r) = (t);			\
	(t) = left(t);			\
    } while (0)

#define FULL_ZIG_ZAG 0 		/* if true, perform full zig-zags, if
				 * false, simple top-down splaying */

void
SplayScheduler::insert(Event *n) 
{
	Event *l;	// bottom right in the left tree
	Event *r;	// bottom left in the right tree
	Event *t;	// root of the remaining tree
	Event *x;   	// current node
    
	++qsize_;

	double time = n->time_;
    
	if (root_ == 0) {
		left(n) = right(n) = 0;
		root_ = n;
		//validate();
		return;
	}
	t = root_;
	root_ = n;	// n is the new root
	l = n;
	r = n;
	for (;;) {
		if (time < t->time_) {
			x = left(t);
			if (x == 0) {
				left(r) = t;
				right(l) = 0;
				break;
			}
			if (time < x->time_) {
				ROTATE_RIGHT(t, x);
				LINK_RIGHT(r, t);
			} else {
				LINK_RIGHT(r, t);
#if FULL_ZIG_ZAG
				if (t != 0) 
					LINK_LEFT(l, t);
#endif
			}
			if (t == 0) {
				right(l) = 0;
				break;
			}
		}
		if (time >= t->time_) {
			x = right(t);
			if (x == 0) {
				right(l) = t; 
				left(r) = 0;
				break;	
			}
			if (time >= x->time_) {
				ROTATE_LEFT(t, x);
				LINK_LEFT(l, t);
			} else {
				LINK_LEFT(l, t);
#if FULL_ZIG_ZAG
				if (t != 0)
					LINK_RIGHT(r, t);
#endif
			}
			if (t == 0) {
				left(r) = 0;
				break;
			}
		}
	} /* for (;;) */

	// assemble:
	//   swap left and right children
	x = left(n);
	left(n) = right(n);
	right(n) = x;
	//validate();
}

Event *
SplayScheduler::deque()
{
	Event *t;
	Event *l;
	Event *ll;
	Event *lll;

	if (root_ == 0)
		return 0;

	--qsize_;

	t = root_;
	l = left(t);

	if (l == 0) {		// root is the element to dequeue
		root_ = right(t);	// right branch becomes the root
		//validate();
		return t;
	}
	for (;;) { 
		ll = left(l);
		if (ll == 0) {
			left(t) = right(l);
			//validate();
			return l;
		}

		lll = left(ll);
		if (lll == 0) {
			left(l) = right(ll);
			//validate();
			return ll;
		}

		// zig-zig: rotate l with ll
		left(t) = ll;
		left(l) = right(ll);
		right(ll) = l;

		t = ll;
		l = lll;
	}
} 

void 
SplayScheduler::cancel(Event *e)
{

	if (e->uid_ <= 0) 
		return; // event not in queue

	Event **t;
	//validate();
	if (e == root_) {
		t = &root_;
	} else {
		// searching among same-time events is a real bugger,
		// all because we don't have a parent pointer; use
		// uid_ to resolve conflicts.
		for (t = &root_; *t;) {
			t = ((e->time_ > (*t)->time_) || 
			     ((e->time_ == (*t)->time_) && e->uid_ > (*t)->uid_))
				? &right(*t) : &left(*t);
			if (*t == e)
				break;
		}
		if (*t == 0) {
			fprintf(stderr, "did not find it\n");
			abort(); // not found
		}
	}
	// t is the pointer to e in the parent or to root_ if e is root_
	e->uid_ = -e->uid_;
	--qsize_;

	if (right(e) == 0) {
		*t = left(e);
		left(e) = 0;
		//validate();
		return;
	} 
	if (left(e) == 0) {
		*t = right(e);
		right(e) = 0;
		//validate();
		return;
	}

	// find successor
	Event *p = right(e), *q = left(p);

	if (q == 0) {
		// p is the sucessor
		*t = p;
		left(p) = left(e);
		//validate();
		return;
	}
	for (; left(q); p = q, q = left(q)) 
		;
	// q is the successor
	// p is q's parent
	*t = q;
	left(p) = right(q);
	left(q) = left(e);
	right(q) = right(e);
	right(e) = left(e) = 0;
	//validate();
}


Event *
SplayScheduler::lookup(scheduler_uid_t uid) 
{
	lookup_uid_ = uid;
	return uid_lookup(root_);
}

Event *
SplayScheduler::uid_lookup(Event *root)
{
	if (root == 0)
		return 0;
	if (root->uid_ == lookup_uid_)
		return root;

	Event *res = uid_lookup(left(root));
 
	if (res) 
		return res;

	return uid_lookup(right(root));
}

int
SplayScheduler::validate(Event *root) 
{
	int size = 0;
	if (root) {
		++size;
		assert(left(root) == 0 || root->time_ >= left(root)->time_);
		assert(right(root) == 0 || root->time_ <= right(root)->time_);
		size += validate(left(root));
		size += validate(right(root));
		return size;
	}
	return 0;
}
