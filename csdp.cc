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
 */

#include "random.h"
#include "ll.h"
#include "mac.h"
#include "csdp.h"


static class CsdpClass : public TclClass {
public:
	CsdpClass() : TclClass("Queue/Csdp") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Csdp);
	}
} class_csdp;


Csdp::Csdp() : maxq_(4), numq_(0), qlen_(0), totalweight_(0), gqid_(0), bqid_(0), em_(0)
{
	bind("off_ll_", &off_ll_);
	bind("off_mac_", &off_mac_);
	q_ = new IdPacketQueue*[maxq_];
}


int
Csdp::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "errormodel") == 0) {
			em_ = (ErrorModel*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}


void
Csdp::enque(Packet* p)
{
	int id = ((hdr_mac*)p->access(off_mac_)) ->macDA();
	int i;

	if (qlen_ >= qlim_) {
		drop(p);
		return;
	}
	qlen_++;

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
	enque(p, q_[i]);
}


Packet*
Csdp::deque()
{
	if (qlen_ == 0)
		return 0;
	double r = Random::uniform(totalweight_);
	for (int i = 0;  i < numq_;  i++) {
		gqid_ = (gqid_ + 1) % numq_;
		if (q_[gqid_]->length() > 0) {
			qlen_--;
			return deque(q_[gqid_]);
		}
	}
	return 0;
}


void
Csdp::enque(Packet* p, IdPacketQueue* q)
{
	q->enque(p);
	if (((hdr_ll*)p->access(off_ll_)) ->error())
		q->loss()++;
	q->total()++;
}


Packet*
Csdp::deque(IdPacketQueue* q)
{
	Packet *p = q->deque();
	return p;
}


double
Csdp::weight(IdPacketQueue* q)
{
	double w = 1.0 - (double) q->loss() / (q->total() + 1);
	return q->length() ? w : 0;
}
