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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/cbq.cc,v 1.2 1997/03/27 05:09:05 kfall Exp $ (LBL)";
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
 * This version includes the notion of "specialized" flows.  That is,
 * particular flow signatures (e.g. src/dst addr, port, classid, etc)
 * can received "special" treatment.  This is generally used for
 * penalizing the flow somehow because it has been using a large fraction
 * of the link bandwidth [and has failed to take steps to avoid congestion].
 * This is the reason for the references to 'penalty box' below.
 * 
 * Parameters for a particular class:
 * 
 *   The parameters "parent" and "borrow" indicate the parent class
 * in the class structure, and the class to borrow bandwidth from.
 * Generally these two parameters will be the same.
 * This version includes support for an arbitrary queueing discipline
 * to be associated with each CBQ class.  For now, the queueing discipline
 * is maintained as a "Link*" type in ns 1.x, but will evolve in the
 * 2.x releases.
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

#include "agent.h"
#include "node.h"
#include "Tcl.h"
#include "packet.h"
#include "link.h"
#include "scheduler.h"

#define MAXDEPTH 	32
#define MAXPRIO 	8	/* number of priorities in the scheduler */
#define POWEROFTWO	16	/* for calculating avgidle */
#define	MAXPBOX		2048	/* size of penalty box */
#define MAXCLASS	2048	/* # of different packet classes allowed
				   (see class_ field in packet.h) */

class CBQueue;
class CBQClass;
typedef CBQClass* (CBQClass::*LinkageMethod)();

/*
 * a CBQ Class object: holds state about allocations of a particular
 * CBQ class and includes a reference to the actual Queue underneath
 * (e.g. red or drop-tail)
 */
class CBQClass : public NsObject {
    public:
	CBQClass();
	virtual int command(int argc, const char*const* argv);
	inline CBQClass* parent() const { return (parent_); }
	inline CBQClass* borrow() const { return (borrow_); }
	void traverse(LinkageMethod);
	void newallot(double);

	inline Queue* qdisc() { return (qdisc_); }
	inline CBQueue* cbq() { return (cbq_); }

	CBQClass	*peer_;	/* circ linked list of peers at same prio */
	int	bytes_alloc_;	/* num. of bytes allocated for current round */
	Queue	*qdisc_;	/* underlying per-class q-ing discipline */
	CBQueue	*cbq_;		/* ref to corresponding CBQueue object */

	CBQClass* next_;	/* linked list for all classes */
	int	mark_;		/* for walking graph [not used now] */
	double	last_;		/* last time this class completed a pkt xmit */
	double	undertime_;	/* time class will become unsat <?> */
	double	avgidle_;	/* EWMA of idle (see below) */
	int	pri_;		/* priority [0..MAXPRIO] */
	int	depth_;		/* "depth" of this class in class tree */
	double	allotment_;	/* this class' fraction of link bandwidth */
	double	maxidle_;	/* upper bound for avgidle */
	double	minidle_;	/* lower bound for avgidle, no longer used */
	double	extradelay_;	/* 'offtime'-time overlim class waits to snd */
	double	maxrate_;	/* bytes per second allowed this class */
	int	delayed_;	/* 1 for an already-delayed class */
	int	plot_;		/* to plot for debugging */
    protected:
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
 * the CBQ link sharing machinery
 */

class CBQueue : public Queue {
    public:	
	CBQueue();
	virtual int command(int argc, const char*const* argv);
	Packet	*deque();
	int	enque(Packet *pkt);
	int	qlen();			// sum of all q's managed by this CBQ
inline double bandwidth() { return (bandwidth_); }	// from link.h
	void	addallot(int, double);	// change allotment at some pri level
    protected:
	virtual void insert_class(CBQClass*);	// WRR and 
	void	delay_action(CBQClass*);
	int	ancestor(CBQClass *cl, CBQClass *p);
	int	satisfied(CBQClass *cl);
	int	ok_to_borrow(CBQClass *cl);	// formal LS
	int 	ok_to_borrow3(CBQClass *cl);	// old formal LS
	int	under_limit(CBQClass *cl, double now);
	void	plot(CBQClass *cl, double idle);
	void	update_class_util(CBQClass *cl, double now, int pktsize);
	void check_for_cycles(LinkageMethod);

int	special(Flow *, CBQClass *);
void	unspecial(Flow *);
void	unspecial_all();
inline Class* determine_class(Packet *pkt) {
// used in incoming path to classify a packet
Class *cl = classtab_[pkt->class_];
PenaltyBoxEntry *pb;
if (npbox_ > 0) {
if ((pb = findpbox(pkt)) != NULL)
cl = pb->class_;
}
return (cl);
}
inline int pmatch(Packet* pkt, Flow *fp) {
	return (pkt->src_ == fp->src() &&
		pkt->dst_ == fp->dst() &&
		pkt->class_ == fp->fclass());
};

