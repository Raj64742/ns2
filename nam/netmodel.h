/*
 * Copyright (c) 1991,1993-1994 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/netmodel.h,v 1.1 1997/03/29 04:38:07 mccanne Exp $ (LBL)
 */

#ifndef nam_netmodel_h
#define nam_netmodel_h

#include "nam-trace.h"

class NamEdge;
class NamNode;
class TraceEvent;
class NamQueue;
class Animation;
class NetView;
class Paint;
class NamPacket;
struct BBox;

/*
 * The underlying model for a NetView.  We factor this state out of the
 * NetView since we might have multiple displays of the same network
 * (i.e., zoomed windows), as opposed to multiple interpretations 
 * (i.e, strip charts).
 */
class NetModel : public TraceHandler {
    public:
	NetModel();
	virtual ~NetModel();

	void render(NetView*);
	void update(double, Animation*);

	/* TraceHandler hooks */
	void update(double);
	void reset(double);
	void handle(const TraceEvent&, double);

	void BoundingBox(BBox&);
	void addView(NetView*);
	Animation* inside(double now, float px, float py) const;
    protected:
	void scale_estimate();
	int traverse(NamNode* n);
	void move(double& x, double& y, double angle, double d) const;
	void layout();
	void placeEdge(NamEdge* e, NamNode* src) const;
	NamNode* lookupNode(int nn) const;
	int command(int argc, const char*const* argv);
#define EDGE_HASH_SIZE 256
	inline int ehash(int src, int dst) const {
		return (src ^ (dst << 4)) & (EDGE_HASH_SIZE-1);
	}
	struct EdgeHashNode {
		EdgeHashNode* next;
		int src;
		int dst;
		NamEdge* edge;
		NamQueue* queue;
	};
	EdgeHashNode* lookupEdge(int, int) const;
	void enterEdge(NamEdge* e);
	void saveState(double);

	NetView* views_;
	NamNode* nodes_;
	double now_;
	double nymin_, nymax_;	/*XXX*/
	Animation *drawables_;
	Animation *animations_;
	NamQueue *queues_;
	EdgeHashNode* hashtab_[EDGE_HASH_SIZE];

	int nclass_;		// no. of classes (colors)
	int* paint_;
};

#endif
