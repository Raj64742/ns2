/*
 * Copyright (c) 1991,1993 Regents of the University of California.
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
 *      This product includes software developed by the Computer Systems
 *      Engineering Group at Lawrence Berkeley Laboratory.
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
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/netmodel.cc,v 1.1 1997/03/29 04:38:06 mccanne Exp $ (LBL)";
#endif

#include <osfcn.h>
#include <ctype.h>
#include "math.h"

#include "netview.h"
#include "animation.h"
#include "nam-queue.h"
#include "nam-drop.h"
#include "nam-packet.h"
#include "nam-edge.h"
#include "nam-node.h"
#include "nam-trace.h"
#include "netmodel.h"
#include "paint.h"
#include "sincos.h"
#include "state.h"

#include <float.h>

extern int lineno;
static int next_pat;

/*XXX */
double defsize = 0.;

class NetworkModelClass : public TclClass {
 public:
	NetworkModelClass() : TclClass("NetworkModel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NetModel);
	}
} networkmodel_class;

NetModel::NetModel()
	: drawables_(0), animations_(0), queues_(0),
	  views_(0), nodes_(0)
{
	/*XXX*/
	int cacheSize = 100;
	State::init(cacheSize);

	int i;
	for (i = 0; i < EDGE_HASH_SIZE; ++i)
		hashtab_[i] = 0;

	/*XXX*/
	nymin_ = 1e6;
	nymax_ = -1e6;

	Paint *paint = Paint::instance();
	int p = paint->thin();
	nclass_ = 64;
	paint_ = new int[nclass_];
	for (i = 0; i < nclass_; ++i)
		paint_[i] = p;
}

NetModel::~NetModel()
{
}

void NetModel::update(double now)
{
	Animation *a, *n;
	for (a = animations_; a != 0; a = n) {
		n = a->next();
		a->update(now);
	}

	/*
	 * Draw all animations and drawables on display to reflect
	 * current time.
	 */
	now_ = now;
	for (NetView* p = views_; p != 0; p = p->next_)
		p->draw();
}

void NetModel::update(double now, Animation* a)
{
	for (NetView* p = views_; p != 0; p = p->next_)
		a->draw(p, now);
}

/* Reset all animations and queues to time 'now'. */
void NetModel::reset(double now)
{
	Animation* a;
	for (a = animations_; a != 0; a = a->next())
		a->reset(now);
	for (a = drawables_; a != 0; a = a->next())
		a->reset(now);
	for (NamQueue* q = queues_; q != 0; q = q->next_)
		q->reset(now);
}

/*
 * Draw this NetModel's animations and drawables (nodes, edges,
 * packets, queues).
 */
void NetModel::render(NetView* view)
{
	Animation *a;
	for (a = animations_; a != 0; a = a->next())
		a->draw(view, now_);
	for (a = drawables_; a != 0; a = a->next())
		a->draw(view, now_);
}

/* Return a pointer to the edge between 'src' and 'dst'. */
NetModel::EdgeHashNode* NetModel::lookupEdge(int src, int dst) const
{
	EdgeHashNode* h;
	for (h = hashtab_[ehash(src, dst)]; h != 0; h = h->next)
		if (h->src == src && h->dst == dst)
			break;
	return (h);
}

void NetModel::enterEdge(NamEdge* e)
{
	int src = e->src();
	int dst = e->dst();
	EdgeHashNode *h = lookupEdge(src, dst);
	if (h != 0) {
		/* XXX */
		fprintf(stderr, "nam: duplicate edge (%d,%d)\n");
		exit(1);
	}
	h = new EdgeHashNode;
	h->src = src;
	h->dst = dst;
	h->queue = 0;
	h->edge = e;
	int k = ehash(src, dst);
	h->next = hashtab_[k];
	hashtab_[k] = h;
}

/* XXX Make this cheaper (i.e. cache it) */
void NetModel::BoundingBox(BBox& bb)
{
	/* ANSI C limits, from float.h */
	bb.xmin = bb.ymin = FLT_MAX;
	bb.xmax = bb.ymax = -FLT_MAX;
	
	for (Animation* a = drawables_; a != 0; a = a->next())
		a->merge(bb);
}


Animation* NetModel::inside(double now, float px, float py) const
{
	for (Animation* a = animations_; a != 0; a = a->next())
		if (a->inside(now, px, py))
			return (a);
	for (Animation* d = drawables_; d != 0; d = d->next())
		if (d->inside(now, px, py))
			return (d);

	return (0);
}

