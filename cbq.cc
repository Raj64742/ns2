/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/cbq.cc,v 1.7 1997/04/08 02:39:19 kfall Exp $ (LBL)";
#endif

//
// attempt at new version of cbq using the ns-2 fine-grain
// objects.  Also, re-orginaize CBQ to look more like how
// its description reads in ToN v3n4 and simplify extraneous stuff -KF
//
// there is a 1-1 relationship between classes and queues
//
// Definitions:
//	overlimit:
//		recently used more than allocated link-sharing bandwidth
//			(in bytes/sec averaged over specified interval)
//	
//	level:
//		all leaves are at level 1
//		interior nodes are at a level 1 greater than
//		    the highest level number of any of its children
//
//	unsatisfied:
//		(leaf): underlimit and has demand
//		(interior): underlimit and has some descendant w/demand
//			[either a leaf or interior descendant]
//
//	formal link-sharing:
//		class may continue unregulated if either:
//		1> class is underlimit or at-limit
//		2> class has a under(at)-limit ancestor at level i
//			and no unsatisfied classes at any levels < i
//
//	ancestors-only link-sharing:
//		class may continue unregulated if either:
//		1> class is under/at limit
//		2> class has an UNDER-limit ancestor [at-limit not ok]
//
//	top-level link-sharing:
//		class may continue unregulated if either:
//		1> class is under/at limit
//		2> class has an UNDER-limit ancestor with level
//			<= the value of "top-level"

#include "queue-monitor.h"
#include "queue.h"
#include "delay.h"

#define	MAXPRIO		8	/* # priorities in scheduler */
#define	MAXLEVEL	32	/* max depth of link-share tree(s) */
#define	LEAF_LEVEL	1	/* level# for leaves */
#define	POWEROFTWO	16

class CBQClass : public Connector {
public:
	friend class CBQueue;
	friend class WRR_CBQueue;

	CBQClass();
	int	command(int argc, const char*const* argv);
	void	recv(Packet*, Handler*);	// from upstream classifier

protected:

	void	update(Packet*, double);	// update when sending pkt
	void	delayed(double);		// when overlim/can't borrow
	void	newallot(double);		// change an allotment

	int	satisfied(double);		// satisfied?
	int 	demand();			// do I have demand?
	int 	leaf();				// am I a leaf class?

	int	ancestor(CBQClass*p);		// are we an ancestor of p?
	int	desc_with_demand();		// any desc has demand?

	CBQueue*	cbq_;			// the CBQueue I'm part of

	CBQClass*	peer_;			// peer at same sched prio level
	CBQClass*	level_peer_;		// peer at same LS level
	CBQClass*	lender_;		// parent I can borrow from

	Queue*		q_;			// underlying queue
	QueueMonitor*	qmon_;			// monitor for the queue

	double		allotment_;		// frac of link bw
	double		maxidle_;		// bound on idle time
	double		maxrate_;		// bound on bytes/sec rate
	double		extradelay_;		// adjustment to delay
	double		last_time_;		// last xmit time this class
	double		undertime_;		// will become unsat/eligible
	double		avgidle_;		// EWMA of idle
	int		pri_;			// priority for scheduler
	int		level_;			// depth in link-sharing tree
	int		delayed_;		// boolean-was I delayed
	int		bytes_alloc_;		// for wrr only

};

class CBQueue : public Queue {
public:
	CBQueue();
	virtual int	command(int argc, const char*const* argv);
	void		enque(Packet*) { abort(); /* shouldn't happen */ }
	void		recv(Packet*, Handler*);
	LinkDelay*	link() const { return (link_); }
	CBQClass*	level(int n) const { return levels_[n]; }
	Packet*		deque();
	virtual void	addallot(int prio, double diff) { }

protected:
	virtual int	insert_class(CBQClass*);
	int		send_permitted(CBQClass*, double);
	CBQClass*	find_lender(CBQClass*, double);

	Packet*		pending_pkt_;
	LinkDelay*	link_;			// managed link

	CBQClass*	active_[MAXPRIO];	// classes at prio of index
	CBQClass*	levels_[MAXLEVEL+1];	// LL of classes per level
	int		maxprio_;		// highest prio# seen
	int		maxlevel_;		// highest level# seen
	int		toplevel_;		// for top-level LS

	int (CBQueue::*eligible_)(CBQClass*, int, double); // eligible function
	int	eligible_formal(CBQClass*, int, double);
	int	eligible_ancestors(CBQClass*, int, double) { return (1); }
	int	eligible_toplevel(CBQClass* cl, int, double) {
		return(cl->level_ <= toplevel_);
	}
};

