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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/errmodel.cc,v 1.10 1997/07/23 19:34:32 gnguyen Exp $ (UCB)";
#endif

#include "packet.h"
#include "ll.h"
#include "errmodel.h"

static class ErrorModelClass : public TclClass {
public:
	ErrorModelClass() : TclClass("ErrorModel") {}
	TclObject* create(int argc, const char*const* argv) {
		ErrorUnit eu = EU_PKT;
		if (argc >= 5)
			eu = STR2EU(argv[4]);
		return (new ErrorModel(eu));
	}
} class_errormodel;

static char* eu_names[] = { EU_NAMES };


ErrorModel::ErrorModel(ErrorUnit eu) : DropConnector(), eu_(eu), rate_(0)
{
	bind("rate_", &rate_);
	bind("off_ll_", &off_ll_);
}


int 
ErrorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "ranvar") == 0) {
			ranvar_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "unit") == 0) {
			eu_ = STR2EU(argv[2]);
			return (TCL_OK);
		}
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "ranvar") == 0) {
			tcl.resultf("%s", ranvar_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "unit") == 0) {
			tcl.resultf("%s", eu_names[eu_]);
			return (TCL_OK);
		}
	}
	return DropConnector::command(argc, argv);
}


void
ErrorModel::recv(Packet* p, Handler*)
{
	hdr_ll* llh = (hdr_ll*)p->access(off_ll_);
	if (corrupt(p)) {
		if (drop_) {
			drop_->recv(p);
			return;
		}
		llh->error() |= 1;
	}
	if (target_)
		target_->recv(p);
	// XXX if no target, assume packet is still used by other object
}


int
ErrorModel::corrupt(Packet* p)
{
	hdr_cmn *hdr;
	double rv = ranvar_->value();

	switch (eu_) {
	case EU_PKT:
		return (rv < rate_);
	case EU_BIT:
		hdr = (hdr_cmn*) p->access(off_cmn_);
		return (rv < (1 - pow((1-rate_), 8. * hdr->size())));
	default:
		break;
	}
	return 0;
}