void NetModel::saveState(double tim)
{
	State* state = State::instance();
	StateInfo min;
	min.time = 10e6;
	min.offset = 0;
	Animation *a, *n;
	StateInfo si;

	/*
	 * Update the animation list first to remove any unnecessary
	 * objects in the list.
	 */
	for (a = animations_; a != 0; a = n) {
		n = a->next();
		a->update(tim);
	}
	for (a = animations_; a != 0; a = n) {
		n = a->next();
		si = a->stateInfo();
		if (si.time < min.time)
			min = si;
	}
	if (min.offset)
		state->set(tim, min);
}

/* Trace event handler. */
void NetModel::handle(const TraceEvent& e, double now)
{
	QueueItem *q;
	EdgeHashNode *ehn;
	NamEdge* ep;
	double txtime;
	if (e.time > State::instance()->next())
		saveState(e.time);
	
	switch (e.tt) {

	case 'v':	/* 'variable' -- just execute it as a tcl command */
		Tcl::instance().eval((char *)(e.image + e.ve.str));
		break;
		
	case 'h':
		ehn = lookupEdge(e.pe.src, e.pe.dst);
		if (ehn == 0 || (ep = ehn->edge) == 0)
			return;

		/*
		 * If the current packet will arrive at its destination
		 * at a later time, insert the arrival into the queue
		 * of animations and set its paint id (id for X graphics
		 * context.
		 */
		txtime = ep->txtime(e.pe.pkt.size);
		if (e.time + txtime + ep->GetDelay() >= now) {
			/* XXX */
			NamPacket *p = new NamPacket(ep, e.pe.pkt, e.time,
						     txtime, e.offset);
			p->insert(&animations_);
			p->paint(paint_[e.pe.pkt.attr & 0xff]);
		}
		break;
		
	case '+':
		ehn = lookupEdge(e.pe.src, e.pe.dst);
		if (ehn != 0 && ehn->queue != 0) {
			QueueItem *qi = new QueueItem(e.pe.pkt, e.time,
						      e.offset);
			qi->paint(paint_[e.pe.pkt.attr & 0xff]);
			ehn->queue->enque(qi);
			qi->insert(&animations_);
		}
		break;
		
	case '-':
		ehn = lookupEdge(e.pe.src, e.pe.dst);
		if (ehn != 0 && ehn->queue != 0) {
			q = ehn->queue->remove(e.pe.pkt);
			delete q;
		}
		break;
		
	case 'd':
		/* Get packet (queue item) to drop. */
		ehn = lookupEdge(e.pe.src, e.pe.dst);
		if (ehn == 0 || ehn->queue == 0)
			return;
		q = ehn->queue->remove(e.pe.pkt);
		if (q == 0) {
			fprintf(stderr,
				"trace file drops packet %d which is not in queue\n",
				e.pe.pkt.id);
			return;
		}
		float x, y;
		q->position(x, y);
		int pno = q->paint();
		delete q;
		/*
		 * Compute the point at which the dropped packet disappears.
		 * Let's just make this sufficiently far below the lowest
		 * thing on the screen.
		 *
		 * Watch out for topologies that have all their nodes lined
		 * up horizontally. In this case, nymin_ == nymax_ == 0.
		 * Set the bottom to -0.028. This was chosen empirically.
		 * The nam display was set to the maximum size and the lowest
		 * position on the display was around -0.028.
		 */
		float bot;
		if (nymin_ == nymax_ && nymin_ == 0.)
			bot = -0.03;
		else
			bot = nymin_ - (nymax_ - nymin_);
		
		NamDrop *d = new NamDrop(x, y, bot, ehn->edge->PacketHeight(),
					 e.time, e.offset, e.pe.pkt);
		d->paint(pno);
		d->insert(&animations_);
		break;
	}

}

void NetModel::addView(NetView* p)
{
	p->next_ = views_;
	views_ = p;
}

NamNode *NetModel::lookupNode(int nn) const
{
	for (NamNode* n = nodes_; n != 0; n = n->next_)
		if (n->num() == nn)
			return (n);
	/* XXX */
	fprintf(stderr, "nam: no such node %d\n", nn);
	exit(1);
}