	CBQClass* active_[MAXPRIO];	/* active class at each level */
	CBQClass* borrowed_;		/* the class last borrowed from */
	CBQClass* classtab_[MAXCLASS];	/* class of newly-arrived pkts */
	CBQClass* list_;		/* beginning of list of classes */
	int	maxpkt_;	/* max packet size in bytes */
	int	maxdepth_;	/* max depth at which borrowing is allowed */
	int	algorithm_;	/* 1 to restrict borrowing, 0 otherwise */
				/* 2 for idealized link-sharing guidelines */
	int	num_[MAXPRIO];	/* number of classes at each priority level */
	double	alloc_[MAXPRIO];	/* total allocation at each level, 
					 *for WRR */
PenaltyBoxEntry* findpbox(Flow *);	/* special handling entry */
PenaltyBoxEntry* findpbox(Packet *);	/* special handling entry */
PenaltyBoxEntry	penalty_box_[MAXPBOX];	/* penalty box */
u_int	npbox_;				/* # entries in pbox */
};

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


/*********************** Class member functions *************************/

CBQClass::CBQClass() : peer_(0), bytes_alloc_(0), qdisc_(0), next_(0), mark_(0),
	last_(0.0), undertime_(0.0), avgidle_(0.0), delayed_(0),
	parent_(0), borrow_(0), cbq_(0)

{
	bind("priority_", &pri_);
	bind("depth_", &depth_);
	bind("plot_", &plot_);
	bind("allotment_", &allotment_);
	bind("maxidle_", &maxidle_);
	bind("minidle_", &minidle_);
	bind("extradelay_", &extradelay_);

	/*XXX*/
	if (pri_ < 0 || pri_ > MAXPRIO)
		abort();
}

void
CBQClass::newallot(double bw)
{
	maxrate_ = bw * ( cbq_->bandwidth() / 8.0 );
	double diff = allotment_ - bw;
	allotment_ = bw;
	cbq_->addallot(pri_, diff);
	return;
}

/* 
 * $class1 parent $class2
 * $class1 borrow $class2
 * $class1 qdisc $class2
 * $class1 allot
 * $class1 allot new-bw
 */
int CBQClass::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "allot") == 0) {
			tcl.resultf("%g", allotment_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "cbq") == 0) {
			tcl.resultf("%s", cbq()->name());
			return(TCL_OK);
		}
		if (strcmp(argv[1], "qdisc") == 0) {
			tcl.resultf("%s", qdisc()->name());
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
			qdisc_ = (Link*) TclObject::lookup(argv[2]);
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
	return (NsObject::command(argc, argv));
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

CBQueue::CBQueue() : borrowed_(0), list_(0), maxdepth_(MAXDEPTH),
	npbox_(0)
{
	bind("algorithm_", &algorithm_);
	bind("max-pktsize_", &maxpkt_);

	int i;
	for (i = 0; i < MAXPRIO; ++i) {
		active_[i] = 0;
		num_[i] = 0;
		alloc_[i] = 0.0;
	}
	for (i = 0; i < MAXCLASS; ++i)
		classtab_[i] = 0;

for (i = 0; i < MAXPBOX; ++i) {
	penalty_box_[i].flow_ = 0;
	penalty_box_[i].class_ = 0;
}

}

/*
 * This implements the following tcl commands:
 *  $cbq insert $class
 *  $cbq bind $class $classID
 *  $cbq bind $class $first $last
 *  $cbq special $flow
 *  $cbq unspecial $flow
 *  $cbq clookup $flow
 *  $cbq sclass $flow
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
		if (strcmp(argv[1], "unspecial") == 0) {
			if (strcmp(argv[2], "all") == 0) {
				unspecial_all();
				return (TCL_OK);
			}
			Flow *fp = (Flow*)TclObject::lookup(argv[2]);
			unspecial(fp);
			return (TCL_OK);
		}
		// get specialty class for this flow
		if (strcmp(argv[1], "sclass") == 0) {
			char *bp = tcl.buffer();
			Flow *fp = (Flow*)TclObject::lookup(argv[2]);
			PenaltyBoxEntry* pb = findpbox(fp);
			sprintf(bp, "%s",
			    (pb != 0) ? pb->class_->TclObject::name() : "0");
			return(TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "special") == 0) {
			Flow *fp = (Flow*)TclObject::lookup(argv[2]);
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[3]);
			return(special(fp, p));
		}
		if (strcmp(argv[1], "bind") == 0) {
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[2]);
			int id = atoi(argv[3]);
			/*XXX check id range */
			classtab_[id] = p;
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "bind") == 0) {
			CBQClass* p = (CBQClass*)TclObject::lookup(argv[2]);
			int firstid = atoi(argv[3]);
			int lastid = atoi(argv[4]);
			/*XXX check id range */
			register i;
			for (i=firstid; i <= lastid; i++) {
			   classtab_[i] = p;
			}
			return (TCL_OK);
		}
	}
	return (Link::command(argc, argv));
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
	p->maxrate_ = p->allotment_ * ( bandwidth() / 8 );

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
 * This is used only for algorithm_ ==2 or 3.
 */
