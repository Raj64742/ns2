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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-edge.cc,v 1.2 1997/03/29 06:20:06 mccanne Exp $ (LBL)";
#endif

#include <math.h>

#include "nam-edge.h"
#include "nam-node.h"
#include "netview.h"
#include "transform.h"
#include "paint.h"

NamEdge::NamEdge(NamNode* src, NamNode* dst, double ps,
		 double bw, double delay, double angle) :
	Animation(0, 0),
	src_(src->num()), dst_(dst->num()),
	neighbor_(dst),
	x0_(0), y0_(0),
	x1_(0), y1_(0),
	psize_(ps),
	angle_(angle),
	bandwidth_(bw),
	delay_(delay),
	marked_(0)
{
	paint_ = Paint::instance()->thick();
}

NamEdge::~NamEdge()
{
}

void NamEdge::place(double x0, double y0, double x1, double y1)
{
	x0_ = x0;
	y0_ = y0;
	x1_ = x1;
	y1_ = y1;

	double dx = x1 - x0;
	double dy = y1 - y0;
	/*XXX*/
	delay_ = sqrt(dx * dx + dy * dy);
	matrix_.rotate((180 / M_PI) * atan2(dy, dx));
	matrix_.translate(x0, y0);

	bb_.xmin = x0; bb_.ymin = y0;
	bb_.xmax = x1; bb_.ymax = y1;
}

void NamEdge::draw(NetView* view, double now) const
{
	view->line(x0_, y0_, x1_, y1_, paint_);
}

void NamEdge::reset(double)
{
	paint_ = Paint::instance()->thick();
}

int NamEdge::inside(double, float px, float py) const
{
	return (px >= bb_.xmin &&
		px <= bb_.xmax &&
		py >= bb_.ymin - .0005 &&
		py <= bb_.ymax + .0005);
}

const char* NamEdge::info() const
{
	static char text[128];
	sprintf(text, "link %d-%d:\n  bw: %g bits/sec\n  delay: %g sec\n",
		src_, dst_, bandwidth_, delay_);
	return (text);
}