CBQueue::CBQueue() : pending_pkt_(NULL), link_(NULL),
	maxprio_(-1), maxlevel_(-1), toplevel_(-1), eligible_(NULL)
{
	bind("toplevel_", &toplevel_);
	memset(active_, '\0', sizeof(active_));
	memset(levels_, '\0', sizeof(levels_));
}

/*
 * invoked by passing a packet from one of our managed queues
 */

void
CBQueue::recv(Packet* p, Handler*)
{
if (pending_pkt_ != NULL)
abort();
	pending_pkt_ = p;
	if (!blocked_) {
		blocked_ = 1;
		resume();
	}
	return;
}

/*
 * deque: this gets invoked by way of our downstream neighbor
 * doing a 'resume' on us via our handler (by Queue::resume()).
 * (or by our own recv when we weren't busy sending anything else).
 *
 * produce the packet that last arrived and cause another class to
 * sending something
 */

Packet *
CBQueue::deque()
{
	double now = Scheduler::instance().clock();

	CBQClass* first = NULL;
	CBQClass* cl;
	register int prio;

	Packet* rval = pending_pkt_;
	pending_pkt_ = NULL;

	/*
	 * prio runs from 0 .. maxprio_
	 *
	 * round-robin through all the classes at priority 'prio'
	 * 	if any class is ok to send, resume it's queue
	 * go on to next lowest priority (higher prio nuber) and repeat
	 * [lowest priority number is the highest priority]
	 */
	for (prio = 0; prio <= maxprio_; prio++) {
		// see if there is any class at this prio
		if ((cl = active_[prio]) == NULL) {
			// nobody at this prio level
			continue;
		}

		do {
			// anything to send?
			if (cl->demand()) {
				if (first == NULL && cl->lender_ != NULL)
					first = cl;
				if (send_permitted(cl, now)) {
					// ok to send
					cl->delayed_ = 0;
					active_[prio] = cl->peer_;
// question if this happens RIGHT AWAY?
					cl->q_->resume();
					return (rval);
				} else {
					// not ok right now
					cl->delayed(now);
				}
			}
			cl = cl->peer_;	// move to next at same prio
		} while (cl != active_[prio]);
	}
	if (first != NULL) {
		active_[first->pri_] = first->peer_;
		first->q_->resume();
	}
	return (rval);
}

/*
 * we are permitted to send if either
 *	1> we are not overlimit (ie we are underlimit or at limit)
 *	2> one of the varios algorithm-dependent conditions is met
 *
 * if we are permitted, who did we borrow from? [could be ourselves
 * if we were not overlimit]
 */
int CBQueue::send_permitted(CBQClass* cl, double now)
{
	if (cl->undertime_ < now) {
		return (1);
	} else if ((cl = find_lender(cl, now)) != NULL) {
		return (1);
	}
	return (0);
}

/*
 * find_lender(class, time)
 *
 *	find a lender for the provided class according to the
 *	various algorithms
 *
 */

CBQClass*
CBQueue::find_lender(CBQClass* cl, double now)
{
	int last_level = LEAF_LEVEL;

	if ((cl = cl->lender_) == NULL)
		return (NULL);		// no ancestor to borrow from

	while (cl != NULL) {
		// skip past overlimit ancestors
		if (cl->undertime_ > now) {
			cl = cl->lender_;
			continue;
		}

		if (eligible_(cl, last_level, now))
			return (cl);
		last_level = cl->level_;
		cl = cl->lender_;
	}
	return (cl);
}

/*
 * rule #2 for formal link-sharing
 *	class must have no unsatisfied classes below it
 *	optimization here: no unsatisfied classes below this class
 *		and on or above level 'baselevel'
 */
int
CBQueue::eligible_formal(CBQClass *cl, int baselevel, double now)
{
	int level;
	CBQClass *p;

	// check from level base to (cl->level - 1)
	for (level = baselevel; level < cl->level_; level++) {
		p = levels_[level];
		while (p != NULL) {
			if (!p->satisfied(now))
				return (0);
			p = p->level_peer_;
		}
	}
	return (1);
}

/*
 * insert a class into the cbq object
 */

