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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-node.cc,v 1.2 1997/03/29 06:20:06 mccanne Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
	
#include "netview.h"
#include "nam-node.h"
#include "nam-edge.h"
#include "paint.h"

NamNode::NamNode(const char* name, double size) :
	Animation(0, 0),
	next_(0),
	size_(size),
	x_(0.),
	y_(0.),
	links_(0),
	anchor_(0),
	mark_(0)
{
	label_ = new char[strlen(name) + 1];
	strcpy(label_, name);
	nn_ = atoi(name); /*XXX*/
	paint_ = Paint::instance()->thick();
}

void NamNode::size(double s)
{
	size_ = s;
	update_bb();
}

void NamNode::update_bb()
{
	double off = 0.5 * size_;
	/*XXX*/
	bb_.xmin = x_ - off;
	bb_.ymin = y_ - off;
	bb_.xmax = x_ + off;
	bb_.ymax = y_ + off;
}

int NamNode::inside(double, float px, float py) const
{
	return (px >= bb_.xmin && px <= bb_.xmax &&
		py >= bb_.ymin && py <= bb_.ymax);
}

const char* NamNode::info() const
{
	static char text[128];
	sprintf(text, "%s", label_);
	return (text);
}

void NamNode::add_link(NamEdge* e)
{
	e->next_ = links_;
	links_ = e;
}

void NamNode::label(const char* name, int anchor)
{
	delete label_;
	label_ = new char[strlen(name) + 1];
	strcpy(label_, name);
	anchor_ = anchor;
}

void NamNode::drawlabel(NetView* nv) const
{
	/*XXX node number */
	if (label_ != 0)
		nv->string(x_, y_, size_, label_, anchor_);
}

void NamNode::reset(double)
{
	paint_ = Paint::instance()->thick();
}

void NamNode::place(double x, double y)
{ 
	x_ = x;
	y_ = y;
	mark_ = 1;
	update_bb();
}

BoxNode::BoxNode(const char* name, double size) : NamNode(name, size)
{
	BoxNode::size(size);
}

void BoxNode::size(double s)
{
	NamNode::size(s);
	double delta = 0.5 * s;
	x0_ = x_ - delta;
	y0_ = y_ - delta;
	x1_ = x_ + delta;
	y1_ = y_ + delta;
}

void BoxNode::draw(NetView* nv, double now) const
{
	nv->rect(x0_, y0_, x1_, y1_, paint_);
	drawlabel(nv);
}

void BoxNode::place(double x, double y)
{
	NamNode::place(x, y);

	double delta = 0.5 * size_;
	x0_ = x_ - delta;
	y0_ = y_ - delta;
	x1_ = x_ + delta;
	y1_ = y_ + delta;
}

CircleNode::CircleNode(const char* name, double size) : NamNode(name, size)
{
	CircleNode::size(size);
}

void CircleNode::size(double s)
{
	NamNode::size(s);
	radius_ = 0.5 * s;
}

void CircleNode::draw(NetView* nv, double now) const
{
	nv->circle(x_, y_, radius_, paint_);
	drawlabel(nv);
}

HexagonNode::HexagonNode(const char* name, double size) : NamNode(name, size)
{
	HexagonNode::size(size);
}

void HexagonNode::size(double s)
{
	NamNode::size(s);
	double hd = 0.5 * s;
	double qd = 0.5 * hd;

	xpoly_[0] = x_ - hd; ypoly_[0] = y_;
	xpoly_[1] = x_ - qd; ypoly_[1] = y_ + hd;
	xpoly_[2] = x_ + qd; ypoly_[2] = y_ + hd;
	xpoly_[3] = x_ + hd; ypoly_[3] = y_;
	xpoly_[4] = x_ + qd; ypoly_[4] = y_ - hd;
	xpoly_[5] = x_ - qd; ypoly_[5] = y_ - hd;
}

void HexagonNode::draw(NetView* nv, double now) const
{
	nv->polygon(xpoly_, ypoly_, 6, paint_);
	drawlabel(nv);
}

void HexagonNode::place(double x, double y)
{
	NamNode::place(x, y);

	double hd = 0.5 * size_;
	double qd = 0.5 * hd;
	xpoly_[0] = x_ - hd; ypoly_[0] = y_;
	xpoly_[1] = x_ - qd; ypoly_[1] = y_ + hd;
	xpoly_[2] = x_ + qd; ypoly_[2] = y_ + hd;
	xpoly_[3] = x_ + hd; ypoly_[3] = y_;
	xpoly_[4] = x_ + qd; ypoly_[4] = y_ - hd;
	xpoly_[5] = x_ - qd; ypoly_[5] = y_ - hd;
}
