/*
 * Copyright (c) 1997 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/packet.h,v 1.6 1997/03/18 23:42:57 mccanne Exp $ (LBL)
 */

#ifndef ns_packet_h
#define ns_packet_h

#include <sys/types.h>
#include "config.h"
#include "scheduler.h"

class PacketHeader : public TclObject {
protected:
	int offset_;	// # of bytes from beginning of packet for this hdr
public:
	PacketHeader() : offset_(-1) { }
	virtual int hdrsize() = 0; 
	void setbase(int off) { offset_ = off; }
};

class Packet : public Event {
private:
	friend class PacketQueue;
	u_char* bits_;
	static int hdrsize_;
	Packet* next_;	// for queues and the free list
protected:
	static Packet* free_;
	static int uidcnt_;
public:
	Packet() : bits_(0), next_(0) { }
	u_char* const bits() { return (bits_); }
	Packet* copy() const;
	static void addhsize(int grow) { hdrsize_ += grow; }
        static Packet* alloc();
        static void free(Packet*);
};

#include "trace.h"

inline Packet* Packet::alloc()
{
	Packet* p = free_;
	if (p != 0)
		free_ = p->next_;
	else {
		p = new Packet;
		p->bits_ = new u_char[hdrsize_];
		if (p == 0 || p->bits_ == 0)
			abort();
	}
	// for tracing packets
	TraceHeader *th = TraceHeader::access(p->bits_);
	th->uid() = uidcnt_++;
	return (p);
}

inline void Packet::free(Packet* p)
{
	p->next_ = free_;
	free_ = p;
}

inline Packet* Packet::copy() const
{
	Packet* p = alloc();
	memcpy(p->bits(), bits_, hdrsize_);
	TraceHeader *th = TraceHeader::access(p->bits_);
	th->uid() = uidcnt_++;
	return (p);
}

#endif