int NetModel::command(int argc, const char *const *argv)
{
	Tcl& tcl = Tcl::instance();
	
	if (argc == 2) {
		if (strcmp(argv[1], "layout") == 0) {
			/*
			 * <net> layout
			 * Compute reasonable defaults for missing node or edge
			 * sizes based on the maximum link delay. Lay out the
			 * nodes and edges as specified in the layout file.
			 */
			scale_estimate();
			layout();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "view") == 0) {
			/*
			 * <net> view <viewName>
			 * Create the window for the network layout/topology.
			 */
			NetView *v = new NetView(argv[2], this);
			v->next_ = views_;
			views_ = v;
			return (TCL_OK);
		}
	} else if (argc >= 4 && strcmp(argv[1], "node") == 0) {
		/*
		 * <net> node <name> <shape> [<size>]
		 * Create a node using the specified name
		 * and the default size and insert it into this
		 * NetModel's list of drawables.
		 */
		double size = defsize;
		if (argc > 4)
			size = atof(argv[4]);
		NamNode *n;
		if (!strcmp(argv[3], "circle"))
			n = new CircleNode(argv[2], size);
		else if (!strcmp(argv[3], "box"))
			n = new BoxNode(argv[2], size);
		else
			n = new HexagonNode(argv[2], size);
		n->next_ = nodes_;
		nodes_ = n;
		n->insert(&drawables_);
		return (TCL_OK);
	} else if (argc == 4) {
		if (strcmp(argv[1], "color") == 0) {
			/*
			 * <net> color <packetClass> <colorName>
			 * Display packets of the specified class using
			 * the specified color.
			 */
			int c = atoi(argv[2]);
			if ((u_int)c > 1024) {
				tcl.resultf("%s color: class %d out of range",
					    argv[0], c);
				return (TCL_ERROR);
			}
			Paint *paint = Paint::instance();
			if (c > nclass_) {
				int n, i;
				for (n = nclass_; n < c; n <<= 1);
				int *p = new int[n];
				for (i = 0; i < nclass_; ++i)
					p[i] = paint_[i];
				delete paint_;
				paint_ = p;
				nclass_ = n;
				int pno = paint->thin();
				for (; i < n; ++i)
					paint_[i] = pno;
			}
			int pno = paint->lookup(Tk_GetUid((char *)argv[3]), 1);
			if (pno < 0) {
				tcl.resultf("%s color: no such color: %s",
					    argv[0], argv[3]);
				return (TCL_ERROR);
			}
			paint_[c] = pno;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ncolor") == 0) {
			/*
			 * <net> ncolor <node> <colorName>
			 * set color of node to the specified color.
			 */
			Paint *paint = Paint::instance();
			NamNode *n = lookupNode(atoi(argv[2]));
			int pno = paint->lookup(Tk_GetUid((char *)argv[3]), 3);
			if (pno < 0) {
				tcl.resultf("%s ncolor: no such color: %s",
					    argv[0], argv[3]);
				return (TCL_ERROR);
			}
			n->paint(pno);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "queue") == 0) {
			/*
			 * <net> queue <src> <dst> <angle>
			 * Create a queue for the edge from 'src' to 'dst'.
			 * Add it to this NetModel's queue list.
			 * Display the queue at the specified angle from
			 * the edge to which it belongs.
			 */
			int src = atoi(argv[2]);
			int dst = atoi(argv[3]);
			EdgeHashNode *h = lookupEdge(src, dst);
			if (h == 0) {
				tcl.resultf("%s queue: no such edge (%d,%d)",
					    argv[0], src, dst);
				return (TCL_ERROR);
			}
			/* XXX can we assume no duplicate queues? */
			double angle = atof(argv[4]);
			NamEdge *e = h->edge;
			angle += e->angle();
			NamQueue *q = new NamQueue(angle);
			h->queue = q;
			q->next_ = queues_;
			queues_ = q;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ecolor") == 0) {
			/*
			 * <net> ecolor <src> <dst> <colorName>
			 * set color of edge to the specified color.
			 */
			Paint *paint = Paint::instance();
			EdgeHashNode* h = lookupEdge(atoi(argv[2]), atoi(argv[3]));
			if (h == 0) {
				tcl.resultf("%s ecolor: no such edge (%s,%s)",
					    argv[0], argv[2], argv[3]);
				return (TCL_ERROR);
			}
			int pno = paint->lookup(Tk_GetUid((char *)argv[4]), 3);
			if (pno < 0) {
				tcl.resultf("%s ncolor: no such color: %s",
					    argv[0], argv[3]);
				return (TCL_ERROR);
			}
			h->edge->paint(pno);
			return (TCL_OK);
		}
	} else if (argc == 7) {
		if (strcmp(argv[1], "link") == 0) {
			/*
			 * <net> link <src> <dst> <bandwidth> <delay> <angle>
			 * Create a link/edge between the specified source
			 * and destination. Add it to this NetModel's list
			 * of drawables and to the source's list of links.
			 */
			NamNode *src = lookupNode(atoi(argv[2]));
			NamNode *dst = lookupNode(atoi(argv[3]));
			double bw = atof(argv[4]);
			double delay = atof(argv[5]);
			double angle = atof(argv[6]);
			NamEdge *e = new NamEdge(src, dst, defsize,
						 bw, delay, angle);
			enterEdge(e);
			e->insert(&drawables_);
			src->add_link(e);
			tcl.resultf("%g", delay);
			return (TCL_OK);
		}
	}
	/* If no NetModel commands matched, try the Object commands. */
	return (TclObject::command(argc, argv));
}

