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
 * 	Group at Lawrence Berkeley National Laboratory and the Daedalus 
 *      Research Group and the University of California, Berkeley.
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

#ifndef ns_queue_nonfifo_h
#define ns_queue_nonfifo_h

#include "queue.h"

class NonFifoPacketQueue : public PacketQueue {
  public:
	NonFifoPacketQueue() : ack_count(0), data_count(0), 
		acks_to_send(0) {}
	/* interleave between TCP acks and others while dequeuing */
	Packet* deque_interleave(int off_cmn);
	/* deque TCP acks before any other type of packet */
	Packet* deque_acksfirst(int off_cmn);
	/* 
	 * If a packet needs to be removed because the queue is full, 
	 * pick the TCP ack closest to the head. Otherwise, drop the 
	 * specified pkt (target).
	 */
	Packet* remove_ackfromfront(Packet* target, int off_cmn);
	/* 
	 * Remove a specific packet given pointers to it and its predecessor
	 * in the queue. p and/or pp may be NULL.
	 */
	void remove(Packet* p, Packet* pp);
	void purge(Packet *, Packet *);	/* remove pkt 1 (located after 2) */

	inline Packet* head() { return head_; }
        int ack_count;        /* number of TCP acks in the queue */
	int data_count;       /* number of non-ack packets in the queue */
	int acks_to_send;     /* number of acks to send in current schedule */
};

class QueueHelper {
  public:
	QueueHelper() {}
  protected:
	/* These control (helper) variables are bound in derived objects */
	int interleave_;	/* interleave TCP acks and other data */
	int acksfirst_;         /* deque TCP acks before any other data */
	int ackfromfront_;      /* try and drop ack from front of the queue */
	virtual Packet* deque_helper(NonFifoPacketQueue *, int);
	virtual void enque_helper(NonFifoPacketQueue *, Packet *, int);
	virtual void remove_helper(NonFifoPacketQueue *, Packet *, int);
};

#endif