int
CBQueue::insert_class(CBQClass *p)
{
        p->cbq_ = this;

        /*
         * Add to circularly-linked list "active_"
	 *    of peers for the given priority.
         */

	if (p->pri_ < 0 || p->pri_ > (MAXPRIO-1))
		return (-1);

        if (active_[p->pri_] != NULL) {
                p->peer_ = active_[p->pri_]->peer_;
                active_[p->pri_]->peer_ = p;
        } else {
                p->peer_ = p;
                active_[p->pri_] = p;
        }
	if (p->pri_ > maxprio_)
		maxprio_ = p->pri_;

        /*
         * Compute maxrate from allotment.
	 * convert to bytes/sec
	 *	and store the highest prio# we've seen
         */

	if (p->allotment_ < 0.0 || link_->bandwidth() <= 0.0)
		return (-1);
        p->maxrate_ = p->allotment_ * (link_->bandwidth() / 8.0);

	/*
	 * Add to per-level list
	 *     and store the highest level# we've seen
	 */

	if (p->level_ <= 0 || p->level_ > MAXLEVEL)
		return (-1);

	p->level_peer_ = levels_[p->level_];
	levels_[p->level_] = p;
	if (p->level_ > maxlevel_)
		maxlevel_ = p->level_;

        /*
         * Check that parent and borrow linkage are acyclic.
         */
#ifdef notdef
        check_for_cycles(CBQClass::parent);
        check_for_cycles(CBQClass::borrow);
#endif
	return 0;
}

