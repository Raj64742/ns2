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
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/net-interface.cc,v 1.5 1997/08/10 08:47:08 ahelmy Exp $ (USC/ISI)";
#endif

#include "connector.h"
#include "packet.h"
#include "trace.h"

class NetworkInterface : public Connector {
public:
	NetworkInterface() : intf_label_(-1) {
		bind("off_cmn_", &off_cmn_);
		bind("intf_label_", &intf_label_);
	}
	NetworkInterface(int32_t lbl) : intf_label_(lbl) {
		bind("off_cmn_", &off_cmn_);
		bind("intf_label_", &intf_label_);
	}
	int command(int argc, const char*const* argv);
	void recv(Packet* pkt, Handler* h) {
        	((hdr_cmn*)pkt->access(off_cmn_))->iface() = intf_label_;
		send(pkt, h);
	}
protected:
	int32_t intf_label_;
};

static class NetworkInterfaceClass : public TclClass {
public:
	NetworkInterfaceClass() : TclClass("networkinterface") {}
	TclObject* create(int, const char*const*) {
                return (new NetworkInterface);
	}
} class_networkinterface;

int NetworkInterface::command(int argc, const char*const* argv)
{
        if (argc == 3) {
                if (strcmp(argv[1], "label") == 0) {
			intf_label_ = atoi(argv[2]);
			return (TCL_OK);
		}
        }
        return (Connector::command(argc, argv));
}
