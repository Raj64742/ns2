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
 */

#include "packet.h"
#include "queue.h"
#include "queue-filter.h"

int
QueueFilter::compareFlows(hdr_ip *ip1, hdr_ip *ip2)
{
	return ((ip1->src_ == ip2->src_) && (ip1->dst == ip2->dst));
}

void
QueueFilter::filter(PacketQueue *queue, Packet *p) 
{
	/* first check for ack */
	if (p->type() != PT_ACK)
		return;

	Packet *q, *qq;
	hdr_tcp *tcph = (hdr_tcp*) p->access(off_tcp_);
	int &ack = tcph->seqno();

	hdr_ip *iph = (hdr_ip*) p->access(off_ip_);
	for (q = queue->head(), qq = q; q != 0; ) {
		if (compareFlows((hdr_ip*) q->access(off_ip_), iph)) {
			if (q->type() == PT_ACK) {
				hdr_tcp *th = (hdr_tcp*) q->access(off_tcp_);
				if (th->seqno() < ack) { // remove this ack
				      queue->purge(q, qq);
				      queue->ack_count_--;
				      Packet::free(q); // should really be drop
				      q = qq->next();
				      continue;
				}
			}
		}
		qq = q;
		q = q->next();
	}
}
	  
DropTailFilter::DropTailFilter()
{
	bind("off_ip_", &off_ip_);
        bind("off_tcp_", &off_tcp_);
	QueueFilter::off_ip(off_ip_);
	QueueFilter::off_tcp(off_tcp_);
}

void
DropTailFilter::recv(Packet* p, Handler* handler)
{
	/* First filter packet/queue, and then enque it. */
	filter(&q_, p);
	Queue::recv(p);
}

REDFilter::REDFilter()
{
	bind("off_ip_", &off_ip_);
        bind("off_tcp_", &off_tcp_);
	QueueFilter::off_ip(off_ip_);
	QueueFilter::off_tcp(off_tcp_);
}

void
REDFilter::recv(Packet* p, Handler* handler)
{
	/* First filter packet/queue, and then enque it. */
	filter(&q_, p);
	Queue::recv(p);
}
