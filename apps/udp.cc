/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (C) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/udp.cc,v 1.12 1998/08/22 02:41:29 haoboy Exp $ (Xerox)";
#endif

#include "udp.h"
#include "rtp.h"
#include "random.h"

static class UdpAgentClass : public TclClass {
public:
	UdpAgentClass() : TclClass("Agent/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UdpAgent());
	}
} class_udp_agent;

UdpAgent::UdpAgent() : seqno_(-1), Agent(PT_UDP)
{
	bind("packetSize_", &size_);
	bind("off_rtp_", &off_rtp_);
}

UdpAgent::UdpAgent(int type) : Agent(type)
{
	bind("packetSize_", &size_);
}

void UdpAgent::sendmsg(int nbytes, const char* /*flags*/)
{
	Packet *p;
	int n;

	if (size_)
		n = nbytes / size_;
	else
		printf("Error: UDP size = 0\n");

	if (nbytes == -1) {
		printf("Error:  sendmsg() for UDP should not be -1\n");
		return;
	}	
	while (n-- > 0) {
		p = allocpkt();
		hdr_rtp* rh = (hdr_rtp*)p->access(off_rtp_);
		rh->seqno() = ++seqno_;
		target_->recv(p);
	}
	n = nbytes % size_;
	if (n > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		hdr_rtp* rh = (hdr_rtp*)p->access(off_rtp_);
		rh->seqno() = ++seqno_;
		target_->recv(p);
	}
	idle();
}

