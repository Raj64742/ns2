/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/queue.cc,v 1.22 1999/02/19 23:03:17 yuriy Exp $ (LBL)";
#endif

#include "queue.h"
#include <stdio.h>

void PacketQueue::remove(Packet* target)
{
	for (Packet *pp= 0, *p= head_; p; pp= p, p= p->next_) {
		if (p == target) {
			if (!pp) deque();
			else {
				if (p == tail_) 
					tail_= pp;
				else
					pp->next_= p->next_;
				--len_;
			}
			return;
		}
	}
	fprintf(stderr, "PacketQueue:: remove() couldn't find target\n");
	abort();
}

/*
 * Remove packet pkt located after packet prev on the queue.  Either p or prev
 * could be NULL.  If prev is NULL then pkt must be the head of the queue.
 */
void PacketQueue::remove(Packet* pkt, Packet *prev) //XXX: screwy
{
	if (pkt) {
		if (head_ == pkt)
			PacketQueue::deque(); /* decrements len_ internally */
		else {
			prev->next_ = pkt->next_;
			if (tail_ == pkt)
				tail_ = prev;
			--len_;
		}
	}
	return;
}

void QueueHandler::handle(Event*)
{
	queue_.resume();
}

Queue::Queue() : Connector(), blocked_(0), unblock_on_resume_(1), qh_(*this), 
	pq_(0)			/* temporarily NULL */
{
	bind("limit_", &qlim_);
	bind_bool("blocked_", &blocked_);
	bind_bool("unblock_on_resume_", &unblock_on_resume_);
}

void Queue::recv(Packet* p, Handler*)
{
	enque(p);
	if (!blocked_) {
		/*
		 * We're not blocked.  Get a packet and send it on.
		 * We perform an extra check because the queue
		 * might drop the packet even if it was
		 * previously empty!  (e.g., RED can do this.)
		 */
		p = deque();
		if (p != 0) {
			blocked_ = 1;
			target_->recv(p, &qh_);
		}
	}
}

void Queue::resume()
{
	Packet* p = deque();
	if (p != 0) {
		target_->recv(p, &qh_);
	} else {
		if (unblock_on_resume_)
			blocked_ = 0;
		else
			blocked_ = 1;
	}
}

void Queue::reset()
{
	Packet* p;
	while ((p = deque()) != 0)
		drop(p);
}
