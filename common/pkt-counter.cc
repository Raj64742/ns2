/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * pkt-counter.cc
 * Copyright (C) 1997 by USC/ISI
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/pkt-counter.cc,v 1.1 1998/11/14 04:23:22 polly Exp $ (USC/ISI)";
#endif

#include "connector.h"

class PktCounter : public Connector {
public:
	PktCounter() : count_(0) {
	}
	int command(int argc, const char*const* argv);
	void recv(Packet* pkt, Handler* h) {
		count_++;
		send(pkt, h);
	}
protected:
	int count_;
};

static class PktCounterClass : public TclClass {
public:
	PktCounterClass() : TclClass("PktCounter") {}
	TclObject* create(int, const char*const*) {
		return (new PktCounter);
	}
} class_pktcounter;

int PktCounter::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			count_ = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "value") == 0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d", count_);
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}
