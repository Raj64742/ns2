/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * net-interface.cc
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/net-interface.cc,v 1.8 1999/06/21 18:13:58 tomh Exp $ (USC/ISI)";
#endif

#include "net-interface.h"

int NetworkInterface::command(int argc, const char*const* argv) {
	if (argc > 1) {
		if (strcmp(argv[1], "label") == 0) {
			if (argc == 2) {
				Tcl::instance().resultf("%d", intf_label_);
				return TCL_OK;
			}
			if (argc == 3) {
				intf_label_ = atoi(argv[2]);
				return TCL_OK;
			}
		}
	}
      	return (Connector::command(argc, argv));
}

void NetworkInterface::recv(Packet* p, Handler* h) {
	hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
#ifdef LEO_DEBUG
	printf("Marking to %d\n", intf_label_);
#endif
	ch->iface() = intf_label_;
	send(p, h);
}

static class NetworkInterfaceClass : public TclClass {
public:
	NetworkInterfaceClass() : TclClass("NetworkInterface") {}
	TclObject* create(int, const char*const*) {
		return (new NetworkInterface);
	}
} class_networkinterface;

