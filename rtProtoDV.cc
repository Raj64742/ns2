/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * rtProtoDV.cc
 *
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/rtProtoDV.cc,v 1.4 1998/06/27 01:24:36 gnguyen Exp $ (USC/ISI)";
#endif

#include "agent.h"
#include "rtProtoDV.h"

static class rtDVHeaderClass : public PacketHeaderClass {
public:
	rtDVHeaderClass() : PacketHeaderClass("PacketHeader/rtProtoDV",
					      sizeof(hdr_DV)) { } 
} class_rtProtoDV_hdr;

static class rtProtoDVclass : public TclClass {
public:
	rtProtoDVclass() : TclClass("Agent/rtProto/DV") {}
	TclObject* create(int, const char*const*) {
		return (new rtProtoDV);
	}
} class_rtProtoDV;


int rtProtoDV::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "send-update") == 0) {
		nsaddr_t  dst   = atoi(argv[2]);
		u_int32_t mtvar = atoi(argv[3]);
		u_int32_t size  = atoi(argv[4]);
		sendpkt(dst, mtvar, size);
		return TCL_OK;
	}
	return Agent::command(argc, argv);
}

void rtProtoDV::sendpkt(nsaddr_t dst, u_int32_t mtvar, u_int32_t size)
{
	dst_ = dst;
	size_ = size;
	
	Packet* p = Agent::allocpkt();
	hdr_DV *rh = (hdr_DV*)p->access(off_DV_);
	rh->metricsVar() = mtvar;

	target_->recv(p);
}

void rtProtoDV::recv(Packet* p, Handler*)
{
	hdr_DV* rh = (hdr_DV*)p->access(off_DV_);
	hdr_ip* ih = (hdr_ip*)p->access(off_ip_);
	Tcl::instance().evalf("%s recv-update %d %d", name(),
			      ih->src(), rh->metricsVar());
	Packet::free(p);
}
