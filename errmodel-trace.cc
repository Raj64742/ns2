/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
#include "trace.h"
#include "errmodel-trace.h"


static class ErrorModelTraceClass : public TclClass {
public:
	ErrorModelTraceClass() : TclClass("ErrorModel/Trace") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new ErrorModelTrace);
	}
} class_errormodel_trace;


ErrorModelTrace::ErrorModelTrace() : ErrorModel(), channel_(0), start_(0), len_(0), hTmo_(*this)
{
}


int
ErrorModelTrace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (channel_ == 0) {
				tcl.resultf("trace: can't attach %s for reading", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
	}
	return ErrorModel::command(argc, argv);
}


int
ErrorModelTrace::corrupt(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	if (errorLen_ <= 0)
		return 0;
	if (unit_ == EuPacket)
		errorLen_ -= 1;
	else if (unit_ == EuBit) {
		hdr_cmn *hdr = (hdr_cmn*) p->access(off_cmn_);
		errorLen_ -= 8. * hdr->size();
	}
	else if (time_ + errorLen_ < s.clock())
		return 0;
	return 1;
}


void
ErrorModelTrace::start()
{
	Scheduler& s = Scheduler::instance();
	Tcl_DString line;
	start_ = s.clock();
	if (channel_ && (Tcl_Gets(channel_, &line) >= 0) &&
	    sscanf(Tcl_DStringValue(&line), "%f %f", &ts_, &len_) == 2) {
		s.schedule(&hTmo_, &intr_, start_ + ts_ - s.clock());
	}
}


void
ErrorModelTrace::timeout()
{
	Scheduler& s = Scheduler::instance();
	Tcl_DString line;
	time_ = start_ + ts_;
	errorLen_ = len_;
	if (sscanf(Tcl_DStringValue(&line), "%f %f", &ts_, &len_) == 2) {
		s.schedule(&hTmo_, &intr_, start_ + ts_ - s.clock());
	}
}


void
EmTraceHandler::handle(Event*)
{
	em_.timeout();
}
