/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 1997 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// ADU and ADU processor
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/ns-process.cc,v 1.4 2001/11/30 02:45:19 buchheim Exp $

#include "ns-process.h"

static class ProcessClass : public TclClass {
 public:
	ProcessClass() : TclClass("Process") {}
	TclObject* create(int, const char*const*) {
		return (new Process);
	}
} class_process;

void Process::process_data(int, AppData*)
{
	abort();
}

AppData* Process::get_data(int&, AppData*)
{
	abort();
	/* NOTREACHED */
	return NULL; // Make msvc happy 
}

int Process::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "target") == 0) {
		if (argc == 2) {
			Process *p = target();
			tcl.resultf("%s", p ? p->name() : "");
			return TCL_OK;
		} else if (argc == 3) { 
			Process *p = (Process *)TclObject::lookup(argv[2]);
			if (p == NULL) {
				fprintf(stderr, "Non-existent media app %s\n",
					argv[2]);
				abort();
			}
			target() = p;
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}