int CBQueue::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "insert") == 0) {
			CBQClass *cl = (CBQClass*)TclObject::lookup(argv[2]);
			if (insert_class(cl) < 0)
				return (TCL_ERROR);

			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

class WRR_CBQueue : public CBQueue {
public:
	WRR_CBQueue() {
		memset(M_, '\0', sizeof(M_));
		memset(alloc_, '\0', sizeof(alloc_));
		memset(cnt_, '\0', sizeof(cnt_));
	}
	void	addallot(int prio, double diff) {
		alloc_[prio] += diff;
	}
protected:
	Packet *deque();
	virtual int	insert_class(CBQClass*);
	void	setM();
	double	alloc_[MAXPRIO];
	double	M_[MAXPRIO];
	int	cnt_[MAXPRIO];		// # classes at prio of index
	int	maxpkt_;		// max packet size
};

Packet *
WRR_CBQueue::deque()
{
// this needs work
	double now = Scheduler::instance().clock();

	CBQClass* first = NULL;
	CBQClass* cl;
	register int prio;
	int deficit, done;

	Packet* rval = pending_pkt_;
	pending_pkt_ = NULL;

	/*
	 * prio runs from 0 .. maxprio_
	 */
	for (prio = 0; prio <= maxprio_; prio++) {
		// see if there is any class at this prio
		if ((cl = active_[prio]) == NULL)
			continue;
		deficit = done = 0;
		while (!done) {
			do {
				if (deficit < 2 && cl->bytes_alloc_ <= 0)
					cl->bytes_alloc_ +=
					  (int)(cl->allotment_ * M_[cl->pri_]);
				if (cl->demand()) {
					if (first == NULL && cl->lender_ != NULL)
						first = cl;
					if (send_permitted(cl, now)) {
						cl->delayed_ = 0;
						active_[prio] = cl->peer_;
						cl->q_->resume();
						return (rval);
					} else {
						cl->delayed(now);
					}
				}
				cl = cl->peer_;
			} while (cl != active_[prio] && cl != 0);
			if (deficit == 1)
				deficit = 2;
			else
				done = 1;
		}
	}
	if (first != NULL) {
		active_[first->pri_] = first->peer_;
		first->q_->resume();
	}
	return (rval);
}

int
WRR_CBQueue::insert_class(CBQClass *p)
{
	if (CBQueue::insert_class(p) < 0)
		return (-1);
	alloc_[p->pri_] += p->allotment_;
	++cnt_[p->pri_];
	setM();
	return (0);
}

void
WRR_CBQueue::setM()
{
	int i;
	for (i = 0; i < maxprio_; i++) {
		if (alloc_[i] > 0.0)
			M_[i] = cnt_[i] * maxpkt_ * 1.0 / alloc_[i];
		else
			M_[i] = 0.0;
	}
	return;
}

/******************** CBQClass definitions **********************/

CBQClass::CBQClass() : cbq_(0), peer_(0), level_peer_(0), lender_(0),
	q_(0), qmon_(0), allotment_(0.0), maxidle_(0.0), maxrate_(0.0),
	extradelay_(0.0), last_time_(0.0), undertime_(0.0), avgidle_(0.0),
	pri_(-1), level_(-1), delayed_(0), bytes_alloc_(-1)
{
	bind("maxidle_", &maxidle_);
	bind("priority_", &pri_);
	bind("level_", &level_);
	bind("extradelay_", &extradelay_);

	if (pri_ < 0 || pri_ > (MAXPRIO-1))
		abort();

        if (level_ <= 0 || level_ > MAXLEVEL)
		abort();
}

// why can't these two be inline (?)
int
CBQClass::demand()
{
	return (qmon_->pkts() > 0);
}

int
CBQClass::leaf()
{
	return (level_ == LEAF_LEVEL);
}

/*
 * the queue directly upstream from us is the one we're
 * in charge of.  If it gets to send something it must
 * be because it was unblocked by the CBQueue downstream.
 * Thus, its time to perform accounting...
 */
void
CBQClass::recv(Packet *pkt, Handler *h)
{
	double now = Scheduler::instance().clock();
	update(pkt, now);
	send(pkt, h);		// Connector::send()
	return;
}

/*
 * update a class' statistics and all parent classes
 * up to the root
 */
void CBQClass::update(Packet* p, double now)
{
	double idle, avgidle;

	hdr_cmn* hdr = (hdr_cmn*)p->access(off_cmn_);
	int pktsize = hdr->size();

	double tx_time = cbq_->link()->txtime(p);
	double fin_time = now + tx_time; // should this include delay?

	idle = (fin_time - last_time_) - (pktsize / maxrate_);
	avgidle = avgidle_;
	avgidle += (idle - avgidle) / POWEROFTWO;
	if (avgidle > maxidle_)
		avgidle = maxidle_;
	avgidle_ = avgidle;
	if (avgidle <= 0) {
		undertime_ = fin_time + tx_time *
			(1.0 / allotment_ - 1.0);
		undertime_ += (1-POWEROFTWO) * avgidle;
	}
	last_time_ = fin_time;
	// tail-recurse up to root of tree performing updates
	if (lender_)
		lender_->update(p, now);

	return;
}

/*
 * satisfied: is this class satisfied?
 */

int
CBQClass::satisfied(double now)
{
	if (leaf()) {
		/* leaf is unsat if underlimit with backlog */
		if (undertime_ < now && demand())
			return (0);
		else
			return (1);
	}
	if (undertime_ < now && desc_with_demand())
		return (0);

	return (1);
}

/*
 * desc_with_demand: is there a descendant of this class with demand
 *	really, is there a leaf which is a descendant of me with
 *	a backlog
 */

int
CBQClass::desc_with_demand()
{
	CBQClass *p = cbq_->level(LEAF_LEVEL);
	for (; p != NULL; p = p->level_peer_) {
		if (p->demand() && ancestor(p))
			return (1);
	}
	return (0);
}

/*
 * happens when a class is unable to send because it
 * is being regulated
 */

void CBQClass::delayed(double now)
{
	double delay = undertime_ - now + extradelay_;

	if (delay > 0 && !delayed_) {
		undertime_ += extradelay_;
		undertime_ -= (1-POWEROFTWO) * avgidle_;
		delayed_ = 1;
	}
}

/*
 * return 1 if we are an ancestor of p, 0 otherwise
 */
int
CBQClass::ancestor(CBQClass *p)
{
	if (p->lender_ == NULL)
		return (0);
	else if (p->lender_ == this)
		return (1);
	return (ancestor(p->lender_));
}

/*
 * change an allotment
 */
void
CBQClass::newallot(double bw)
{
        maxrate_ = bw * ( cbq_->link()->bandwidth() / 8.0 );
        double diff = allotment_ - bw;
        allotment_ = bw;
        cbq_->addallot(pri_, diff);
        return;
}


/*
 * OTcl Interface
 */
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
                        tcl.resultf("%s", cbq_->name());
                        return(TCL_OK);
                }
                if (strcmp(argv[1], "qdisc") == 0) {
                        tcl.resultf("%s", q_->name());
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "qmon") == 0) {
                        tcl.resultf("%s", qmon_->name());
                        return (TCL_OK);
		}
	} else if (argc == 3) {
		// for now these are the same
                if ((strcmp(argv[1], "parent") == 0) ||
		    (strcmp(argv[1], "borrow") == 0)) {
                        lender_ = (CBQClass*)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
                if (strcmp(argv[1], "qdisc") == 0) {
                        q_ = (Queue*) TclObject::lookup(argv[2]);
                        if (q_ != NULL)
                                return (TCL_OK);
                        return (TCL_ERROR);
                }
                if (strcmp(argv[1], "qmon") == 0) {
                        qmon_ = (QueueMonitor*) TclObject::lookup(argv[2]);
                        if (qmon_ != NULL)
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
