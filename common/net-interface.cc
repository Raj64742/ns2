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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/net-interface.cc,v 1.2 1997/07/23 00:57:03 kfall Exp $ (USC/ISI)";
#endif

#include "connector.h"
#include "packet.h"
#include "ip.h"

class NetworkInterface : public Connector {
public:
	interfaceLabel label;
	NetworkInterface() : label(-1) { bind("off_ip_", &off_ip_); }
	NetworkInterface(interfaceLabel lbl) : label(lbl) { bind("off_ip_", &off_ip_); }

	~NetworkInterface() {}

	command(int argc, const char*const* argv);

	void recv(Packet* pkt, Handler*);
protected:
        int off_ip_;
};

void NetworkInterface::recv(Packet* pkt, Handler* h) 
{
        hdr_ip *iph = (hdr_ip*)pkt->access(off_ip_);
	iph->iface() = label;
	//printf ("networkinterface: %u\n", iph->iface());
	send(pkt,h);
}

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
                        label = atoi(argv[2]);
                return (TCL_OK);
                }
        }
        return (Connector::command(argc, argv));
}
