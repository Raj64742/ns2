/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/queue.h,v 1.10.2.4 1997/04/26 01:47:49 hari Exp $ (LBL)
 */

#ifndef ns_queue_h
#define ns_queue_h

#include "connector.h"
#include "packet.h"
#include "ip.h"
#include "queue-monitor.h"


class PacketQueue {
public:
	PacketQueue() : head_(0), tail_(&head_), len_(0), qm_(0) {}
	inline int length() const { return (len_); }
	inline void enque(Packet* p) {
		*tail_ = p;
		tail_ = &p->next_;
		*tail_ = 0;
		++len_;
		if (qm_)
			qm_->in(p);
	}
	Packet* deque() {
		Packet* p = head_;
		if (p != 0) {
			head_ = p->next_;
			if (head_ == 0)
				tail_ = &head_;
			--len_;
			if (qm_)
				qm_->out(p);
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
	void purge(Packet *, Packet *);	/* remove pkt 1 (located after 2) */
	inline Packet* head() { return head_; }
	QueueMonitor*& qm() { return qm_; }
protected:
	Packet* head_;
	Packet** tail_;
	QueueMonitor* qm_;	// queue monitor
	int len_;		// packet count
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
	int blocked() const { return (blocked_ == 1); }
	void unblock() { blocked_ = 0; }
	void block() { blocked_ = 1; }
 protected:
	Queue();
	int command(int argc, const char*const* argv);
	void reset();
	void drop(Packet* p);
	NsObject* drop_;	/* node to send all dropped packets to */
	int qlim_;		/* maximum allowed pkts in queue */
	int blocked_;
	PacketQueue q_;
	QueueHandler qh_;
};


#endif
