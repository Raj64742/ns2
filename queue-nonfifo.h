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
	/* deque TCP acks before any other type of packet */
	Packet* deque_acksfirst(int off_cmn);
	/* determine whether two packets belong to the same connection */
	int compareFlows(hdr_ip *ip1, hdr_ip *ip2);
	/*
	 * When a new ack is enqued, purge older acks (as determined by the 
	 * sequence number of the ack field) from the queue. The enqued ack
	 * remains at the tail of the queue, unless replace_head is true, in 
	 * which case the new ack takes the place of the old ack closest to
	 * the head of the queue.
	 */
	void filterAcks(Packet *pkt, int replace_head, int off_cmn, int off_tcp, int off_ip); 
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

	/* These indicator variables are bound in derived objects */
	int acksfirst_;         /* deque TCP acks before any other data */
	int filteracks_;        /* purge queue of old acks when new ack is enqued */
	int replace_head_;      /* whether new ack should take the place of old ack
				   closest to the head */
	virtual Packet* deque(int);
	virtual void enque(Packet *, int, int, int);
	virtual void remove(Packet *, int);
};

#endif
