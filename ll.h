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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ll.h,v 1.10 1998/01/23 08:11:08 gnguyen Exp $ (UCB)
 */

#ifndef ns_ll_h
#define ns_ll_h

#include "delay.h"

enum LLFrameType {
	LL_DATA   = 0x0001,
	LL_ACK    = 0x0010,
};

struct hdr_ll {
	LLFrameType lltype_;	// link-layer frame type
	int seqno_;		// sequence number
	int ack_;		// acknowledgement number

	inline LLFrameType& lltype() { return lltype_; }
	inline int& seqno() { return seqno_; }
	inline int& ack() { return ack_; }
};


class LL : public LinkDelay {
public:
	LL();
	virtual void recv(Packet* p, Handler* h);
	virtual void recvfrom(Packet* p);
	virtual void sendto(Packet* p, Handler* h);
protected:
	int command(int argc, const char*const* argv);
	int seqno_;		// link-layer sequence number
	int macDA_;		// destination MAC address
	int off_ll_;		// offset of link-layer header
	int off_mac_;		// offset of MAC header
        Queue* ifq_;		// interface queue
        NsObject* sendtarget_;	// where packet is passed down the stack
	NsObject* recvtarget_;	// where packet is passed up the stack
};

#endif
