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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-packet.cc,v 1.1 1997/03/29 04:38:01 mccanne Exp $ (LBL)";
#endif

#include "transform.h"
#include "netview.h"
#include "nam-edge.h"
#include "nam-packet.h"

/*
 * Compute the start and end points of the packet in the one dimensional
 * time space [0, link delay].
 */
inline int NamPacket::EndPoints(double now, double& tail, double& head) const
{
	int doarrow;

	if (now < ta_) {
		head = now - ts_;
		doarrow = 1;
	} else {
		head = edge_->GetDelay();
		doarrow = 0;
	}
	double t = now - tx_;
	tail = (t <= ts_) ? 0. : t - ts_;

	return (doarrow);
}

NamPacket::NamPacket(NamEdge *e, const PacketAttr& p, double s, double txtime,
	       long offset) : Animation(s, offset)
{
	edge_ = e;
	pkt_ = p;
	ts_ = s;
	ta_ = s + e->GetDelay();
	tx_ = txtime;
}

NamPacket::~NamPacket()
{
}

/*
 * Compute the unit-space points for the packet polygon.
 * Return number of points in polygon.
 */
int NamPacket::ComputePolygon(double now, float ax[5], float ay[5]) const
{
	double tail, head;
	int doarrow = EndPoints(now, tail, head);
	double deltap = head - tail;
	const Transform& m = edge_->transform();
	double height = edge_->PacketHeight();
	float bot = 0.2 * height;
	float top = 1.2 * height;
	float mid = 0.7 * height;
	/*XXX put some air between packet and link */
	bot += 0.5 * height;
	top += 0.5 * height;
	mid += 0.5 * height;

	if (doarrow && deltap >= height) {
		/* packet with arrowhead */
		m.map(tail, bot, ax[0], ay[0]);
		m.map(tail, top, ax[1], ay[1]);
		m.map(head - 0.75 * height, top, ax[2], ay[2]);
		m.map(head, mid, ax[3], ay[3]);
		m.map(head - 0.75 * height, bot, ax[4], ay[4]);
		return (5);
	} else {
		/* packet without arrowhead */
		m.map(tail, bot, ax[0], ay[0]);
		m.map(tail, top, ax[1], ay[1]);
		m.map(head, top, ax[2], ay[2]);
		m.map(head, bot, ax[3], ay[3]);
		return (4);
	}
}

int NamPacket::inside(double now, float px, float py) const
{
	if (now < ts_ || now > ta_ + tx_)
		return (0);

	float x[5], y[5], minx, maxx, miny, maxy;
	int npts;
	npts = ComputePolygon(now, x, y);
	maxx = minx = x[0]; maxy = miny = y[0];
	while (--npts > 0) {
		if (x[npts] < minx)
			minx = x[npts];
		if (x[npts] > maxx)
			maxx = x[npts];
		if (y[npts] < miny)
			miny = y[npts];
		if (y[npts] > maxy)
			maxy = y[npts];
	}
	return (px >= minx && px <= maxx && py >= miny && py <= maxy);
}

const char* NamPacket::info() const
{
	static char text[128];
	sprintf(text, "%s %d: %s\n  Sent at %.6f\n  %d bytes\n",
		pkt_.type, pkt_.id, pkt_.convid, ts_, pkt_.size);
	return (text);
}

void NamPacket::draw(NetView* nv, double now) const
{
	/* XXX */
	if (now < ts_ || now > ta_ + tx_)
		return;

	float x[5], y[5];
	int npts;
	npts = ComputePolygon(now, x, y);

	nv->polygon(x, y, npts, paint_);
	if ((pkt_.attr & 0x100) == 0)
	    nv->fill(x, y, npts, paint_);
}

void NamPacket::update(double now)
{
	if (now > ta_ + tx_ || now < ts_)
		/* XXX this does not belong here */
		delete this;
	/* XXX Alert edge that we're here. */
}
