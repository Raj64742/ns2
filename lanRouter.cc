/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * lanRouter.cc
 * Copyright (C) 1998 by USC/ISI
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /usr/src/mash/repository/vint/ns-2/lanRouter.cc";
#endif

#include <tcl.h>
#include "lanRouter.h"
#include "address.h"
#include "ip.h"

static class LanRouterClass : public TclClass {
public:
	LanRouterClass() : TclClass("LanRouter") {}
	TclObject* create(int, const char*const*) {
		return (new LanRouter());
	}
} class_mac;

int LanRouter::next_hop(Packet *p) {
 	if (switch_ && switch_->classify(p)==1) {
		return -1;
 	}
	if (!routelogic_) return -1;

	hdr_ip* iph= hdr_ip::access(p);
	char* adst= Address::instance().print_nodeaddr(iph->daddr());
	int next_hopIP;

	if (enableHrouting_) {
		char* bdst;

		routelogic_->lookup_hier(lanaddr_, adst, next_hopIP);
		// hacking: get rid of the last "."
		bdst = Address::instance().print_nodeaddr(next_hopIP);
// 		bdst[strlen(bdst)-1] = '\0';
		Tcl &tcl = Tcl::instance();
		tcl.evalf("[Simulator instance] get-node-id-by-addr %s", bdst);
		sscanf(tcl.result(), "%d", &next_hopIP);
		delete [] bdst;
	} else {
		routelogic_->lookup_flat(lanaddr_, adst, next_hopIP);
	}
	delete [] adst;

	return next_hopIP;
}
int LanRouter::command(int argc, const char*const* argv)
{
	// Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		// cmd lanaddr <addr>
		if (strcmp(argv[1], "lanaddr") == 0) {
			strcpy(lanaddr_, argv[2]);
			return (TCL_OK);
		}
		// cmd routing hier|flat
		if (strcmp(argv[1], "routing") == 0) {
			if (strcmp(argv[2], "hier")==0)
				enableHrouting_= true;
			else if (strcmp(argv[2], "flat")==0)
				enableHrouting_= false;
			else return (TCL_ERROR);
			return (TCL_OK);
		}
		// cmd switch <switch>
		if (strcmp(argv[1], "switch") == 0) {
			switch_ = (Classifier*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		// cmd routelogic <routelogic>
		if (strcmp(argv[1], "routelogic") == 0) {
			routelogic_ = (RouteLogic*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return NsObject::command(argc, argv);
}






