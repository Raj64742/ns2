/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#include "random.h"
#include "errmodel.h"
#include "ll.h"
#include "mac.h"
#include "cdls.h"


static class CdlsClass : public TclClass {
public:
	CdlsClass() : TclClass("Queue/Cdls") {}
	TclObject* create(int, const char*const*) {
		return (new Cdls);
	}
} class_cdls;


Cdls::Cdls() : qlen_(0), total_(0), maxq_(4), numq_(0)
{
	bind("off_ll_", &off_ll_);
	bind("off_mac_", &off_mac_);
	q_ = new IdPacketQueue*[maxq_];
}


int
Cdls::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "error") == 0) {
			IdPacketQueue* q = getQueue(atoi(argv[2]));
			q->em() = (ErrorModel*)TclObject::lookup(argv[3]);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}


void
Cdls::enque(Packet* p)
{
	if (qlen_ >= qlim_) {
		drop(p);
		return;
	}
	qlen_++;
	getQueue(((hdr_mac*)p->access(off_mac_))->macDA())->enque(p);
}


Packet*
Cdls::deque()
{
	if (qlen_ == 0)
		return 0;
	IdPacketQueue* q = selectQueue();
	Packet* p = q->deque();
	if (p != 0) {
		qlen_--;
		total_++;
		if (q->em() && q->em()->corrupt(p))
			((hdr_ll*)p->access(off_ll_))->error() = 1;
	}
	return p;
}


/*
 * getQueue:  find a queue with identifier id, create new one if necessary
 */
IdPacketQueue*
Cdls::getQueue(int id)
{
	int i;
	for (i = 0;  i < numq_;  i++) {
		if (q_[i]->id() == id)
			return q_[i];
	}
	if (i >= numq_) {
		if (++numq_ == maxq_) {
			maxq_ *= 2;
			q_ = (IdPacketQueue**)
				realloc(q_, maxq_ * sizeof(IdPacketQueue*));
		}
		q_[i] = new IdPacketQueue;
	}
	q_[i]->id() = id;
	return q_[i];
}


IdPacketQueue*
Cdls::selectQueue()
{
	// Lottery Scheduling: randomly pick a queue based on its weight
	int i;
	double r, w = 0;
	for (i = 0;  i < numq_;  i++)
		w += weight(q_[i]);
	r = Random::uniform(w);
	for (i = 0;  i < numq_;  i++) {
		w = weight(q_[i]);
		r -= w;
		if (r <= 0)
			return q_[i];
	}
	return 0;
}


double
Cdls::weight(IdPacketQueue* q)
{
	double w = 1;
	if (q->length() == 0)
		return 0;
	if (q->em()) {
		// w = 1.0 / (q->em()->rate() + 1e-9);
		w = 1.0 - q->em()->rate();
	}
	else {
		// w = (double) total_ / (numq_ * (q->total() + 1));
	}
	return w;
}


Packet*
IdPacketQueue::deque()
{
	total_++;
	return PacketQueue::deque();
}
