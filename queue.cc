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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/queue.cc,v 1.8 1997/03/28 01:24:31 mccanne Exp $ (LBL)";
#endif

#include "queue.h"

void PacketQueue::remove(Packet* target)
{
	IPHeader *ip = IPHeader::access(target->bits());
	for (Packet** p = &head_; *p != 0; p = &(*p)->next_) {
		Packet* pkt = *p;
		if (pkt == target) {
			*p = pkt->next_;
			if (tail_ == &pkt->next_)
				tail_ = p;
			--len_;
			bcount_ -= ip->size();
			return;
		}
	}
	abort();
}

void QueueHandler::handle(Event*)
{
	queue_.resume();
}

Queue::Queue() : drop_(0), blocked_(0), qh_(*this)
{
	Tcl& tcl = Tcl::instance();
	bind("limit_", &qlim_);
}

int Queue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "drop-target") == 0) {
			if (drop_ != 0)
				tcl.resultf("%s", drop_->name());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "drop-target") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			drop_ = p;
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
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
	if (p != 0)
		target_->recv(p, &qh_);
	else
		blocked_ = 0;
}

void Queue::reset()
{
	while (Packet* p = deque())
		drop(p);
}
