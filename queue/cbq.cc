/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	Group at Lawrence Berkeley National Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/cbq.cc,v 1.4 1997/04/02 04:38:41 kfall Exp $ (LBL)";
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
 * link-sharing.  (algorithm=3 for the old version of Formal link-sharing).
 *
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
 * This version includes support for an arbitrary queueing discipline
 * to be associated with each CBQ class.
 *
 *   The parameter "depth" indicates the "depth" of the class in
 * the class structure, used by Top-Level link-sharing.  Leaf classes have
 * "depth=0".
 *
 *   The parameter "allotment" indicates the link-sharing bandwidth
 * allocated to the class, as a fraction of the link bandwidth.
 *   Parameters "maxidle" and "extradelay" are used in
 * calculating the bandwidth used by the class.  The parameter "minidle"
 * has been removed.  Definitions and
 * guidelines for setting these parameters are in the draft paper
 * at URL ftp://ftp.ee.lbl.gov/papers/params.ps.Z.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>

#include "connector.h"
#include "queue.h"
#include "packet.h"
#include "trace.h"
#include "scheduler.h"

#define MAXDEPTH 	32	/* max depth of the class tree */
#define MAXPRIO 	8	/* number of priorities in the scheduler */
#define POWEROFTWO	16	/* for calculating avgidle */

class CBQueue;
class CBQClass;
typedef CBQClass* (CBQClass::*LinkageMethod)();

/*
 * a CBQ Class object: holds state about allocations of a particular
 * CBQ class and includes a reference to the actual Queue underneath
 * (e.g. red or drop-tail)
 *
 * general arrangement:
 *
 *		    CBQClass-\
 *		   /	      \
 * --->[classifier]-CBQClass---+>Queue/CBQueue
 *		   \	      /
 *		    CBQClass-/
 */
class CBQClass : public Connector {
    public:
	friend class CBQueue;
	void recv(Packet*, Handler*);	// from classifier
	Packet* nextpkt() { return (qdisc_->deque()); }
	CBQClass();
	virtual int command(int argc, const char*const* argv);
	inline CBQClass* parent() const { return (parent_); }
	inline CBQClass* borrow() const { return (borrow_); }
	inline Queue* qdisc() { return (qdisc_); }
	inline CBQueue* cbq() { return (cbq_); }

	void traverse(LinkageMethod);
	void newallot(double);	/* give this class a new link-share */

    protected:
	CBQClass* peer_;	/* circ linked list of peers at same prio */
	int       bytes_alloc_;	/* num. of bytes allocated for current round */
	Queue *   qdisc_;	/* underlying per-class q-ing discipline */
	int	  pcount_;	/* packet cnt; current queue len for class */
	CBQueue*  cbq_;		/* ref to corresponding CBQueue object */

	CBQClass* next_;	/* linked list for all classes */
	int       mark_;	/* for walking graph [not used now] */
	double    last_;	/* last time this class completed a pkt xmit */
	double	  undertime_;	/* time class will become unsat <?> */
	double	  avgidle_;	/* EWMA of idle (see below) */
	int	  pri_;		/* priority [0..MAXPRIO] */
	int	  depth_;	/* "depth" of this class in class tree */
	double	  allotment_;   /* this class' fraction of link bandwidth */
	double    maxidle_;	/* upper bound for avgidle */
	double	extradelay_;	/* 'offtime'-time overlim class waits to snd */
	double	maxrate_;	/* bytes per second allowed this class */
	int	delayed_;	/* 1 for an already-delayed class */
	CBQClass	*parent_;
	CBQClass	*borrow_;
};

static class CBQClassClass : public TclClass {
public:
	CBQClassClass() : TclClass("CBQClass") { }
        TclObject* create(int argc, const char*const* argv) {
                return (new CBQClass);
        }
} class_cbq_class;

/*
 * the CBQ link sharing machinery is incorporates into one Queue
 * object.  Does packet-by-packet RR with priority between the
 * queues.
 */

class CBQueue : public Queue {
    public:	
	CBQueue();
	virtual int command(int argc, const char*const* argv);
	Packet	*deque();
	void	enque(Packet* pkt);
	void	recv(Packet *, Handler*);
	void	addallot(int, double);	// change allotment at some pri level
	double	linkbw() { return (link_bw_); }
    protected:
	virtual void insert_class(CBQClass*);	// WRR and PKT-by-PKT RR
	void	delay_action(CBQClass*);
	int	ancestor(CBQClass *cl, CBQClass *p);
	int	satisfied(CBQClass *cl);
	int	ok_to_borrow(CBQClass *cl);	// formal LS
	int 	ok_to_borrow3(CBQClass *cl);	// old formal LS
	int	under_limit(CBQClass *cl, double now);
	void	plot(CBQClass *cl, double idle);
	void	update_class_util(CBQClass *cl, double now, int pktsize);
	void	check_for_cycles(LinkageMethod);

