/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/queue.h,v 1.1.1.1 1996/12/19 03:22:45 mccanne Exp $ (LBL)
 */

#ifndef ns_queue_h
#define ns_queue_h

#include "node.h"
#include "packet.h"

class PacketQueue {
public:
	PacketQueue() : head_(0), tail_(&head_), len_(0), bcount_(0) {}
	inline int length() const { return (len_); }
	inline int bcount() const { return (bcount_); }
	inline void enque(Packet* p) {
		*tail_ = p;
		tail_ = &p->next_;
		*tail_ = 0;
		++len_;
		bcount_ += p->size_;
	}
	Packet* deque() {
		Packet* p = head_;
		if (p != 0) {
			--len_;
			bcount_ -= p->size_;
			head_ = p->next_;
			if (head_ == 0)
				tail_ = &head_;
		}
		return (p);
	}
	Packet* lookup(int n) {
		for (Packet* p = head_; p != 0; p = p->next_) {
			if (--n < 0)
				return (p);
		}
		return (0);
	}
	/* remove a specific packet, which must be in the queue */
	void remove(Packet*);
protected:
	Packet* head_;
	Packet** tail_;
	int len_;		// packet count
	int bcount_;		// byte count
};

class DefaultDropNode : public Node {
public:
	void recv(Packet*, Handler*);
};

class Queue;

class QueueHandler : public Handler {
public:
	inline QueueHandler(Queue& q) : queue_(q) {}
	void handle(Event*);
 private:
	Queue& queue_;
};

class Queue : public Connector {
 public:
	virtual void enque(Packet*) = 0;
	virtual Packet* deque() = 0;
	void recv(Packet*, Handler*);
	void resume();
 protected:
	Queue();
	int command(int argc, const char*const* argv);
	inline void drop(Packet* p) { drop_->recv(p); }
	Node* drop_;		/* node to send all dropped packets to */
	int qlim_;		/* maximum allowed pkts in queue */
 private:
	int blocked_;
	DefaultDropNode dnode_;
	QueueHandler qh_;
};


#endif
