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
#include "csdp-ls.h"


static class CsdpLsClass : public TclClass {
public:
	CsdpLsClass() : TclClass("Queue/Csdp/Ls") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CsdpLs);
	}
} class_csdp_ls;


CsdpLs::CsdpLs() : Csdp()
{
}


Packet*
CsdpLs::deque()
{
	if (qlen_ == 0)
		return 0;
	double r = Random::uniform(totalweight_);
	for (int i = 0;  i < numq_;  i++) {
		r -= weight(q_[i]);
		if (r <= 0) {
			qlen_--;
			return deque(q_[i]);
		}
	}
	return 0;
}


void
CsdpLs::enque(Packet* p, IdPacketQueue* q)
{
	double oldweight = weight(q);
	q->enque(p);
	if (((hdr_ll*)p->access(off_ll_)) ->error())
		q->loss()++;
	q->total()++;
	totalweight_ += weight(q) - oldweight;
}


Packet*
CsdpLs::deque(IdPacketQueue* q)
{
	double oldweight = weight(q);
	Packet *p = q->deque();
	totalweight_ += weight(q) - oldweight;
	return p;
}


double
CsdpLs::weight(IdPacketQueue* q)
{
	double w = 1.0 - (double) q->loss() / (q->total() + 1);
	return q->length() ? w : 0;
}
