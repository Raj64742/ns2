/*
 * Copyright (c) 1991-1995 Regents of the University of California.
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
 *	This product includes software developed by the Network Research
 *	Group at Lawrence Berkeley Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/cbq.cc,v 1.1 1996/12/19 03:22:44 mccanne Exp $ (LBL)";
#endif

/*
 * CBQ simulations can be run from "test-all-cbq" or from 
 * "test-suite-cbq.tcl".   
 * 
 * We have made no attempt to produce efficient code; our goal has simply
 * been to write code that correctly implements the many algorithms
 * that we have been experimenting with.  This code has NO
 * relationship to production code that would be used to implement any of 
 * these resource management algorithms in routers.
 * 
 *   The parameter "algorithm" chooses one of three different
 * link-sharing algorithms:  algorithm=0 for Ancestor-Only link-sharing,
 * algorithm=1 for Top-Level link-sharing, and algorithm=2 for Formal
 * link-sharing.  
 *   The class CBQueue uses packet-by-packet round-robin, and
 * the class WRR_CBQueue uses weighted round-robin.  (The
 * implementation of weighted round-robin could use a great deal of
 * improvement.)
 * 
 * Parameters for a particular class:
 * 
 *   The parameters "parent" and "borrow" indicate the parent class
 * in the class structure, and the class to borrow bandwidth from.
 * Generally these two parameters will be the same.
 *   Currently only the Drop-Tail queueing discipline is supported,
 * but we will add support for RED queueing.
 *   The parameter "depth" indicates the "depth" of the class in
 * the class structure, used by Top-Level link-sharing.  Leaf classes have
 * "depth=0".
 *   The parameter "allotment" indicates the link-sharing bandwidth
 * allocated to the class, as a fraction of the link bandwidth.
 *   Parameters "maxidle", "minidle", and "extradelay" are used in
 * calculating the bandwidth used by the class.  Guidelines for
 * setting these parameters are in the draft paper
 * at URL ftp://ftp.ee.lbl.gov/papers/params.ps.Z.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>

#include "agent.h"
#include "node.h"
#include "Tcl.h"
#include "packet.h"
#include "link.h"
#include "scheduler.h"

#define MAXDEPTH 	32
#define MAXPRIO 	8
#define MAXCLASS	256
#define POWEROFTWO	16	/* for calculating avgidle */

class CBQueue;

class Class;

typedef Class* (Class::*LinkageMethod)();

class Class : public NsObject {
public:
	Class();
	virtual int command(int argc, const char*const* argv);
	Class* parent() const { return (parent_); }
	Class* borrow() const { return (borrow_); }
	inline int qlen() const { return (q_.length()); }
	void traverse(LinkageMethod);
	Class		*peer_;
	int bytes_alloc_;	      /* num. of bytes allocated for current round */
	Queue		q_;
	Class		*next_;
	int		mark_;
	double		last_;
	double		undertime_;
	double		avgidle_;
	int		pri_;	/* priority */
	int		depth_;	/* "depth" of class in class tree */
	double		allotment_; /* fraction of link bandwidth */
	double		maxidle_;
	double		minidle_;
	double		extradelay_;
	double		maxrate_;     /* bytes per second allowed this class */
	int		delayed_;     /* 1 for an already-delayed class */
	int		plot_;		/* to plot for debugging */
protected:
	Class		*parent_;
	Class		*borrow_;
};

static class ClassMatcher : public Matcher {
public:
	ClassMatcher() : Matcher("class") {}
	TclObject* match(const char* id) {
		return (new Class);
	}
} matcher_class;

class CBQueue : public Link {
public:	
	CBQueue();
	virtual int command(int argc, const char*const* argv);
	Packet	*deque(void);
	void	enque(Packet *pkt);
protected:
	virtual void insert_class(Class*);
	void	delay_action(Class*);
	int	ancestor(Class *cl, Class *p);
	int	satisfied(Class *cl);
	int	ok_to_borrow(Class *cl);
	int 	ok_to_borrow3(Class *cl);
	int	under_limit(Class *cl, double now);
	void	plot(Class *cl, double idle);
	void	update_class_util(Class *cl, double now, int pktsize);

