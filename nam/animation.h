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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/animation.h,v 1.1 1997/03/29 04:37:56 mccanne Exp $ (LBL)
 */

#ifndef nam_animation_h
#define nam_animation_h

#include "bbox.h"
#include "state.h"

class NetView;

class Animation {
    public:
	virtual ~Animation();
	virtual void draw(NetView*, double now) const = 0;
	/* XXX */
	inline void merge(BBox& b) { 
		if (b.xmin > bb_.xmin)
			b.xmin = bb_.xmin;
		if (b.ymin > bb_.ymin)
			b.ymin = bb_.ymin;
		if (b.xmax < bb_.xmax)
			b.xmax = bb_.xmax;
		if (b.ymax < bb_.ymax)
			b.ymax = bb_.ymax;
	}
	virtual void update(double now);
	virtual void reset(double now);
	virtual int inside(double now, float px, float py) const;
	virtual const char* info() const;
	void insert(Animation **);
	void paint(int id) { paint_ = id; }
	inline StateInfo stateInfo() { return (si_); }
	inline int paint() const { return (paint_); }
	inline Animation* next() const { return (next_); }
	inline Animation** prev() const { return (prev_); }
    protected:
	Animation();
	Animation(double, long);

	BBox bb_;
	/*
	 * id for X graphics context
	 * (e.g., color and linewidth)
	 */
	int paint_;
	
	StateInfo si_;
	Animation* next_;
	Animation** prev_;
};

#endif
