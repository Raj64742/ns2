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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/queue.h,v 1.10.2.8 1997/04/30 19:51:09 padmanab Exp $ (LBL)
 */

#ifndef ns_queue_h
#define ns_queue_h

#include "connector.h"
#include "packet.h"
#include "ip.h"
#include "queue-monitor.h"


class PacketQueue {
public:
	PacketQueue() : head_(0), tail_(&head_), len_(0), qm_(0), ack_count_(0), data_count_(0), acks_to_send_(0) {}
	inline int length() const { return (len_); }
	inline void enque(Packet* p) {
		*tail_ = p;
		tail_ = &p->next_;
		*tail_ = 0;
		++len_;
		if (qm_)
			qm_->in(p);
	}

	inline void enque(Packet* p, int off_cmn) {
		int type = ((hdr_cmn*)p->access(off_cmn))->ptype_;
		if (type == PT_ACK) 
			ack_count_++;
		else
			data_count_++;
		enque(p);
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
	/* interleave between TCP acks and others while dequeuing */
	Packet* deque_interleave(int off_cmn);
	/* deque TCP acks before any other type of packet */
	Packet* deque_acksfirst(int off_cmn);
	/* remove a specific packet, which must be in the queue */
	void remove(Packet*);
	/* 
	 * remove a specific packet, which must be in the queue, and update 
	 * ack_count_ and data_count_
	 */
	void remove(Packet*, int off_cmn);
	/* 
	 * If a packet needs to be removed because the queue is full, pick the TCP ack
	 * closest to the head. Otherwise, drop the specified pkt (target).
	 */
	Packet* remove_ackfromfront(Packet* target, int off_cmn);
	/* 
	 * Remove a specific packet given pointers to it and its predecessor
	 * in the queue. p and/or pp may be NULL.
	 */
	void remove(Packet* p, Packet* pp);

	void purge(Packet *, Packet *);	/* remove pkt 1 (located after 2) */

	inline Packet* head() { return head_; }
	QueueMonitor*& qm() { return qm_; }

        int ack_count_;         /* number of TCP acks in the queue */
	int data_count_;        /* number of non-ack packets in the queue */
	int acks_to_send_;      /* number of acks to send in current scheduling
				   cycle */
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
	int interleave_;        /* whether to interleave transmission of TCP
				   acks and other data */
	int acksfirst_;         /* whether to deque TCP acks before any other type
				   of data */
	int ackfromfront_;      /* whether to try and drop ack from front of the
				 queue */
};


#endif
