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


Csdp::Csdp() : qsize_(16), qlen_(0)
{
	q_ = new Packet*[qsize_];
}


void
Csdp::recv(Packet* p, Handler*)
{
	enque(p);
	if (!blocked_) {
		blocked_ = 1;
		p = deque();
		if (p != 0)
			target_->recv(p, &qh_);
		else
			blocked_ = 0;
	}
}


void
Csdp::enque(Packet* p)
{
	if (++qlen_ >= qsize_) {
		qsize_ *= 2;
		realloc(q_, qsize_);
	}
	for (int i=0;  i < qsize_;  i++) {
		if (q_[i] == 0) {
			q_[i] = p;
			break;
		}
	}
}


Packet*
Csdp::deque()
{
	if (qlen_ < 1)
		return 0;
	int bestindex = 0;
	double bestscore = score(q_[0]);
	for (int i=1;  i < qlen_;  i++) {
		if (score(q_[i]) > bestscore)
			bestindex = i;
	}
	Packet* p = q_[bestindex];
	q_[bestindex] = NULL;
	qlen_--;
	((BlockingLL*)p->source()) ->resume();
	return p;
}


double
Csdp::score(Packet* p)
{
	BlockingLL* ll = (BlockingLL*) p->source();
	if (ll->em())
		return (- ll->em()->rate());
	else
		return 0;
}
