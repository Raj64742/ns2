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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/net-interface.h,v 1.2 2000/08/17 00:03:38 haoboy Exp $
 *
 * Ported by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 */

#ifndef ns_net_int_h
#define ns_net_int_h

#include "connector.h"
#include "packet.h"
#include "lib/bsd-list.h"

class NetworkInterface;
LIST_HEAD(netint_head, NetworkInterface); // declare list head structure

class NetworkInterface : public Connector {
public:
	NetworkInterface() : intf_label_(UNKN_IFACE.value()) { /*NOTHING*/ }
	int command(int argc, const char*const* argv); 
	void recv(Packet* p, Handler* h);
	int32_t intf_label() { return intf_label_; } 
	// list of all networkinterfaces on a node
        inline void insertiface(struct netint_head *head) {
                LIST_INSERT_HEAD(head, this, iface_entry);
        } 
	NetworkInterface* nextiface(void) const { return iface_entry.le_next; }

protected:
	int32_t intf_label_;
	LIST_ENTRY(NetworkInterface) iface_entry;
};

#endif