	CBQClass* active_[MAXPRIO];	/* active class at each level */
	int	  num_[MAXPRIO];	/* number of classes at each prio lev */
	CBQClass* borrowed_;		/* the class last borrowed from */
	CBQClass* list_;		/* beginning of list of all classes */
	int	maxpkt_;	/* max packet size in bytes */
	int	maxdepth_;	/* max depth at which borrowing is allowed */
	int	algorithm_;	/* 1 to restrict borrowing, 0 otherwise */
				/* 2 for idealized link-sharing guidelines */
	double	link_bw_;		/* total link bandwidth */
	double	alloc_[MAXPRIO];	/* total allocation at each level, 
					 *for WRR */
};

/*
 * a weighted-round-robin version of the CBQueue class
 */

class WRR_CBQueue : public CBQueue {
public:
	WRR_CBQueue();
protected:
	Packet* deque(void);
	virtual void insert_class(CBQClass*);
	int npri(int pri);
	void setM();
	double	M_[MAXPRIO];	/* for weighted RR, using num[] and alloc[] */
};

static class PBPCBQueueClass : public TclClass {
public:
	PBPCBQueueClass() : TclClass("Queue/CBQ/PBP") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new CBQueue);
        }
} class_cbq_pbp;

static class WRRCBQueueClass : public TclClass {
public:
	WRRCBQueueClass() : TclClass("Queue/CBQ/WRR") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new WRR_CBQueue);
        }
} class_cbq_wrr;


/*********************** CBQClass member functions *************************/

CBQClass::CBQClass() : peer_(0), bytes_alloc_(0), qdisc_(0), next_(0), mark_(0),
	last_(0.0), undertime_(0.0), avgidle_(0.0), delayed_(0),
	parent_(0), borrow_(0), cbq_(0), pcount_(0)

{
	bind("priority_", &pri_);		/* this class' priority */
	bind("depth_", &depth_);		/* this class' depth in tree */
	bind("allotment_", &allotment_);	/* frac of link allotment */
	bind("maxidle_", &maxidle_);		/* max idle bound */
	bind("extradelay_", &extradelay_);	/* add extra delay */

	/*XXX*/
	if (pri_ < 0 || pri_ > MAXPRIO)
		abort();
}

/*
 * set up a new allotment for this cbq class
 * let the cbq queue object know about the change
 */

void
CBQClass::newallot(double bw)
{
	maxrate_ = bw * ( cbq_->linkbw() / 8.0 );
	double diff = allotment_ - bw;
	allotment_ = bw;
	cbq_->addallot(pri_, diff);
	return;
}

/* 
 *
 * $class cbq
 * $class qdisc
 *
 * $class1 parent $class2
 * $class1 borrow $class2
 * $class1 qdisc $class2
 * $class1 allot $newbw
 */
int CBQClass::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
                if (strcmp(argv[1], "cbq") == 0) {
                        tcl.resultf("%s", cbq_->name());
                        return(TCL_OK);
                }
                if (strcmp(argv[1], "qdisc") == 0) {
                        tcl.resultf("%s", qdisc_->name());
                        return (TCL_OK);
                }
	} else if (argc == 3) {
                if (strcmp(argv[1], "parent") == 0) {
                        parent_ = (CBQClass*)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "borrow") == 0) {
                        borrow_ = (CBQClass*)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "qdisc") == 0) {
                        qdisc_ = (Queue*) TclObject::lookup(argv[2]);
                        if (qdisc_ != NULL)
                                return (TCL_OK);
                        return (TCL_ERROR);
                }
                if (strcmp(argv[1], "allot") == 0) {
                        double bw = atof(argv[2]);
                        if (bw < 0)
                                return (TCL_ERROR);
                        newallot(bw);
                        return (TCL_OK);
                }
	}
	return (Connector::command(argc, argv));
}

void CBQClass::traverse(LinkageMethod method)
{
	if (mark_)
		return;
	mark_ = -1;
	CBQClass* target = (this->*method)();
	if (target != 0) {
		traverse(method);
		mark_ = target->mark_;
	}
}

/*********************** CBQueue member functions *************************/

