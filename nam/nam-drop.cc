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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-drop.cc,v 1.1 1997/03/29 04:37:58 mccanne Exp $ (LBL)";
#endif

#include "netview.h"
#include "nam-drop.h"

NamDrop::NamDrop(float cx, float cy, float b, float size, double now, long offset,
	   const PacketAttr& p)
	: x_(cx), y_(cy), bot_(b), psize_(size), start_(now), pkt_(p), rot_(0),
	  Animation(now, offset)
{
}

void NamDrop::draw(NetView* c, double now) const
{
	/* XXX nuke array */
	float fx[4], fy[4];
	double yy = CurPos(now);

	if (rot_ & 2) {
		double d = (0.75 * 0.7071067812) * psize_;
		fx[0] = x_;
		fy[0] = yy - d;
		fx[1] = x_ - d;
		fy[1] = yy;
		fx[2] = x_;
		fy[2] = yy + d;
		fx[3] = x_ + d;
		fy[3] = yy;
	} else {
		double d = (0.75 * 0.5) * psize_;
		fx[0] = x_ - d;
		fy[0] = yy - d;
		fx[1] = x_ - d;
		fy[1] = yy + d;
		fx[2] = x_ + d;
		fy[2] = yy + d;
		fx[3] = x_ + d;
		fy[3] = yy - d;
	}
	c->polygon(fx, fy, 4, paint_);
	if ((pkt_.attr & 0x100) == 0)
		c->fill(fx, fy, 4, paint_);
}

void NamDrop::update(double now)
{
	++rot_;
	if (now < start_ || CurPos(now) < bot_)
		/* XXX */
		delete this;
}

void NamDrop::reset(double now)
{
	if (now < start_ || CurPos(now) < bot_)
		/* XXX */
		delete this;
}

int NamDrop::inside(double now, float px, float py) const
{
	float minx, maxx, miny, maxy;
	double yy = CurPos(now);
	double d = (0.75 * 0.7071067812) * psize_;
	return (px >= x_ - d && px <= x_ + d && py >= yy - d && py <= yy + d);
}

const char* NamDrop::info() const
{
	static char text[128];
	sprintf(text, "%s %d: %s\n  dropped at %g\n  %d bytes\n",
		pkt_.type, pkt_.id, pkt_.convid, start_, pkt_.size);
	return (text);
}
