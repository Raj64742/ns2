/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *
 * miscellaneous "ns" commands
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/misc.cc,v 1.8 1999/01/26 18:30:43 haoboy Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <ctype.h>
#include "scheduler.h"
#include "random.h"

#if defined(HAVE_STRTOQ) || defined(HAVE_STRTOLL)
class Add64Command : public TclCommand {
public: 
	Add64Command() : TclCommand("ns-add64") {}
	virtual int command(int argc, const char*const* argv);
};

int Add64Command::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		char res[22]; /* A 64 bit int at most 20 digits */
#if defined(HAVE_STRTOQ)
		quad_t d1 = strtoq(argv[1], NULL, 0);
		quad_t d2 = strtoq(argv[2], NULL, 0);
		sprintf(res, "%qd", d1+d2);
#elif defined(HAVE_STRTOLL)
		long long d1 = strtoll(argv[1], NULL, 0);
		long long d2 = strtoll(argv[2], NULL, 0);
		sprintf(res, "%lld", d1+d2);
#endif
		tcl.resultf("%s", res);
		return (TCL_OK);
	}
	tcl.add_error("ns-add64 requires two arguments.");
	return (TCL_ERROR);
}
#endif

class RandomCommand : public TclCommand {
public:
	RandomCommand() : TclCommand("ns-random") { }
	virtual int command(int argc, const char*const* argv);
};

/*
 * ns-random
 * ns-random $seed 
 */
int RandomCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 1) {
		sprintf(tcl.buffer(), "%u", Random::random());
		tcl.result(tcl.buffer());
	} else if (argc == 2) {
		int seed = atoi(argv[1]);
		if (seed == 0)
			seed = Random::seed_heuristically();
		else
			Random::seed(seed);
		tcl.resultf("%d", seed);
	}
	return (TCL_OK);
}

extern "C" char version_string[];

class VersionCommand : public TclCommand {
public:
	VersionCommand() : TclCommand("ns-version") { }
	virtual int command(int, const char*const*) {
		Tcl::instance().result(version_string);
		return (TCL_OK);
	}
};

class TimeAtofCommand : public TclCommand {
public:
	TimeAtofCommand() : TclCommand("time_atof") { }
	virtual int command(int argc, const char*const* argv) {
		if (argc != 2)
			return (TCL_ERROR);
		char* s = (char*) argv[1];
		char wrk[32];
		char* cp = wrk;
		while (isdigit(*s) || *s == 'e' ||
		       *s == '+' || *s == '-' || *s == '.')
			*cp++ = *s++;
		*cp = 0;
		double v = atof(wrk);
		switch (*s) {
		case 'm':
			v *= 1e-3;
			break;
		case 'u':
			v *= 1e-6;
			break;
		case 'n':
			v *= 1e-9;
			break;
		case 'p':
			v *= 1e-12;
			break;
		}
		Tcl::instance().resultf("%g", v);
		return (TCL_OK);
	}
};

void init_misc(void)
{
	(void)new VersionCommand;
	(void)new RandomCommand;
	(void)new TimeAtofCommand;
#if defined(HAVE_STRTOQ) || defined(HAVE_STRTOLL)
	(void)new Add64Command;
#endif
}
