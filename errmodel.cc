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
#include "packet.h"
#include "ll.h"
#include "errmodel.h"


static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new ErrorModel);
	}
} class_errormodel;


ErrorModel::ErrorModel(ErrorUnit eu) : unit_(eu), rate_(0), time_(0), loss_(0), good_(0), errorLen_(0)
{
	bind("rate_", &rate_);
	bind("time_", &time_);
	bind("errorLen_", &errorLen_);
	bind("off_ll_", &off_ll_);
}


int 
ErrorModel::command(int argc, const char*const* argv)
{
	int ac = 0;
	if (!strcmp(argv[ac+1], "uniform")) {
		rate_ = atof(argv[ac+2]);
		return (TCL_OK);
	}
	return Connector::command(argc, argv);
}


void
ErrorModel::recv(Packet* p, Handler*)
{
	if (corrupt(p)) {
		loss_++;
		((hdr_ll*)p->access(off_ll_)) ->error() |= 1;
	}
	good_++;
	if (target_)
		target_->recv(p);
	// XXX assume packet is still used by other object
}


int
ErrorModel::corrupt(Packet* p)
{
	double u = Random::uniform();
	if (unit_ == EU_PKT)
		return (u < rate_);
	else if (unit_ == EU_BIT) {
		hdr_cmn *hdr = (hdr_cmn*) p->access(off_cmn_);
		double per = 1 - pow((1-rate_), 8. * hdr->size());
		return (u < per);
	}
	return 0;
}
