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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-edge.h,v 1.1 1997/03/29 04:37:59 mccanne Exp $ (LBL)
 */

#ifndef nam_edge_h
#define nam_edge_h

#include <math.h>
#include "animation.h"
#include "transform.h"

class NetView;
class Transform;
class NamNode;

class NamEdge : public Animation {
    public:
	NamEdge(NamNode* src, NamNode* dst, double ps,
		double bw, double delay, double angle);
	~NamEdge();
	virtual void draw(NetView*, double) const;
	virtual void reset(double);
	inline double GetDelay() const { return (delay_); }
	inline double PacketHeight() const { return (psize_); }
	inline double size() const { return (psize_); }
	inline void size(double s) { psize_ = s; }
	inline int src() const { return (src_); }
	inline int dst() const { return (dst_); }
	inline int match(int s, int d) const { return (src_ == s && dst_ == d); }
	inline const Transform& transform() const { return (matrix_); }
	inline double txtime(int n) const { return (double(n << 3)/bandwidth_); }
	inline double angle() const { return (angle_); }
	inline double delay() const { return (delay_); }
	inline double x0() const { return (x0_); }
	inline double y0() const { return (y0_); }
	inline NamNode* neighbor() const { return (neighbor_); }
	void place(double, double, double, double);
	int inside(double, float, float) const;
	const char* info() const;
	inline int marked() const { return (marked_); }
	inline int mark() { marked_ = 1; }

	NamEdge* next_;
    private:
	const int src_, dst_;
	NamNode* neighbor_;
	double x0_, y0_;
	double x1_, y1_;
	double psize_;		/* packet size XXX */
	double angle_;
	double bandwidth_;	/* link bandwidth (bits/sec) */
	double delay_;		/* link delay */
	Transform matrix_;	/* rotation matrix for packets */
	int marked_;
};

#endif
