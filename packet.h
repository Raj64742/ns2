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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/packet.h,v 1.16 1997/06/03 21:33:39 kannan Exp $ (LBL)
 */

#ifndef ns_packet_h
#define ns_packet_h

#include <sys/types.h>
#include "config.h"
#include "scheduler.h"

class PacketHeaderClass : public TclClass {
protected:
	PacketHeaderClass(const char* className, int hdrsize);
	int hdrlen_;	// # of bytes from beginning of packet for this hdr
public:
	virtual void bind();
        TclObject* create(int argc, const char*const* argv);
};

class Packet : public Event {
private:
	friend class PacketQueue;
	u_char* bits_;
	u_char* data_;  // variable size buffer for 'data'
	u_int datalen_; // length of variable size buffer
	Packet* next_;	// for queues and the free list
protected:
	static Packet* free_;
public:
	static int hdrlen_;
	Packet() : bits_(0), datalen_(0), next_(0) { }
	u_char* const bits() { return (bits_); }
	Packet* copy() const;
        static Packet* alloc();
        static Packet* alloc(int);
	inline void allocdata(int);
        static void free(Packet*);
	inline u_char* access(int off) { if (off < 0) abort(); return (&bits_[off]); }
	inline u_char* accessdata() {return data_;}
};

#include "trace.h"

inline Packet* Packet::alloc()
{
	Packet* p = free_;
	if (p != 0)
		free_ = p->next_;
	else {
		p = new Packet;
		p->bits_ = new u_char[hdrlen_];
		if (p == 0 || p->bits_ == 0)
			abort();
	}
	return (p);
}

/* allocate a packet with an n byte data buffer */

inline Packet* Packet::alloc(int n)
{
        Packet* p = alloc();
	if (n > 0) 
	       p->allocdata(n);
	return (p);
}

/* allocate an n byte data buffer to an existing packet */

inline void Packet::allocdata(int n)
{
        datalen_ = n;
	data_ = new u_char[n];
	if (data_ == 0)
	        abort();

}

inline void Packet::free(Packet* p)
{
	p->next_ = free_;
	free_ = p;
	if (p->datalen_) {
	        delete p->data_;
		p->datalen_ = 0;
	}
}

inline Packet* Packet::copy() const
{
	Packet* p = alloc();
	memcpy(p->bits(), bits_, hdrlen_);
	if (datalen_) {
	        p->datalen_ = datalen_;
	        p->data_ = new u_char[datalen_];
		memcpy(p->data_, data_, datalen_);
	}
	return (p);
}

#endif