CBQueue::CBQueue() : borrowed_(0), list_(0), maxdepth_(MAXDEPTH)
{
	bind("algorithm_", &algorithm_);
	bind("max-pktsize_", &maxpkt_);
	bind("bandwidth_", &link_bw_);

	memset(active_, '\0', sizeof(active_));
	memset(num_, '\0', sizeof(num_));
	memset(alloc_, '\0', sizeof(alloc_));
}

/*
 * This implements the following tcl commands:
 *  $cbq insert $class
 *  $cbq bind $class $classID
 *  $cbq bind $class $first $last
 */
int CBQueue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "insert") == 0) {
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[2]);
			insert_class(p);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		/* $cbq bind $class $flow */
		if (strcmp(argv[1], "bind") == 0) {
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[2]);
			int id = atoi(argv[3]);
			/*XXX check id range */
abort(); // need to deal with classifier?
			return (TCL_OK);
		}
	} else if (argc == 5) {
		/* $cbq bind $class n1 nk */
		if (strcmp(argv[1], "bind") == 0) {
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[2]);
			int firstid = atoi(argv[3]);
			int lastid = atoi(argv[4]);
			/*XXX check id range */
			register i;
			for (i=firstid; i <= lastid; i++) {
abort();
			}
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

/*
 * Add a class to this cbq object.
 */
void CBQueue::insert_class(CBQClass* p)
{
	/*
	 * Add to list of all classes
	 */
	p->next_ = list_;
	list_ = p;
	p->cbq_ = this;

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
	p->maxrate_ = p->allotment_ * ( linkbw() / 8.0 );

	/*
	 * Check that parent and borrow linkage are acyclic.
	 */
#ifdef notdef
	check_for_cycles(CBQClass::parent);
	check_for_cycles(CBQClass::borrow);
#endif
}

void CBQueue::addallot(int priority, double allotdiff)
{
	alloc_[priority] += allotdiff;
}

/* 
 * Delay_action sets cl->undertime.  Delay_action is only called
 * for a class that is overlimit and unable to borrow.
 *
 * The variable "cl->delayed_" is checked so that "extradelay_" is only
 * added one time before the class next gets to send a packet.
*/
void CBQueue::delay_action(CBQClass *cl)
{
	CBQClass	*p, *head, *orig_cl;
	double	now = Scheduler::instance().clock();
	double	delay;

	delay = cl->undertime_ - now + cl->extradelay_;
	if (delay > 0 && !cl->delayed_) {
		// only add "extradelay_" one time 
		cl->undertime_ += cl->extradelay_;
		// when class is regulated, don't penalize it for
		// having used more than its allocated bandwidth in
		// the past.
		cl->undertime_ -= (1-POWEROFTWO) * cl->avgidle_;
		cl->delayed_ = 1;
	}
}

/*
 * Ancestor returns 1 if class cl is an ancestor of class p. 
 *	Search up the tree following 'borrow' links for the answer.
 * [This is used only for algorithm_ == 2 or 3.]
 */
int CBQueue::ancestor(CBQClass* cl, CBQClass* p)
{
	if (cl == p->borrow())
		return (1);
	else while (p->borrow() != 0) {
		p = p->borrow();
		if (cl == p->borrow())
			return (1);
	}
	return (0);
}

/*
 * Satisfied returns 1 if the class is satisfied.
 * [This is used only for algorithm == 2 or 3.]
 */
int CBQueue::satisfied(CBQClass* cl)
{
	CBQClass	*p;
	double	now = Scheduler::instance().clock();

	if (cl->undertime_ > now)
		return (1);
	else if (cl->depth_ == 0) {
		if (cl->pcount_ > 0)
			return (0);
		else
			return (1);
	} else {
		for (p = list_; p != 0; p = p->next_) {
			if (p->depth_ == 0 && ancestor(cl, p)) {
				if (p->pcount_ > 0)
					return (0);
			}
		}
	}
	return (1);
}

/*
 * ok_to_borrow() returns 1 if it is ok for a descendant to borrow
 * from this underlimit interior class cl.
 * It is ok to borrow if there are no unsatisfied classes at a lower level.
 * This is for the new version of Formal link-sharing.
 * Put another way: if there are any unsatisfied classes below this class
 * then it's not ok to borrow.
 */
int CBQueue::ok_to_borrow(CBQClass* cl)
{
	CBQClass *p;
	for (p = list_; p != 0; p = p->next_)
		if ((p->depth_ < cl->depth_) && !satisfied(p))
			return (0);
	return (1);
}

/*
 * ok_to_borrow3() returns 1 if it is ok for a descendant to borrow
 * from this underlimit interior class cl.
 * It is ok to borrow if there are no unsatisfied descendants.
 * This is used only for algorithm==3, for the older version of 
 * Formal link-sharing.
 */
int CBQueue::ok_to_borrow3(CBQClass* cl)
{
	CBQClass *p;
	for (p = list_; p != 0; p = p->next_)
		if (ancestor(cl,p) && !satisfied(p))
			return (0);
	return (1);
}

/*
 * Under_limit tests whether a class is either underlimit
 * or able to borrow.  
 */
int CBQueue::under_limit(CBQClass* cl, double now)
{
	if (cl->undertime_ < now) {
		// our time to send has already passed
		cl->delayed_ = 0;
	} else if (algorithm_ == 0) {
		// ancestor-only
		while (cl->undertime_ > now ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	} else if (algorithm_ == 1) {
		// top-level
		while (cl->undertime_ > now) {
			cl = cl->borrow();
			if (cl == 0 || cl->depth_ > maxdepth_) 
				return (0);
		}
	} else if (algorithm_ == 2) {
		// formal
		while (cl->undertime_ > now || !ok_to_borrow(cl) ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	} else if (algorithm_ == 3) {
		// (old formal)
		while (cl->undertime_ > now || !ok_to_borrow3(cl) ) {
			cl = cl->borrow();
			if (cl == 0) 
				return (0);
		}
	}
	borrowed_ = cl;		// who we borrowed from (may be us)
	return (1);
}

/* Plot needs to be implemented still. */ 
void CBQueue::plot(CBQClass* cl, double idle)
{
#ifdef notdef
	double t = Scheduler::instance().clock();
   	sprintf(trace_->buffer(), "%g avg %g\n", t, cl->avgidle_);
	trace_->dump();
	sprintf(trace_->buffer(), "%g idl %g\n", t, idle);
	trace_->dump();
#endif
}

/*
 * Update_class_util calculates "idle" and "avgidle", and updates "undertime",
 * immediately after a packet is sent from a class.  Update_class_util
 * is only called for a class that is underlimit or able to borrow.
 *
 * Idle indicates the difference between the desired packet inter-departure
 * time for the class and the actual measured value (desired - actual).
 * Thus, idle is <0 when a class is exceeding its allocated banwidth over
 * this interval.
 *
 * Avgidle is a EWMA of Idle, and if it is <0 indicates the class is
 * overlimit.
 *
 * Update_class_util also updates the "depth" parameter:  if this class
 * could send another packet, borrowing from depth at most 
 * "borrowed->depth", then "depth" is set to
 * at most "borrowed->depth".  
 */
void CBQueue::update_class_util(CBQClass* cl, double now, int pktsize)
{
	int queue_length; 
	double txt = pktsize * 8. / bandwidth();	// xmit time
	now += txt;				// after this pkt is sent
	queue_length = cl->pcount_;

	// walk up to root of class tree, and determine
	do {
		double idle = (now - cl->last_) - (pktsize / cl->maxrate_);
		double avgidle = cl->avgidle_;
		// avgidle = (POWEROFTWO-1)/POWEROFTWO avgidle +
		//      1/POWEROFTWO idle
		avgidle += (idle - avgidle) / POWEROFTWO;
		if (avgidle > cl->maxidle_)
			avgidle = cl->maxidle_;
		cl->avgidle_ = avgidle;
		if (avgidle <= 0) {
			// class is overlimit
			cl->undertime_ = now + txt * (1 / cl->allotment_ - 1); 
			cl->undertime_ += (1-POWEROFTWO) * avgidle;
		}
		cl->last_ = now;
	} while (cl = cl->parent());

	if (algorithm_ == 1 && maxdepth_ >= borrowed_->depth_) {
		// for top-level only
		if ((queue_length <= 1) || (borrowed_->undertime_ > now))
			maxdepth_ = MAXDEPTH;
		else 
			maxdepth_ = borrowed_->depth_;
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
Packet*
CBQueue::deque()
{
	double now = Scheduler::instance().clock();
	CBQClass* first = 0;

	// strict priorities (priority 0 is most preferred)

	for (int i = 0; i < MAXPRIO; ++i) {
		CBQClass* cl = active_[i];
		if (cl == 0)
			continue;

		// pkt-by-pkt round robin loop at same prio
		// regulated by the LS alg in under_limit()
		do {
			if (cl->pcount_ > 0) {
				if (first == 0 && cl->borrow() != 0)
					first = cl;
				if (!under_limit(cl, now)) {
					/* not underlimit and can't borrow  */
					delay_action(cl);
				} else {
					/* under-limit or can borrow */
					/* so send and update stats */
					Packet* pkt = cl->nextpkt();
					hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
					update_class_util(cl, now, ch->size());
					active_[i] = cl->peer_;
					// could let qdisc do tracing, but
					// instead do it here
					return (pkt);
				}
			}
			cl = cl->peer_;	// next class at same prio
		} while (cl != active_[i]);
	}
	/*
	 * no class is underlimit or able to borrow, so first unbounded class
	 * with something to send gets to go (if there is such a class)
	 */
	maxdepth_ = MAXDEPTH;
	if (first != 0) {
		Packet* pkt = first->nextpkt();
		if (pkt) {
			active_[first->pri_] = first->peer_;
			return (pkt);
		}
	}
	return (0);
}

/*
 * main entrypoint for a packet:
 *	a classifier has already determined which class the packet
 *	belongs to, so we just need to do the appropriate stuff here
 */
void CBQClass::recv(Packet *p, Handler *h)

	double now = Scheduler::instance().clock();
	if (cbq_->algorithm_ == 1 && cbq_->maxdepth_ > 0) {
		if (undertime_ < now) 
			cbq_->maxdepth_ = 0;
		else if (cbq_->maxdepth_ > 1 && borrow_ != 0) {
			if (borrow_->undertime_ < now)
				cbq_->maxdepth_ = 1;
			}
	}
	if (qdisc_ == NULL) {
		fprintf(stderr, "CBQ: enque: no qdisc for pkt@%p\n",
			pkt);
		abort();
	}
	qdisc_->enque(p);
	pcount_++;
}

/*
 * Check that a linkage forms a forest.
 */
void CBQueue::check_for_cycles(LinkageMethod linkage)
{
	CBQClass *p;
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
	memset(M_, '\0', sizeof(M_));
}

/* counts the number of cbq classes at the specified priority */
int WRR_CBQueue::npri(int pri)
{
	int n = 0;
	CBQClass* first = active_[pri];
	if (first != 0) {
		CBQClass* p = first;
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
		if (alloc_[i] > 0) {
			/*
			M_[i] = npri(i) * maxpkt_ * 1.0/alloc_[i];
			*/
			M_[i] = num_[i] * maxpkt_ * 1.0/alloc_[i];
		} else {
			M_[i] = 0.0;
		}
	}
}

void WRR_CBQueue::insert_class(CBQClass* p)
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
Packet*
WRR_CBQueue::deque()
{
	register i;
	double now = Scheduler::instance().clock();
	hdr_cmn* ch;
	CBQClass* first = 0;
	for (i = 0; i < MAXPRIO; ++i) {
		/*
		 * Loop through twice for a priority level, if some class
		 * was unable to send a packet the first round because
		 * of the weighted round-robin mechanism.
		 * During the second loop at this level, deficit==2.
		 * (This second loop is not needed if for every class,
		 * "M[cl->pri_])" times "cl->allotment" is greater than
		 * the byte size for the largest packet in the class.)
		 */
		CBQClass* cl = active_[i];
		if (cl == NULL)
			continue;
		int deficit = 0;
		int done = 0;
		while (!done) {
			do {
				if (deficit < 2 && cl->bytes_alloc_ <= 0) 
					cl->bytes_alloc_ +=
					  (int)(cl->allotment_ * M_[cl->pri_]);
				if (cl->pcount_ > 0) {
					if (first == 0 && cl->borrow() != 0)
						first = cl;
					if (!under_limit(cl, now))
					 	delay_action(cl);
					else {
						if (cl->bytes_alloc_ > 0 ||
						    deficit > 1) {
							Packet* pkt =
							   cl->nextpkt();
							ch = (hdr_cmn*)pkt->access(off_cmn_);
							if (cl->bytes_alloc_ > 0)
								cl->bytes_alloc_ -= ch->size();
							update_class_util(cl, now, ch->size());
							if (cl->bytes_alloc_ <= 0)
								active_[i] = cl->peer_;
							else
								active_[i] = cl;
							return (pkt);
						} else {
							deficit = 1;
						}
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
	maxdepth_ = MAXDEPTH;
	if (first != 0) {
		Packet* pkt = first->nextpkt();
		if (pkt) {
			active_[first->pri_] = first->peer_;
			return (pkt);
		}
	}
	return (0);
}