	void check_for_cycles(LinkageMethod);

	Class	*active_[MAXPRIO];	/* active class at each level */
	Class	*borrowed_;		/* the class last borrowed from */
	Class	*classtab_[MAXCLASS];	/* class of newly-arrived pkts */
	Class	*list_;			/* beginning of list of classes */
	int	maxpkt_;	/* max packet size in bytes */
	int	depth_;		/* max depth at which borrowing is allowed */
	int	algorithm_;	/* 1 to restrict borrowing, 0 otherwise */
				/* 2 for idealized link-sharing guidelines */
	int	num_[MAXPRIO];	/* number of classes at each priority level */
	double	alloc_[MAXPRIO];	/* total allocation at each level, 
					 *for WRR */
};

class WRR_CBQueue : public CBQueue {
public:
	WRR_CBQueue();
protected:
	Packet* deque(void);
	virtual void insert_class(Class*);
	int npri(int pri);
	void setM();
	double	M_[MAXPRIO];	/* for weighted RR, using num[] and alloc[] */
};

static class CBQueueMatcher : public Matcher {
public:
	CBQueueMatcher() : Matcher("link") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "cbq") == 0)
			return (new CBQueue);
		if (strcasecmp(id, "wrr-cbq") == 0)
			return (new WRR_CBQueue);
		return (0);
	}
} matcher_cbq;

Class::Class()
{
	/* Does this line belong anywhere?  
	Tcl& tcl = Tcl::instance();
	*/
	link_int("ns_class", "priority", &pri_, 0);
	link_int("ns_class", "depth", &depth_, 0);
	link_int("ns_class", "plot", &plot_, 0);
	link_real("ns_class", "allotment", &allotment_, 0);
	link_real("ns_class", "maxidle", &maxidle_, 0);
	link_real("ns_class", "minidle", &minidle_, 0);
	link_real("ns_class", "extradelay", &extradelay_, 0);

	last_ = 0;	
        undertime_ = 0;
        avgidle_ = 0;
	delayed_ = 0;
	bytes_alloc_ = 0;

	parent_ = 0;
	borrow_ = 0;
	peer_ = 0;

	/*XXX*/
	if (pri_ < 0 || pri_ > MAXPRIO)
		abort();
}

/* 
 * $class1 parent $class2
 * $class1 borrow $class2
 */
int Class::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "parent") == 0) {
			parent_ = (Class*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "borrow") == 0) {
			borrow_ = (Class*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (NsObject::command(argc, argv));
}

void Class::traverse(LinkageMethod method)
{
	if (mark_)
		return;
	mark_ = -1;
	Class* target = (this->*method)();
	if (target != 0) {
		traverse(method);
		mark_ = target->mark_;
	}
}

CBQueue::CBQueue()
	: borrowed_(0), list_(0)
{
	link_int("ns_cbq", "algorithm", &algorithm_, 0);
	link_int("ns_cbq", "max-pktsize", &maxpkt_, 1024);

	depth_ = MAXDEPTH;
	int i;
	for (i = 0; i < MAXPRIO; ++i) {
		active_[i] = 0;
		num_[i] = 0;
		alloc_[i] = 0.0;
	}
	for (i = 0; i < MAXCLASS; ++i)
		classtab_[i] = 0;
}

/*
 * This implements the following tcl commands:
 *  $cbq insert $class
 *  $cbq bind $class $classID
 */
int CBQueue::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "insert") == 0) {
			Class* p = (Class*)TclObject::lookup(argv[2]);
			insert_class(p);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "bind") == 0) {
			Class* p = (Class*)TclObject::lookup(argv[2]);
			int id = atoi(argv[3]);
			/*XXX check id range */
			classtab_[id] = p;
			return (TCL_OK);
		}
	}
	return (Link::command(argc, argv));
}

/*
 * Add a class to this cbq object.
 */
