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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/queue.cc,v 1.11.2.6 1997/04/29 06:30:21 padmanab Exp $ (LBL)";
#endif

#include "queue.h"

void
PacketQueue::purge(Packet *pkt, Packet *prev) 
{
	if (head_ == pkt)
		deque();	// decrements len_ internally
	else {
		prev->next_ = pkt->next_;
		if (tail_ == &pkt->next_)
			tail_ = &prev->next_;
		--len_;
	}
	if (qm_)
		qm_->out(pkt);
	return;
}


void PacketQueue::remove(Packet* target)
{
	for (Packet** p = &head_; *p != 0; p = &(*p)->next_) {
		Packet* pkt = *p;
		if (pkt == target) {
			*p = pkt->next_;
			if (tail_ == &pkt->next_)
				tail_ = p;
			--len_;
			if (qm_)
				qm_->out(pkt);
			return;
		}
	}
	abort();
}

void PacketQueue::remove(Packet* target, int off_cmn)
{
	int type = ((hdr_cmn*)target->access(off_cmn))->ptype_;
	if (type == PT_ACK) 
		ack_count_--;
	else
		data_count_--;
	remove(target);
}

/* 
 * If a packet needs to be removed because the queue is full, pick the TCP ack
 * closest to the head. Otherwise, drop the specified pkt (target).
 */
Packet* PacketQueue::remove_ackfromfront(Packet* target, int off_cmn)
{
	Packet* p = head_;
	Packet* pp = NULL;
	int type;

	if (ack_count_ > 0) {
		while (p) {
			type = ((hdr_cmn*)p->access(off_cmn))->ptype_;
			if (type == PT_ACK)
				break;
			pp = p;
			p = p->next_;
		}
		if (!p) 
			fprintf(stderr, "In remove_ackfromfront(): ack_count_: %d but no acks in queue\n", ack_count_);
		remove(p, pp);
		ack_count_--;
		return p;
	}
	else {
		remove(target);
		if (target)
			data_count_--;
		return target;
	}
}


void PacketQueue::remove(Packet* p, Packet* pp) 
{
	if (p) {
		if (pp) 
			pp->next_ = p->next_;
		else 
			head_ = p->next_;
		if (tail_ == &p->next_) {
			if (pp)
				tail_ = &pp->next_;
			else
				tail_ = &head_;
		}
		--len_;
		if (qm_)
			qm_->out(p);
	}
	return;
}

/* deque TCP acks before any other type of packet */
Packet* PacketQueue::deque_acksfirst(int off_cmn) {
	Packet* p = head_;
	Packet* pp = NULL;
	int type;

	if (ack_count_ > 0) {
		while (p) {
			type = ((hdr_cmn*)p->access(off_cmn))->ptype_;
			if (type == PT_ACK)
				break;
			pp = p;
			p = p->next_;
		}
		if (!p) 
			fprintf(stderr, "In deque_acksfirst(): ack_count_: %d but no acks in queue\n", ack_count_);
		remove(p, pp);
		ack_count_--;
	}
	else {
		p = deque();
		if (p)
			data_count_--;
	}
	return p;
}

/* interleave between TCP acks and others while dequeuing */
Packet* PacketQueue::deque_interleave(int off_cmn) {
	Packet *ack_p, *ack_pp, *data_p, *data_pp;
	Packet* p = head_;
	Packet* pp = NULL;
	int type;
	
	/* 
	 * first find the the leading data & ack pkts and the packets
	 * just ahead of them in the queue
	 */
	ack_p = NULL;
	data_p = NULL;
	while (p) {
		type = ((hdr_cmn*)p->access(off_cmn))->ptype_;
		if (!ack_p && type == PT_ACK) {
			ack_p = p;
			ack_pp = pp;
		}
		else if (!data_p && type != PT_ACK) {
			data_p = p;
			data_pp = pp;
		}
		if (ack_p && data_p)
			break;
		pp = p;
		p = p->next_;
	}
	if ((acks_to_send_ || !data_p) && ack_p) {
		p = ack_p;
		pp = ack_pp;
		if (ack_p) 
			ack_count_--;
		if (ack_p && acks_to_send_)
			acks_to_send_--;
	}
	else {
		p = data_p;
		pp = data_pp;
		if (data_p)
			data_count_--;
		if (data_count_) {
			acks_to_send_ = ack_count_/data_count_;
			if (ack_count_%data_count_)
				acks_to_send_++;
		}
		else if (ack_count_)
			acks_to_send_ = 1;
/*		fprintf(stderr, "ack_count_: %d  data_count_: %d  acks_to_send_: %d\n", ack_count_, data_count_, acks_to_send_);*/
	}
	remove(p, pp); 
	return (p);
}


void QueueHandler::handle(Event*)
{
	queue_.resume();
}

Queue::Queue() : drop_(0), blocked_(0), qh_(*this), interleave_(0), acksfirst_(0), ackfromfront_(0)
{
	Tcl& tcl = Tcl::instance();
	bind("limit_", &qlim_);
	bind_bool("interleave_", &interleave_);
	bind_bool("acksfirst_", &acksfirst_);
	bind_bool("ackfromfront_", &ackfromfront_);
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
		else if (strcmp(argv[1], "set-monitor") == 0) {
			if (drop_ != 0)
				tcl.resultf("%s", q_.qm()->name());
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
		if (strcmp(argv[1], "set-monitor") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			q_.qm() = (QueueMonitor*) p;
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


void Queue::drop(Packet* p)
{
	if (drop_)
		drop_->recv(p);
	else
		Packet::free(p);
}
