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
    "@(#) $Header: ";
#endif

#include "scheduler.h"
#include "tracegen.h"

class TraceGenClass : public TclClass {
public:
	TraceGenClass() : TclClass("TraceGen") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new TraceGen(*argv[4]));
		else
			return NULL;
	}
} tracegen_class;


TraceGen::TraceGen(int tt) : type_(tt), name_(""), channel_(0)
{
}

/*
 * $trace detach
 * $trace flush
 * $trace attach $fileID
 */
int TraceGen::command(int argc, const char*const* argv)
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
	return (TclObject::command(argc, argv));
}


void TraceGen::dump()
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
}

void TraceGen::trace(TracedVar* var)
{
	char tmp[256];
	sprintf(wrk_, "%c t%g n%s v%s",
		type_,
		Scheduler::instance().clock(),
		var->name(),
		var->value(tmp));
	dump();
}
