/*
 * Copyright (c) 1990-1994 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/trace.cc,v 1.5 1997/02/03 16:59:10 mccanne Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "queue.h"

class Trace : public Connector {
 public:
	Trace(int type);
	~Trace();
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler*);
	void dump();
	inline char* buffer() { return (wrk_); }
 protected:
	int type_;
	nsaddr_t src_;
	nsaddr_t dst_;
	Tcl_Channel channel_;
	int callback_;
	char wrk_[256];
	void format(int tt, int s, int d, Packet* p);
};

/*
 * tcl command interface
 */
class HopTraceClass : public TclClass {
public:
	HopTraceClass() : TclClass("Trace/Hop") { }
	TclObject* create(int argc, const char*const* argv) {
		return (new Trace('h'));
	}
} hop_trace_class;

class EnqueTraceClass : public TclClass {
public:
	EnqueTraceClass() : TclClass("Trace/Enque") { }
	TclObject* create(int argc, const char*const* argv) {
		return (new Trace('+'));
	}
} enque_trace_class;

class DequeTraceClass : public TclClass {
public:
	DequeTraceClass() : TclClass("Trace/Deque") { }
	TclObject* create(int argc, const char*const* argv) {
		return (new Trace('-'));
	}
} deque_trace_class;

class DropTraceClass : public TclClass {
public:
	DropTraceClass() : TclClass("Trace/Drop") { }
	TclObject* create(int argc, const char*const* argv) {
		return (new Trace('d'));
	}
} drop_trace_class;

Trace::Trace(int type)
	: type_(type), channel_(0), callback_(0), src_(0), dst_(0)
{
	bind("src_", (int*)&src_);
	bind("dst_", (int*)&dst_);
	bind("callback_", &callback_);
}

Trace::~Trace()
{
}

/*
 * $trace detach
 * $trace flush
 * $trace attach $fileID
 */
int Trace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "detach") == 0) {
			channel_ = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flush") == 0) {
			Tcl_Flush(channel_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (channel_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}

char* pt_names[] = {
	PT_NAMES
};

void Trace::format(int tt, int s, int d, Packet* p)
{
	const char* name = pt_names[p->type_];
	if (name == 0)
		abort();

	char flags[5];
	flags[0] = (p->flags_ & PF_ECN) ? 'C' : '-';
	flags[1] = (p->flags_ & PF_PRI) ? 'P' : '-';
	flags[2] = (p->flags_ & PF_USR1) ? '1' : '-';
	flags[3] = (p->flags_ & PF_USR2) ? '2' : '-';
	flags[4] = 0;

	sprintf(wrk_, "%c %g %d %d %s %d %s %d %d %d %d %d",
		tt, Scheduler::instance().clock(), s, d,
		name, p->size_, flags, p->class_,
		p->src_, p->dst_,
		p->seqno_, p->uid_);
}

void Trace::dump()
{
	if (channel_ != 0) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		int n = strlen(wrk_);
		wrk_[n] = '\n';
		wrk_[n + 1] = 0;
		(void)Tcl_Write(channel_, wrk_, n + 1);
		wrk_[n] = 0;
	}
	if (callback_) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s handle { %s }", name(), wrk_);
	}
}

void Trace::recv(Packet* p, Handler* h)
{
	format(type_, src_, dst_, p);
	dump();
	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
