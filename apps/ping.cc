// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/ping.cc,v 1.4 2001/10/18 22:36:09 buchheim Exp $

/*
 * File: Code for a new 'Ping' Agent Class for the ns
 *       network simulator
 * Author: Marc Greis (greis@cs.uni-bonn.de), May 1998
 *
 * IMPORTANT: Incase of any changes made to this file , 
 * tutorial/examples/ping.cc file (used in Greis' tutorial) should
 * be updated as well.
 */

#include "ping.h"

int hdr_ping::offset_;
static class PingHeaderClass : public PacketHeaderClass {
public:
	PingHeaderClass() : PacketHeaderClass("PacketHeader/Ping", 
					      sizeof(hdr_ping)) {
		bind_offset(&hdr_ping::offset_);
	}
} class_pinghdr;


static class PingClass : public TclClass {
public:
	PingClass() : TclClass("Agent/Ping") {}
	TclObject* create(int, const char*const*) {
		return (new PingAgent());
	}
} class_ping;


PingAgent::PingAgent() : Agent(PT_PING)
{
	bind("packetSize_", &size_);
}

int PingAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "send") == 0) {
			// Create a new packet
			Packet* pkt = allocpkt();
			// Access the Ping header for the new packet:
			hdr_ping* hdr = hdr_ping::access(pkt);
			// Set the 'ret' field to 0, so the receiving node
			// knows that it has to generate an echo packet
			hdr->ret = 0;
			// Store the current time in the 'send_time' field
			hdr->send_time = Scheduler::instance().clock();
			// Send the packet
			send(pkt, 0);
			// return TCL_OK, so the calling function knows that
			// the command has been processed
			return (TCL_OK);
		}
	}
	// If the command hasn't been processed by PingAgent()::command,
	// call the command() function for the base class
	return (Agent::command(argc, argv));
}


void PingAgent::recv(Packet* pkt, Handler*)
{
	// Access the IP header for the received packet:
	hdr_ip* hdrip = hdr_ip::access(pkt);
	// Access the Ping header for the received packet:
	hdr_ping* hdr = hdr_ping::access(pkt);
	// Is the 'ret' field = 0 (i.e. the receiving node is being pinged)?
	if (hdr->ret == 0) {
		// Send an 'echo'. First save the old packet's send_time
		double stime = hdr->send_time;
		// Discard the packet
		Packet::free(pkt);
		// Create a new packet
		Packet* pktret = allocpkt();
		// Access the Ping header for the new packet:
		hdr_ping* hdrret = hdr_ping::access(pktret);
		// Set the 'ret' field to 1, so the receiver won't send
		// another echo
		hdrret->ret = 1;
		// Set the send_time field to the correct value
		hdrret->send_time = stime;
		// Send the packet
		send(pktret, 0);
	} else {
		// A packet was received. Use tcl.eval to call the Tcl
		// interpreter with the ping results.
		// Note: In the Tcl code, a procedure
		// 'Agent/Ping recv {from rtt}' has to be defined which
		// allows the user to react to the ping result.
		char out[100];
		// Prepare the output to the Tcl interpreter. Calculate the
		// round trip time
		sprintf(out, "%s recv %d %3.1f", name(), 
			hdrip->src_.addr_ >> Address::instance().NodeShift_[1],
			(Scheduler::instance().clock()-hdr->send_time) * 1000);
		Tcl& tcl = Tcl::instance();
		tcl.eval(out);
		// Discard the packet
		Packet::free(pkt);
	}
}


