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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-packet.h,v 1.1 1997/03/29 04:38:02 mccanne Exp $ (LBL)
 */

#ifndef nam_packet_h
#define nam_packet_h

#include "animation.h"

static const int PacketTypeSize = 8;

class NetView;

class NamPacket : public Animation {
 public:
        NamPacket(NamEdge*, const PacketAttr&, double, double, long);
        ~NamPacket();
	virtual void draw(NetView*, double now) const;
	virtual void update(double now);
	int ComputePolygon(double now, float ax[5], float ay[5]) const;
	virtual int inside(double now, float px, float py) const;
	virtual const char* info() const;
	inline int id() const { return pkt_.id; }
	inline int size() const { return pkt_.size; }
	inline double start() const { return ts_; }
	inline double finish() const { return ta_ + tx_; }
	inline const char* type() const { return pkt_.type; }
	inline const char* convid() const { return pkt_.convid; }
 private:
	int EndPoints(double, double&, double&) const;

	NamEdge* edge_;
	PacketAttr pkt_;
	double ts_;		/* time of start of transmission */
	double ta_;		/* time of arrival of leading edge */
	double tx_;		/* tranmission time of packet on cur link */
};
#endif