void CBQueue::insert_class(Class* p)
{
	p->next_ = list_;
	list_ = p;

	/*
	 * Add to circularly-linked list of peers for the given priority.
	 */
	if (active_[p->pri_] != 0) {
		p->peer_ = active_[p->pri_]->peer_;
		active_[p->pri_]->peer_ = p;
	} else {
		p->peer_ = p;
		active_[p->pri_] = p;
	}
	++num_[p->pri_];
	alloc_[p->pri_] += p->allotment_;

	/*
	 * Compute maxrate from allotment.
	 */
	p->maxrate_ = p->allotment_ * ( bandwidth_ / 8 );

	/*
	 * Check that parent and borrow linkage are acyclic.
	 */
#ifdef notdef
	check_for_cycles(Class::parent);
	check_for_cycles(Class::borrow);
#endif
}

/* 
 * Delay_action sets cl->undertime.
*/
void CBQueue::delay_action(Class *cl)
{
	Class	*p, *head, *orig_cl;
	double	now = Scheduler::instance().clock();
	double	delay;

	delay = cl->undertime_ - now + cl->extradelay_;
	if (delay > 0 && !cl->delayed_) {
		cl->undertime_ += cl->extradelay_;
		/*XXX THIS CAN"T BE RIGHT! 1-POWEROFTWO == -15 */
		cl->undertime_ -= (1-POWEROFTWO) * cl->avgidle_;
		cl->delayed_ = 1;
	}
}

/*
 * Ancestor returns 1 if class cl is an ancestor of class p. 
 * This is used only for algorithm_ ==2 or 3.
 */
int CBQueue::ancestor(Class* cl, Class* p)
{
	if (cl == p->borrow() ) return (1);
	else while (p->borrow() != 0) {
		p = p->borrow();
		if (cl == p->borrow() ) return (1);
	}
	return (0);
}

/* Satisfied returns 1 if the class is satisfied.
 * This is used only for algorithm==2 or 3.
 */
int CBQueue::satisfied(Class* cl)
{
	Class	*p;
	double	now = Scheduler::instance().clock();

	if (cl->undertime_ > now)
		return (1);
	else if (cl->depth_ == 0) {
		if (cl->qlen() > 0)
			return (0);
		else
			return (1);
	} else {
		for (p = list_; p != 0; p = p->next_) {
			if (p->depth_ == 0 && ancestor(cl, p)) {
				if (p->qlen() > 0)
					return (0);
			}
		}
	}
	return (1);
}

/* Ok_to_borrow returns 1 if it is ok for a descendant to borrow
 * from this underlimit interior class cl.
 * It is ok to borrow if there are no unsatisfied classes at a lower level.
 * This is for the new version of Formal link-sharing.
 */
int CBQueue::ok_to_borrow(Class* cl)
{
	for (Class* p = list_; p != 0; p = p->next_)
		if ((p->depth_ < cl->depth_) && !satisfied(p))
			return (0);
	return (1);
}

/* Ok_to_borrow3 returns 1 if it is ok for a descendant to borrow
 * from this underlimit interior class cl.
 * It is ok to borrow if there are no unsatisfied descendants.
 * This is used only for algorithm==3, for the older version of 
 * Formal link-sharing.
 */
int CBQueue::ok_to_borrow3(Class* cl)
{
	for (Class* p = list_; p != 0; p = p->next_)
		if (ancestor(cl,p) && !satisfied(p))
			return (0);
	return (1);
}

/*
 * Under_limit tests whether a class is either underlimit
 * or able to borrow.  
 */
