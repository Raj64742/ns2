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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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

#include "random.h"
#include "ll-block.h"
#include "csdp.h"


static class CsdpClass : public TclClass {
public:
	CsdpClass() : TclClass("Queue/CSDP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Csdp);
	}
} class_csdp;


Csdp::Csdp() : maxq_(4), numq_(0)
{
	q_ = new IdPacketQueue*[maxq_];
}


void
Csdp::enque(Packet* p)
{
	int id = (int) p->target();
	int i;

	for (i = 0;  i < numq_;  i++) {
		if (q_[i]->id() == id)
			break;
	}
	if (i == numq_) {
		if (++numq_ == maxq_) {
			maxq_ *= 2;
			q_ = (IdPacketQueue**)
				realloc(q_, maxq_ * sizeof(IdPacketQueue*));
		}
		q_[i] = new IdPacketQueue;
		q_[i]->id() = id;
	}

	q_[i]->enque(p);
	updateState(q_[i], p);
}


Packet*
Csdp::deque()
{
	IdPacketQueue* q;
	double r = Random::uniform(totalweight_);
	for (int i = 0;  i < numq_;  i++) {
		q = q_[i];
		r -= 1.0 - q->loss() / q->total();
		if (r <= 0) {
			totalweight_ -= weight(q);
			return q->deque();
		}
	}
	return 0;
}


double
Csdp::weight(IdPacketQueue* q)
{
	double w = 1.0 - q->loss() / (q->total() + 1);
	return w *= q->length();
}


void
Csdp::updateState(IdPacketQueue* q, Packet* p)
{
	double oldweight = weight(q);
	if (p->error())
		q->loss()++;
	q->total()++;
	totalweight_ += weight(q) - oldweight;
}