void NetModel::placeEdge(NamEdge* e, NamNode* src) const
{
	if (e->marked() == 0) {
		double nsin, ncos;
		NamNode *dst = e->neighbor();
		SINCOSPI(e->angle(), &nsin, &ncos);
		double x0 = src->x() + src->size() * ncos * .75;
		double y0 = src->y() + src->size() * nsin * .75;
		double x1 = dst->x() - dst->size() * ncos * .75;
		double y1 = dst->y() - dst->size() * nsin * .75;
		e->place(x0, y0, x1, y1);
		
		/* Place the queue here too.  */
		EdgeHashNode *h = lookupEdge(e->src(), e->dst());
		if (h->queue != 0)
			h->queue->place(e->size(), e->x0(), e->y0());

		e->mark();
	}
}

void NetModel::layout()
{
	/* If there is no fixed node, anchor the first one entered at (0,0). */
	NamNode *n;

	for (n = nodes_; n != 0; n = n->next_)
		n->mark(0);
	if (nodes_)
		nodes_->place(0., 0.);

	int did_something;
	do {
		did_something = 0;
		for (n = nodes_; n != 0; n = n->next_)
			did_something |= traverse(n);
	} while (did_something);

	for (n = nodes_; n != 0; n = n->next_)
		for (NamEdge* e = n->links(); e != 0; e = e->next_)
			placeEdge(e, n);
}

void NetModel::move(double& x, double& y, double angle, double d) const
{
	double ncos, nsin;
	SINCOSPI(angle, &nsin, &ncos);
	x += d * ncos;
	y += d * nsin;
}

/*
 * Traverse node n's neighbors and place them based on the
 * delay of their links to n.  The two branches of the if..else
 * are to handle unidirectional links -- we place ourselves if
 * we haven't been placed & our downstream neighbor has.
 */
int NetModel::traverse(NamNode* n)
{
	int did_something = 0;
	for (NamEdge* e = n->links(); e != 0; e = e->next_) {
		NamNode *neighbor = e->neighbor();
		double d = e->delay() + (n->size() + neighbor->size()) * 0.75;
		if (n->marked() && !neighbor->marked()) {
			double x = n->x();
			double y = n->y();
			move(x, y, e->angle(), d);
			neighbor->place(x, y);
			did_something |= traverse(neighbor);
			if (nymax_ < y)
				nymax_ = y;
			if (nymin_ > y)
				nymin_ = y;
		} else if (!n->marked() && neighbor->marked()) {
			double x = neighbor->x();
			double y = neighbor->y();
			move(x, y, e->angle(), -d);
			n->place(x, y);
			did_something = 1;
		}
	}
	return (did_something);
}

/*
 * Compute reasonable defaults for missing node or edge sizes
 * based on the maximum link delay.
 */
void NetModel::scale_estimate()
{
	/* Determine the maximum link delay. */
	double emax = 0.;
	NamNode *n;
	for (n = nodes_; n != 0; n = n->next_) {
		for (NamEdge* e = n->links(); e != 0; e = e->next_)
			if (e->delay() > emax)
				emax = e->delay();
	}
	
	/*
	 * Check for missing node or edge sizes. If any are found,
	 * compute a reasonable default based on the maximum edge
	 * dimension.
	 */
	for (n = nodes_; n != 0; n = n->next_) {
		if (n->size() <= 0.)
			n->size(.1 * emax);
		for (NamEdge* e = n->links(); e != 0; e = e->next_)
			if (e->size() <= 0.)
				e->size(.03 * emax);
	}
}
