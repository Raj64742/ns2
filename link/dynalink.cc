/*
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
 */

// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
//      Author:         Kannan Varadhan <kannan@isi.edu>
//      Version Date:   Mon Jun 30 15:51:33 PDT 1997

#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/link/dynalink.cc,v 1.8 1997/10/22 21:00:27 kannan Exp $ (USC/ISI)";
#endif

#include "connector.h"

// #include "packet.h"
// #include "queue.h"

class DynamicLink : public Connector {
public:
	DynamicLink() : down_(0), status_(1) { bind("status_", &status_); }
protected:
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
	NsObject* down_;
	int status_;
};

static class DynamicLinkClass : public TclClass {
public:
	DynamicLinkClass() : TclClass("DynamicLink") {}
	TclObject* create(int, const char*const*) {
		return (new DynamicLink);
	}
} class_dynamic_link;


int DynamicLink::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "status?") == 0) {
			Tcl::instance().result(status_ ? "up" : "down");
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}

void DynamicLink::recv(Packet* p, Handler* h)
{
	if (status_)
		target_->recv(p, h);
	else
		drop(p);
}
