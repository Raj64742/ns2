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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-node.h,v 1.1 1997/03/29 04:38:01 mccanne Exp $ (LBL)
 */

#ifndef nam_node_h
#define nam_node_h

class NamEdge;
class NetView;

#include "animation.h"

class NamNode : public Animation {
    public:
	inline const char* name() const { return (label_); }
	inline int num() const { return (nn_); }
	inline double size() const { return (size_); }
	virtual void size(double s);
	virtual void reset(double);
	inline double x() const { return (x_); }
	inline double y() const { return (y_); }
	virtual void place(double x, double y);
	void label(const char* name, int anchor);
	inline int anchor() const { return (anchor_); }
	inline void anchor(int v) { anchor_ = v; }
	void add_link(NamEdge*);
	int inside(double, float, float) const;
	const char* info() const;
	inline NamEdge* links() const { return (links_); }
	inline int marked() const { return (mark_); }
	inline void mark(int v) { mark_ = v; }

	NamNode* next_;
    protected:
	NamNode(const char* name, double size);
	void update_bb();
	void drawlabel(NetView*) const;

	double size_;
	double x_, y_;
	char* label_;
	NamEdge* links_;
	int nn_;
	int anchor_;
	int mark_;
};

class BoxNode : public NamNode {
 public:
	BoxNode(const char* name, double size);
	virtual void size(double s);
	virtual void draw(NetView*, double now) const;
	virtual void place(double x, double y);
 private:
	float x0_, y0_;
	float x1_, y1_;
};

class CircleNode : public NamNode {
 public:
	CircleNode(const char* name, double size);
	virtual void size(double s);
	virtual void draw(NetView*, double now) const;
 private:
	float radius_;
};

class HexagonNode : public NamNode {
 public:
	HexagonNode(const char* name, double size);
	virtual void size(double s);
	virtual void draw(NetView*, double now) const;
	virtual void place(double x, double y);
 private:
	float ypoly_[6];
	float xpoly_[6];
};

#endif
