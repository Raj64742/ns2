/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California at Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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

#include "queue-nonfifo.h"
#include "ip.h"
#include "tcp.h"

/* 
 * Deque TCP acks before any other type of packet.
 */
Packet* 
NonFifoPacketQueue::deque_acksfirst(int off_cmn) {
	Packet* p = head_;
	Packet* pp = NULL;
	int type;

	if (ack_count > 0) {
		while (p) {
			type = ((hdr_cmn*)p->access(off_cmn))->ptype_;
			if (type == PT_ACK)
				break;
			pp = p;
			p = p->next_;
		}
		if (!p) 
			fprintf(stderr, "In deque_acksfirst(): ack_count: %d but no acks in queue, length = %d\n", ack_count, length());
		PacketQueue::remove(p, pp);
	} else {
		p = PacketQueue::deque();
	}
	return p;
}

int
NonFifoPacketQueue::compareFlows(hdr_ip *ip1, hdr_ip *ip2)
{
	return ((ip1->src() == ip2->src()) && (ip1->dst() == ip2->dst()));
}

/*
 * Purge the queue of acks that are older (i.e., have a smaller sequence 
 * number) than the most recent ack. If replace_head is set, the most recent
 * ack (pointed to by pkt) takes the place of the oldest ack that is purged. 
 * Otherwise, it remains at the tail of the queue.
 */
void
NonFifoPacketQueue::filterAcks(Packet *pkt, int replace_head, int off_cmn, int off_tcp, int off_ip) 
{
	int done_replacement = 0;

	/* first check for ack */
	if (!(((hdr_cmn*)pkt->access(off_cmn))->ptype_ == PT_ACK))
		return;

	Packet *p, *pp, *new_p;
	hdr_tcp *tcph = (hdr_tcp*) pkt->access(off_tcp);
	int &ack = tcph->seqno();

	hdr_ip *iph = (hdr_ip*) pkt->access(off_ip);
	for (p = head(), pp = NULL; p != 0; ) {
		/* 
		 * check if the packet in the queue belongs to the same connection as
		 * the most recent ack
		 */
		if (compareFlows((hdr_ip*) p->access(off_ip), iph)) {
			/* check if it is an ack */
			if (((hdr_cmn*)p->access(off_cmn))->ptype_ == PT_ACK) {
				hdr_tcp *th = (hdr_tcp*) p->access(off_tcp);
				/*
				 * check if the ack packet in the queue is older than
				 * the most recent ack
				 */
				if ((th->seqno() < ack) ||
				    (replace_head && th->seqno() == ack)) { 
					/* 
					 * if we haven't yet replaced the ack closest
					 * to the head with the most recent ack, do
					 * so now
					 */
					if (replace_head && !done_replacement && 
					    pkt != p) {
						PacketQueue::remove(pkt);
						pkt->next_ = p;
						if (pp)
							pp->next_ = pkt;
						pp = pkt;
						done_replacement = 1;
						continue;
					}
					else if (done_replacement || pkt != p) {
						new_p = p->next_;
						PacketQueue::remove(p, pp);
						ack_count--;
						if (ack_count == 0)
							fprintf(stderr,"Error: Removed all acks, p=%x head=%x\n", p, head());
						Packet::free(p); // should really be dropping the packet
						p = new_p;
						continue;
					}
				}
			}
		}
		pp = p;
		p = p->next_;
	}
}
	  
void 
NonFifoPacketQueue::enque(Packet *pkt, int off_cmn, int off_tcp, int off_ip)
{
	PacketQueue::enque(pkt);
	if (((hdr_cmn*)pkt->access(off_cmn))->ptype_ == PT_ACK)
		ack_count++;
	else
		data_count++;
	if ((((hdr_cmn*)pkt->access(off_cmn))->ptype_ == PT_ACK) && filteracks_) {
		filterAcks(pkt, replace_head_, off_cmn, off_tcp, off_ip);
	}
}

Packet *
NonFifoPacketQueue::deque(int off_cmn)
{
	Packet *pkt;

	if (acksfirst_)
		pkt = deque_acksfirst(off_cmn);
	else
		pkt = PacketQueue::deque();
	
	if (pkt) {
		if (((hdr_cmn*)pkt->access(off_cmn))->ptype_ == PT_ACK)
			ack_count--;
		else
			data_count--;
	}
	return pkt;
}

void 
NonFifoPacketQueue::remove(Packet *pkt, int off_cmn)
{
	PacketQueue::remove(pkt);
	if (pkt) {
		if (((hdr_cmn*)pkt->access(off_cmn))->ptype_ == PT_ACK)
			ack_count--;
		else
			data_count--;
	}
}
