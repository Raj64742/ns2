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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/errmodel.cc,v 1.15 1997/09/08 22:15:37 gnguyen Exp $ (UCB)";
#endif

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

static char* eu_names[] = { EU_NAMES };


ErrorModel::ErrorModel() : Connector(), eu_(EU_PKT), rate_(0), ranvar_(0)
{
	bind("rate_", &rate_);
}

int ErrorModel::command(int argc, const char*const* argv)
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
	return Connector::command(argc, argv);
}

void ErrorModel::recv(Packet* p, Handler*)
{
	if (corrupt(p)) {
		// if drop_ target exists, drop the corrupted packet
		// else mark the error() flag of the packet and pass it on
		if (drop_) {
			drop_->recv(p);
			return;
		}
		((hdr_cmn*)p->access(off_cmn_))->error() |= 1;
	}
	if (target_)
		target_->recv(p);
	// XXX if no target, assume packet is still used by other object
}

int ErrorModel::corrupt(Packet* p)
{
	// if no random var is specified, assume uniform random variable
	double rv = ranvar_ ? ranvar_->value() : Random::uniform();
	hdr_cmn *hdr;

	switch (eu_) {
	case EU_PKT:
	case EU_TIME:
		return (rv < rate_);
	case EU_BIT:
		hdr = (hdr_cmn*) p->access(off_cmn_);
		return (rv < (1 - pow((1-rate_), 8. * hdr->size())));
	default:
		break;
	}
	return 0;
}


static class SelectErrorModelClass : public TclClass {
public:
	SelectErrorModelClass() : TclClass("SelectErrorModel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new SelectErrorModel);
	}
} class_selecterrormodel;


SelectErrorModel::SelectErrorModel() : ErrorModel()
{
}

int SelectErrorModel::command(int argc, const char*const* argv)
{
        int ac = 0;
        if (strcmp(argv[1], "drop-packet") == 0) {
		pkt_type_ = atoi(argv[2]);
		drop_cycle_ = atoi(argv[3]);
		drop_offset_ = atoi(argv[4]);
		return TCL_OK;
        }
        return ErrorModel::command(argc, argv);
}

int SelectErrorModel::corrupt(Packet* p)
{
	if (eu_ == EU_PKT) {
		hdr_cmn *ch = (hdr_cmn*) p->access(off_cmn_);
		//printf ("may drop packet type %d, uid %d\n", pkt_type_, ch->uid());
		if (ch->ptype() == pkt_type_ && ch->uid() % drop_cycle_ == drop_offset_) {
			printf ("drop packet type %d, uid %d\n", pkt_type_, ch->uid());
			return 1;
		}
	}
	return 0;
}
