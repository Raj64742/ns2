// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:53:32 haoboy>
//
// Copyright (c) 2000 by the University of Southern California
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
// Address parameters. Singleton class
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/addr-params.cc,v 1.2 2000/09/15 20:31:56 haoboy Exp $

#include <stdlib.h>
#include <tclcl.h>

#include "addr-params.h"

AddrParamsClass* AddrParamsClass::instance_ = NULL;

static AddrParamsClass addr_params_class;

void AddrParamsClass::bind()
{
	TclClass::bind();
	add_method("McastShift");
	add_method("McastMask");
	add_method("NodeShift");
	add_method("NodeMask");
	add_method("PortMask");
	add_method("PortShift");
	add_method("hlevel");
	add_method("nodebits");
}

int AddrParamsClass::method(int ac, const char*const* av)
{
	Tcl& tcl = Tcl::instance();
	int argc = ac - 2;
	const char*const* argv = av + 2;

	if (argc == 2) {
		if (strcmp(argv[1], "McastShift") == 0) {
			tcl.resultf("%d", McastShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			tcl.resultf("%d", McastMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			tcl.resultf("%d", PortShift_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			tcl.resultf("%d", PortMask_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			tcl.resultf("%d", hlevel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			tcl.resultf("%d", nodebits_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "McastShift") == 0) {
			McastShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "McastMask") == 0) {
			McastMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortShift") == 0) {
			PortShift_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "PortMask") == 0) {
			PortMask_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "hlevel") == 0) {
			hlevel_ = atoi(argv[2]);
			if (NodeMask_ != NULL)
				delete []NodeMask_;
			if (NodeShift_ != NULL)
				delete []NodeShift_;
			NodeMask_ = new int[hlevel_];
			NodeShift_ = new int[hlevel_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "nodebits") == 0) {
			nodebits_ = atoi(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeShift") == 0) {
			tcl.resultf("%d", node_shift(atoi(argv[2])-1));
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			tcl.resultf("%d", node_mask(atoi(argv[2])-1));
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "NodeShift") == 0) {
			NodeShift_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "NodeMask") == 0) {
			NodeMask_[atoi(argv[2])-1] = atoi(argv[3]);
			return (TCL_OK);
		}
	}
	return TclClass::method(ac, av);
}
