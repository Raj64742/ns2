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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/net-interface.cc,v 1.6.2.2 1998/07/28 20:16:09 yuriy Exp $ (USC/ISI)";
#endif

#include "connector.h"
#include "packet.h"

class NetworkInterface : public Connector {
public:
	NetworkInterface() : intf_label_(UNKN_IFACE.value()) { /*NOTHING*/ }
	int command(int argc, const char*const* argv) {
		if (argc == 2) {
			if (strcmp(argv[1], "label") == 0) {
				Tcl::instance().resultf("%d", intf_label_);
				return TCL_OK;
			}
		}
		if (argc == 3) {
			if (strcmp(argv[1], "label") == 0) {
				intf_label_ = atoi(argv[2]);
				return TCL_OK;
			}
		}
        	return (Connector::command(argc, argv));
        }
	void recv(Packet* p, Handler* h) {
		hdr_cmn* ch = (hdr_cmn*) p->access(off_cmn_);
		ch->iface() = intf_label_;
		send(p, h);
	}
protected:
	int32_t intf_label_;
};

static class NetworkInterfaceClass : public TclClass {
public:
	NetworkInterfaceClass() : TclClass("networkInterface") {}
	TclObject* create(int, const char*const*) {
		return (new NetworkInterface);
	}
} class_networkinterface;
