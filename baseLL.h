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
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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

#ifndef ns_baseLL_h
#define ns_baseLL_h

#include "delay.h"
#include "errmodel.h"

struct hdr_ll {
	int seqno_;		// sequence number
	int ack_;		// ack number

	int& seqno() { return (seqno_);	}
	int& ack() { return (ack_); }
};


class BaseLL : public LinkDelay {
public:
	BaseLL();
	virtual void recv(Packet* p, Handler* h);
	virtual void handle(Event*);
	inline ErrorModel* em() { return em_; }
	inline Queue* ifq() { return ifq_; }
	inline NsObject* sendtarget() { return sendtarget_; }
	inline NsObject* recvtarget() { return recvtarget_; }

protected:
	int command(int argc, const char*const* argv);
	ErrorModel* em_;	// error model
	Queue* ifq_;		// interface queue
        NsObject* sendtarget_;  // usually the link layer of the peer
	NsObject* recvtarget_;  // usually the classifier of the same node
	int off_ll_;            // offset of link-layer header
	int seqno_;             // link-layer sequence number
};


static class BaseLLHeaderClass : public PacketHeaderClass {
public:
	BaseLLHeaderClass() : PacketHeaderClass("PacketHeader/LL", sizeof(hdr_ll)) {}
} class_llhdr;

#endif