int CBQueue::under_limit(Class* cl, double now)
{
	if (cl->undertime_ < now)
		cl->delayed_ = 0;
	else if (algorithm_ == 0) {
		while (cl->undertime_ > now ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	} else if (algorithm_ == 1) {
		while (cl->undertime_ > now) {
			cl = cl->borrow();
			if (cl == 0 || cl->depth_ > depth_) 
				return (0);
		}
	} else if (algorithm_ == 2) {
		while (cl->undertime_ > now || !ok_to_borrow(cl) ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	} else if (algorithm_ == 3) {
		while (cl->undertime_ > now || !ok_to_borrow3(cl) ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	}
	borrowed_ = cl;
	return (1);
}

/* Plot needs to be implemented still. */ 
void CBQueue::plot(Class* cl, double idle)
{
	double t = Scheduler::instance().clock();
   	sprintf(trace_->buffer(), "%g avg %g\n", t, cl->avgidle_);
	trace_->dump();
	sprintf(trace_->buffer(), "%g idl %g\n", t, idle);
	trace_->dump();
}

/*
 * Update_class_util calculates "idle" and "avgidle", and updates "undertime",
 * immediately after a packet is sent from a class.
 * Update_class_util also updates the "depth" parameter:  if this class
 * could send another packet, borrowing from depth at most 
 * "borrowed->depth", then "depth" is set to
 * at most "borrowed->depth".  
 */
void CBQueue::update_class_util(Class* cl, double now, int pktsize)
{
	int queue_length; 
	double txt = pktsize * 8. / bandwidth_;
	now += txt;
	queue_length = cl->qlen(); 
	do {
		double idle = (now - cl->last_) - (pktsize / cl->maxrate_);
		double avgidle = cl->avgidle_;
		avgidle += (idle - avgidle) / POWEROFTWO;
		if (avgidle > cl->maxidle_)
			avgidle = cl->maxidle_;
		cl->avgidle_ = avgidle;
		if (avgidle <= 0) {
			if (avgidle < cl->minidle_) 
				avgidle = cl->minidle_;
			cl->undertime_ = now + txt * (1 / cl->allotment_ - 1); 
			cl->undertime_ += (1-POWEROFTWO) * avgidle;
			if (cl->pri_ == MAXPRIO) 
				cl->undertime_ += cl->extradelay_;
		}
		cl->last_ = now;
#ifdef notyet
		if (plot_) 
			plot(cl, cl->idle);
#endif
	} while (cl = cl->parent());

	if (algorithm_ == 1 && depth_ >= borrowed_->depth_) {
		if ((queue_length <= 1) || (borrowed_->undertime_ > now))
			depth_ = MAXDEPTH;
		else 
			depth_ = borrowed_->depth_;
	}
}

/*
 * The heart of the scheduler, which decides which class
 * next gets to send a packet.  Highest priority first,
 * packet-by-packet round-robin within priorities.
 *
 * If the scheduler finds no class that is underlimit or able to borrow, then
 * the first class that had a nonzero queue and is allowed to borrow gets
 * to send.
 */
Packet* CBQueue::deque(void)
{
	double now = Scheduler::instance().clock();
	Class* first = 0;
	for (int i = 0; i < MAXPRIO; ++i) {
		Class* cl = active_[i];
		if (cl == 0)
			continue;
		do {
			if (cl->qlen() > 0) {
				if (first == 0 && cl->borrow() != 0)
					first = cl;
				if (!under_limit(cl, now))
					delay_action(cl);
				else {
					Packet* pkt = cl->q_.deque();
					update_class_util(cl, now, pkt->size_);
					active_[i] = cl->peer_;
					log_packet_departure(pkt);
					return (pkt);
				}
			}
			cl = cl->peer_;
		} while (cl != active_[i]);
	}
	/*
	 * no class is underlimit or able to borrow ...
	 */
	depth_ = MAXDEPTH;
	if (first != 0) {
		Packet* pkt = first->q_.deque();
		if (pkt) {
			active_[first->pri_] = first->peer_;
			log_packet_departure(pkt);
			return (pkt);
		}
	}
	return (0);
}

/*
 * Map arriving packets to the appropriate class, and add the 
 * arriving packet to the queue for that class.
 * If algorithm==1, and that class could send a packet borrowing 
 * from depth at most d, then "stp->depth" is set to no more than d.
 */
void CBQueue::enque(Packet* pkt)
{
	double	now = Scheduler::instance().clock();
	Class	*cl = classtab_[pkt->class_];
	log_packet_arrival(pkt);
	if (cl != 0 && algorithm_ == 1 && depth_ > 0) {
		if (cl->undertime_ < now) 
			depth_ = 0;
		else if (depth_ > 1 && cl->borrow() != 0) {
			if (cl->borrow()->undertime_ < now)
				depth_ = 1;
		}
	}
	if (cl == 0 || cl->q_.length() >= qlim_) {
		/*
		 *XXX should have a default best-effort class,
		 *XXX should have other queueing disciplines beside drop-tail
		 */
		log_packet_drop(pkt);
		Packet::free(pkt);
		return;
	}
	cl->q_.enque(pkt);
}

/*
 * Check that a linkage forms a forest.
 */
void CBQueue::check_for_cycles(LinkageMethod linkage)
{
	Class *p;
	for (p = list_; p != 0; p = p->next_)
		p->mark_ = ((p->*linkage)() == 0) ? 1 : 0;

	for (p = list_; p != 0; p = p->next_)
		p->traverse(linkage);

	for (p = list_; p != 0; p = p->next_)
		if (p->mark_ <= 0) 
			/*XXX*/
			fprintf(stderr, "class set has cycle");
}

WRR_CBQueue::WRR_CBQueue()
{
	for (int i = 0; i < MAXPRIO; ++i)
		M_[i] = 0.;
}

int WRR_CBQueue::npri(int pri)
{
	int n = 0;
	Class* first = active_[pri];
	if (first != 0) {
		Class* p = first;
		do {
			++n;
			p = p->peer_;
		} while (p != first);
	}
	return (n);
}

void WRR_CBQueue::setM()
{
	for (int i = 0; i < MAXPRIO; ++i) {
		if (alloc_[i] > 0)
			/*
			M_[i] = npri(i) * maxpkt_ * 1.0/alloc_[i];
			*/
			M_[i] = num_[i] * maxpkt_ * 1.0/alloc_[i];
		else
			M_[i] = 0.0;
	}
}

void WRR_CBQueue::insert_class(Class* p)
{
	CBQueue::insert_class(p);
	setM();
}

/*
 * The heart of the weighted round-robin scheduler, which decides which class
 * next gets to send a packet.  Highest priority first,
 * weighed round-robin within priorities.
 *
 * Each able-to-send class gets to send until
 * its byte allocation is exausted.  Thus the active pointer
 * is only changed after a class is finished sending its allocation.
 *
 * If the scheduler finds no class that is underlimit or able to borrow, then
 * the first class found that had a nonzero queue and is allowed to borrow
 * gets to send.
 */
Packet* WRR_CBQueue::deque(void)
{
	double now = Scheduler::instance().clock();
	Class* first = 0;
	for (int i = 0; i < MAXPRIO; ++i) {
		/*
		 * Loop through twice for a priority level, if some class
		 * was unable to send a packet the first round because
		 * of the weighted round-robin mechanism.
		 * During the second loop at this level, deficit==2.
		 * (This second loop is not needed if for every class,
		 * "M[cl->pri_])" times "cl->allotment" is greater than
		 * the byte size for the largest packet in the class.)
		 */
		Class* cl = active_[i];
		if (cl == 0)
			continue;
		int deficit = 0;
		int done = 0;
		while (!done) {
			do {
				if (deficit < 2 && cl->bytes_alloc_ <= 0) 
					cl->bytes_alloc_ += (int)(cl->allotment_ * M_[cl->pri_]);
				if (cl->qlen() > 0) {
					if (first == 0 && cl->borrow() != 0)
						first = cl;
					if (!under_limit(cl, now))
					 	delay_action(cl);
					else {
						if (cl->bytes_alloc_ > 0 || deficit > 1) {
							Packet* pkt = cl->q_.deque();
							if (cl->bytes_alloc_ > 0)
								cl->bytes_alloc_ -= pkt->size_;
							update_class_util(cl, now, pkt->size_);
							if (cl->bytes_alloc_ <= 0)
								active_[i] = cl->peer_;
							else
								active_[i] = cl;
							log_packet_departure(pkt);
							return (pkt);
						} else
							deficit = 1;
					}
				}
				cl->bytes_alloc_ = 0;
				cl = cl->peer_;
			/* XXX Why does cl end up as 0 for algorithm == 1? - SF. */
			} while (cl != active_[i] && cl != 0);

			if (deficit == 1) 
				deficit = 2;
			else
				done = 1;
		}
	}
	/*
	 * If no class is underlimit or able to borrow ...
	 */
	depth_ = MAXDEPTH;
	if (first != 0) {
		Packet* pkt = first->q_.deque();
		if (pkt) {
			active_[first->pri_] = first->peer_;
			return (pkt);
		}
	}
	return (0);
}