int CBQueue::ancestor(CBQClass* cl, CBQClass* p)
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
int CBQueue::satisfied(CBQClass* cl)
{
	CBQClass	*p;
	double	now = Scheduler::instance().clock();

	if (cl->undertime_ > now)
		return (1);
	else if (cl->depth_ == 0) {
		if (cl->qdisc()->qlen() > 0)
			return (0);
		else
			return (1);
	} else {
		for (p = list_; p != 0; p = p->next_) {
			if (p->depth_ == 0 && ancestor(cl, p)) {
				if (p->qdisc()->qlen() > 0)
					return (0);
			}
		}
	}
	return (1);
}

/*
 * consider the "cbq q len" to be the sum of the queues managed
 * by all the classes
 */

int CBQueue::qlen()
{
	int sum = 0;
	CBQClass *p;

	for (p = list_; p != 0; p = p->next_)
		sum += p->qdisc()->qlen();

	return (sum);
}

/* Ok_to_borrow returns 1 if it is ok for a descendant to borrow
 * from this underlimit interior class cl.
 * It is ok to borrow if there are no unsatisfied classes at a lower level.
 * This is for the new version of Formal link-sharing.
 * Put another way: if there are any unsatisfied classes below this class
 * then it's not ok to borrow.
 */
int CBQueue::ok_to_borrow(CBQClass* cl)
{
	for (CBQClass* p = list_; p != 0; p = p->next_)
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
int CBQueue::ok_to_borrow3(CBQClass* cl)
{
	for (CBQClass* p = list_; p != 0; p = p->next_)
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
	double t = Scheduler::instance().clock();
   	sprintf(trace_->buffer(), "%g avg %g\n", t, cl->avgidle_);
	trace_->dump();
	sprintf(trace_->buffer(), "%g idl %g\n", t, idle);
	trace_->dump();
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
	queue_length = cl->qdisc()->qlen(); 

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
#ifdef notyet
		if (plot_) 
			plot(cl, cl->idle);
#endif
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
Packet* CBQueue::deque(void)
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
			if (cl->qdisc()->qlen() > 0) {
				if (first == 0 && cl->borrow() != 0)
					first = cl;
				if (!under_limit(cl, now)) {
					/* not underlimit and can't borrow  */
					delay_action(cl);
				} else {
					/* under-limit or can borrow */
					/* so send and update stats */
					Packet* pkt = cl->qdisc()->deque();
					update_class_util(cl, now, pkt->size_);
					active_[i] = cl->peer_;
					// could let qdisc do tracing, but
					// instead do it here
					log_packet_departure(pkt);
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
		Packet* pkt = first->qdisc()->deque();
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
int CBQueue::enque(Packet* pkt)
{
	double	now = Scheduler::instance().clock();
	CBQClass *cl = determine_class(pkt);
	Flow *f = 0;
	int rval = 0;
	log_packet_arrival(pkt);
        if (flowmgr_) {
                if ((f = flowmgr_->findflow(pkt)) == NULL) 
                        f = flowmgr_->newflow(pkt);
                if (f)
                        f->arrival(pkt);
        }

	//
	// note: the following will DROP packets which don't
	// belong to a known CBQ class.  this should probably be changed
	// when simulations with unknown traffic are used
	//

	if (cl != 0) {
		if (algorithm_ == 1 && maxdepth_ > 0) {
			if (cl->undertime_ < now) 
				maxdepth_ = 0;
			else if (maxdepth_ > 1 && cl->borrow() != 0) {
				if (cl->borrow()->undertime_ < now)
					maxdepth_ = 1;
			}
		}
		if (cl->qdisc() == NULL) {
			fprintf(stderr, "CBQ: enque: no qdisc for class %d\n",
				pkt->class_);
			abort();
		}
		// qdisc may drop packet:
		//	if so, log this.... (this call is a little
		//	dangerous in that the qdisc's enque() generalls
		//	frees the packet, but we need its hdr data
		//	[fortunately, its only on the free list...]

		if ((rval = cl->qdisc()->enque(pkt)) < 0)
			log_packet_drop(pkt);
		return (rval);
	}

	/*
	 * log drops due if no cbq class for this pkt
	 *
	 *	XXX should have a default best-effort class,
	 */
fprintf(stderr, "warning: CBQ received pkt with unknown class %d\n", pkt->class_);
	log_packet_drop(pkt);
	Packet::free(pkt);
	return (-1);
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

/*
 * deal with a specialized flow:
 *	need to determine to which class a penalized flow goes..
 */
int
CBQueue::special(Flow *fp, CBQClass *cp)
{
	if (fp && cp && npbox_ < MAXPBOX) {
		npbox_++;
		penalty_box_[npbox_-1].flow_ = fp;
		penalty_box_[npbox_-1].class_ = cp;
		return (TCL_OK);
	}
	return (TCL_ERROR);
}

/*
 * find a penalty box entry given a flow or a packet
 */

PenaltyBoxEntry*
CBQueue::findpbox(Flow *fp)
{
	register i;
	for (i = 0; i < npbox_; i++)
		if (penalty_box_[i].flow_ == fp)
			return (&penalty_box_[i]);
	return (NULL);
}

PenaltyBoxEntry*
CBQueue::findpbox(Packet *p)
{
	register i;
	for (i = 0; i < npbox_; i++)
		if (pmatch(p, penalty_box_[i].flow_))
			return (&penalty_box_[i]);
	return (NULL);
}

void CBQueue::unspecial_all()
{
	npbox_ = 0;
}

void CBQueue::unspecial(Flow *fp)
{
	PenaltyBoxEntry*	pe = findpbox(fp);
	int index;

	if (pe == NULL)
		return;
	index = pe - penalty_box_;
	if (index != npbox_ - 1)
		penalty_box_[index] = penalty_box_[npbox_-1];
	npbox_--;
	return;
}

WRR_CBQueue::WRR_CBQueue()
{
	for (int i = 0; i < MAXPRIO; ++i)
		M_[i] = 0.;
}

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
		if (alloc_[i] > 0)
			/*
			M_[i] = npri(i) * maxpkt_ * 1.0/alloc_[i];
			*/
			M_[i] = num_[i] * maxpkt_ * 1.0/alloc_[i];
		else
			M_[i] = 0.0;
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
Packet* WRR_CBQueue::deque(void)
{
	double now = Scheduler::instance().clock();
	CBQClass* first = 0;
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
		CBQClass* cl = active_[i];
		if (cl == 0)
			continue;
		int deficit = 0;
		int done = 0;
		while (!done) {
			do {
				if (deficit < 2 && cl->bytes_alloc_ <= 0) 
					cl->bytes_alloc_ += (int)(cl->allotment_ * M_[cl->pri_]);
				if (cl->qdisc()->qlen() > 0) {
					if (first == 0 && cl->borrow() != 0)
						first = cl;
					if (!under_limit(cl, now))
					 	delay_action(cl);
					else {
						if (cl->bytes_alloc_ > 0 || deficit > 1) {
							Packet* pkt = cl->qdisc()->deque();
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
	maxdepth_ = MAXDEPTH;
	if (first != 0) {
		Packet* pkt = first->qdisc()->deque();
		if (pkt) {
			active_[first->pri_] = first->peer_;
			return (pkt);
		}
	}
	return (0);
}
