/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * mcast_ctrl.cc
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mcast/mcast_ctrl.cc,v 1.1.2.1 1998/08/10 19:06:10 yuriy Exp $ (LBL)";
#endif

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "mcast_ctrl.h"

class mcastControlAgent : public Agent {
public:
	mcastControlAgent() : Agent(-1) {
		bind("packetSize_", &size_);
		bind("off_mcast_ctrl_", &off_mcast_ctrl_);
	}

	void recv(Packet* pkt, Handler*) {
		hdr_mcast_ctrl* ph = (hdr_mcast_ctrl*)pkt->access(off_mcast_ctrl_);
		hdr_cmn* ch = (hdr_cmn*)pkt->access(off_cmn_);
		// Agent/Mcast/Control instproc handle {type from src group iface}
		Tcl::instance().evalf("%s handle %s %d %d %d %d", name(),
				      ph->type(), ph->from(),
				      ph->src(), ph->group(), ch->iface());
		Packet::free(pkt);
	}

	/*
 	 * $proc send $type $src $group
 	 */
	int mcastControlAgent::command(int argc, const char*const* argv) {
		if (argc == 6) {
			if (strcmp(argv[1], "send") == 0) {

#define	CASE(c,str,type)						\
		case (c):	if (strcmp(argv[2], (str)) == 0) {	\
			type_ = (type);					\
			break;						\
		}							\
		/*FALLTHROUGH*/

				switch (*argv[2]) {
					CASE('p', "prune", PT_PRUNE);
					CASE('g', "graft", PT_GRAFT);
					CASE('j', "join",  PT_JOIN);
					CASE('a', "assert", PT_ASSERT);
				default:
					Tcl& tcl = Tcl::instance();
					tcl.result("invalid control message");
					return (TCL_ERROR);
				}

				Packet* pkt = allocpkt();
				hdr_mcast_ctrl* ph = (hdr_mcast_ctrl*)pkt->access(off_mcast_ctrl_);
				strcpy(ph->type(), argv[2]);
				ph->from()  = atoi(argv[3]);
				ph->src()   = atoi(argv[4]);
				ph->group() = atoi(argv[5]);
				send(pkt, 0);
				return (TCL_OK);
			}
		}
		return (Agent::command(argc, argv));
	}

protected:
	int off_mcast_ctrl_;
};

//
// Now put the standard OTcl linkage templates here
//
static class mcastControlHeaderClass : public PacketHeaderClass {
public:
        mcastControlHeaderClass() : PacketHeaderClass("PacketHeader/mcastCtrl",
					     sizeof(hdr_mcast_ctrl)) {}
} class_mcast_ctrl_hdr;

static class mcastControlClass : public TclClass {
public:
	mcastControlClass() : TclClass("Agent/Mcast/Control") {}
	TclObject* create(int, const char*const*) {
		return (new mcastControlAgent());
	}
} class_mcast_ctrl;
