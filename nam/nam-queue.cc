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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-queue.cc,v 1.2 1997/03/29 05:08:43 mccanne Exp $ (LBL)";
#endif

#include <osfcn.h>
#include <math.h>

#include "netview.h"
#include "nam-queue.h"
#include "nam-drop.h"

QueueItem::QueueItem(const PacketAttr& p, double tim, long offset)
	: qnext_(0), Animation(tim, offset)
{
	pkt_ = p;
}

void 
QueueItem::draw(NetView* nv, double now) const
{
	nv->polygon(px_, py_, 4, paint_);
	if ((pkt_.attr & 0x100) == 0)
		nv->fill(px_, py_, 4, paint_);
}

void QueueItem::position(float& x, float& y)
{
	/* XXX this is stupid */
	x = px_[0];
	y = py_[0];
}

void QueueItem::locate(float x, float y, float dx, float dy)
{
	px_[0] = x;	py_[0] = y;
	x += dy;	y -= dx;
	px_[1] = x;	py_[1] = y;
	x += dx;	y += dy;
	px_[2] = x;	py_[2] = y;
	x -= dy;	y += dx;
	px_[3] = x;	py_[3] = y;
}

int QueueItem::inside(double now, float px, float py) const
{
	float minx, maxx, miny, maxy;
	int npts = 4;
	maxx = minx = px_[0]; maxy = miny = py_[0];
	while (--npts > 0) {
		if (px_[npts] < minx)
			minx = px_[npts];
		if (px_[npts] > maxx)
			maxx = px_[npts];
		if (py_[npts] < miny)
			miny = py_[npts];
		if (py_[npts] > maxy)
			maxy = py_[npts];
	}
	return (px >= minx && px <= maxx && py >= miny && py <= maxy);
}

const char* QueueItem::info() const
{
	static char text[128];
	sprintf(text, "%s %d: %s\n   %d bytes\n",
		pkt_.type, pkt_.id, pkt_.convid, pkt_.size);
	return (text);
}

NamQueue::NamQueue(float angle)
	: psize_(0.), cnt_(0), nb_(0), angle_(angle)
{
	head_ = 0;
	tail_ = &head_;
}

void NamQueue::place(double psize, double x, double y)
{
	psize_ = psize;

	dx_ = cos(M_PI * angle_) * psize_;
	dy_ = sin(M_PI * angle_) * psize_;

	px_ = 3 * dx_ / 4;
	py_ = 3 * dy_ / 4;

	qx_ = x + 2 * dx_;
	qy_ = y + 2 * dy_;
}

void NamQueue::relocate()
{
	float x = qx_, y = qy_;

	for (QueueItem *q = head_; q != 0; q = q->qnext_) {
		q->locate(x, y, px_, py_);
		x += dx_;
		y += dy_;
	}
}

QueueItem *NamQueue::remove(const PacketAttr& p, int& pos)
{
	QueueItem* q;
	pos = 0;
	for (QueueItem **pp = &head_; (q = *pp) != 0; pp = &q->qnext_) {
		++pos;
		if (q->pkt_.id == p.id
		    && q->pkt_.attr == p.attr
		    && q->pkt_.size == p.size
		    && strcmp(q->pkt_.convid, p.convid) == 0
		    && strcmp(q->pkt_.type, p.type)== 0) {
			--cnt_;
			nb_ -= q->pkt_.size;
			*pp = q->qnext_;
			if (*pp == 0)
				tail_ = pp;
			relocate();
			return (q);
		}
	}
	return (0);
}

void NamQueue::enque(QueueItem *q)
{
	*tail_ = q;
	tail_ = &q->qnext_;
	q->qnext_ = 0;
	++cnt_;
	nb_ += q->pkt_.size;
	relocate();
}

void NamQueue::reset(double now)
{
	while (head_ != 0) {
		QueueItem *n = head_->qnext_;
		delete head_;
		head_ = n;
	}
	head_ = 0;
	tail_ = &head_;
	cnt_ = 0;
	nb_ = 0;
}
