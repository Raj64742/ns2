/*
 * Copyright (c) 1991-1994 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and the Network Research Group at
 *	Lawrence Berkeley Laboratory.
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
 * Routing code for general topologies based on min-cost routing algorithm in
 * Bertsekas' book.  Written originally by S. Keshav, 7/18/88
 * (his work covered by identical UC Copyright)
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/route.cc,v 1.6 1997/07/14 08:52:04 kannan Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <assert.h>
#include "Tcl.h"

#define INFINITY 0x3fff
#define INDEX(i, j, N) ((N) * (i) + (j))

class RouteLogic : public TclObject {
public:
	RouteLogic();
	~RouteLogic();
	int command(int argc, const char*const* argv);
protected:
	void check(int);
	void alloc(int n);
	void insert(int src, int dst, int cost);
	void reset(int src, int dst);
	void compute_routes();
	int* adj_;
	int* route_;
	int size_;
	int maxnode_;
};

class RouteLogicClass : public TclClass {
public:
	RouteLogicClass() : TclClass("RouteLogic") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RouteLogic());
	}
} routelogic_class;

int RouteLogic::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "compute") == 0) {
			compute_routes();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "reset") == 0) {
			delete[] adj_;
			adj_ = 0;
			size_ = 0;
			return (TCL_OK);
		}
	} else if (argc >= 4) {
		if (strcmp(argv[1], "insert") == 0) {
			int src = atoi(argv[2]) + 1;
			int dst = atoi(argv[3]) + 1;
			if (src <= 0 || dst <= 0) {
				tcl.result("negative node number");
				return (TCL_ERROR);
			}
			int cost = (argc == 5 ? atoi(argv[4]) : 1);
			insert(src, dst, cost);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "reset") == 0) {
			int src = atoi(argv[2]) + 1;
			int dst = atoi(argv[3]) + 1;
			if (src <= 0 || dst <= 0) {
				tcl.result("negative node number");
				return (TCL_ERROR);
			}
			reset(src, dst);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lookup") == 0) {
			if (route_ == 0) {
				tcl.result("routes not computed");
				return (TCL_ERROR);
			}
			int src = atoi(argv[2]) + 1;
			int dst = atoi(argv[3]) + 1;
			if (src >= size_ || dst >= size_) {
				tcl.result("node out of range");
				return (TCL_ERROR);
			}
			tcl.resultf("%d", route_[INDEX(src, dst, size_)] - 1);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

RouteLogic::RouteLogic()
{
	size_ = 0;
	adj_ = 0;
	route_ = 0;
}
	
RouteLogic::~RouteLogic()
{
	delete[] adj_;
	delete[] route_;
}

void RouteLogic::alloc(int n)
{
	size_ = n;
	n *= n;
	adj_ = new int[n];
	for (int i = 0; i < n; ++i)
		adj_[i] = INFINITY;
}

/*
 * Check that we have enough storage in the adjacency array
 * to hold a node numbered "n"
 */
void RouteLogic::check(int n)
{
	if (n < size_)
		return;

	int* old = adj_;
	int osize = size_;
	int m = osize;
	if (m == 0)
		m = 16;
	while (m <= n)
		m <<= 1;

	alloc(m);
	for (int i = 0; i < osize; ++i) {
		for (int j = 0; j < osize; ++j)
			adj_[INDEX(i, j, m)] = old[INDEX(i, j, osize)];
	}
	size_ = m;
	delete[] old;
}

void RouteLogic::insert(int src, int dst, int cost)
{
	check(src);
	check(dst);
	adj_[INDEX(src, dst, size_)] = cost;
}

void RouteLogic::reset(int src, int dst)
{
	assert(src < size_);
	assert(dst < size_);
	adj_[INDEX(src, dst, size_)] = INFINITY;
}

void RouteLogic::compute_routes()
{
	int n = size_;
	int* hopcnt = new int[n];
	int* parent = new int[n];
#define ADJ(i, j) adj_[INDEX(i, j, size_)]
#define ROUTE(i, j) route_[INDEX(i, j, size_)]
	delete[] route_;
	route_ = new int[n * n];
	memset((char *)route_, 0, n * n * sizeof(route_[0]));

	/* do for all the sources */
	int k;
	for (k = 1; k < n; ++k) {
		int v;
		for (v = 0; v < n; v++)
			parent[v] = v;

		/* set the route for all neighbours first */
		for (v = 1; v < n; ++v) {
			if (parent[v] != k) {
				hopcnt[v] = ADJ(k, v);
				if (hopcnt[v] != INFINITY)
					ROUTE(k, v) = v;
			}
		}
		for (v = 1; v < n; ++v) {
			/*
			 * w is the node that is the nearest to the subtree
			 * that has been routed
			 */
			int o = 0;
			/* XXX */
			hopcnt[0] = INFINITY;
			int w;
			for (w = 1; w < n; w++)
				if (parent[w] != k && hopcnt[w] < hopcnt[o])
					o = w;
			parent[o] = k;
			/*
			 * update distance counts for the nodes that are
			 * adjacent to o
			 */
			if (o == 0)
				continue;
			for (w = 1; w < n; w++) {
				if (parent[w] != k &&
				    hopcnt[o] + ADJ(o, w) < hopcnt[w]) {
					ROUTE(k, w) = ROUTE(k, o);
					hopcnt[w] = hopcnt[o] + ADJ(o, w);
				}
			}
		}
	}
	/*
	 * The route to yourself is yourself.
	 */
	for (k = 1; k < n; ++k)
		ROUTE(k, k) = k;

	delete[] hopcnt;
	delete[] parent;
}
