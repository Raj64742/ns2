/* -*- c++ -*-
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/new-ll.h,v 1.3 1999/01/26 18:30:44 haoboy Exp $ (UCB)
 */

#ifndef ns_new_ll_h
#define ns_new_ll_h

#include <delay.h>
#include <queue.h>

#include <arp.h>
#include <node.h>
#include <god.h>

enum newLLFrameType {
	LL_DATA	  = 0x0001,
	LL_ACK    = 0x0010,
};

struct newhdr_ll {
	newLLFrameType lltype_;	// link-layer frame type
	int seqno_;		// sequence number
	int ack_;		// acknowledgement number
	static int offset_;
	inline static int& offset() { return offset_; }
	//inline static newhdr_ll* access(Packet* p) {
	//return (newhdr_ll*) p->access(offset_);
	//}
	inline newLLFrameType& lltype() { return lltype_; }
	inline int& seqno() { return seqno_; }
	inline int& ack() { return ack_; }
};


class newLL : public LinkDelay {
public:
	newLL();
	virtual void	recv(Packet* p, Handler* h);
	void 		handle(Event *e);
private:
	inline int initialized() {
		return (arptable_ && mac_ && uptarget_ && downtarget_);
	}

	friend void ARPTable::arpinput(Packet *p, newLL* ll);
	friend void ARPTable::arprequest(nsaddr_t src, nsaddr_t dst, newLL* ll);

	int command(int argc, const char*const* argv);
	void dump(void);

	virtual void	sendUp(Packet* p);
	virtual void	sendDown(Packet* p);

	int seqno_;		// link-layer sequence number
	int off_newll_;		// offset of link-layer header
	int off_newmac_;		// offset of MAC header
        NsObject* downtarget_;	// where packet is passed down the stack
	NsObject* uptarget_;	// where packet is passed up the stack

	ARPTable	*arptable_;
	newMac*		mac_;		// MAC object
        Queue*		ifq_;		/* interface queue */
	double		mindelay_;
};

#endif
