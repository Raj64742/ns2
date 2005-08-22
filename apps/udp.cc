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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/udp.cc,v 1.20 2005/08/22 05:08:32 tomh Exp $ (Xerox)";
#endif

#include "udp.h"
#include "rtp.h"
#include "random.h"
#include "address.h"
#include "ip.h"


static class UdpAgentClass : public TclClass {
public:
	UdpAgentClass() : TclClass("Agent/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UdpAgent());
	}
} class_udp_agent;

UdpAgent::UdpAgent() : Agent(PT_UDP), seqno_(-1)
{
	bind("packetSize_", &size_);
}

UdpAgent::UdpAgent(packet_t type) : Agent(type)
{
	bind("packetSize_", &size_);
}

// put in timestamp and sequence number, even though UDP doesn't usually 
// have one.
void UdpAgent::sendmsg(int nbytes, AppData* data, const char* flags)
{
	Packet *p;
	int n;

	assert (size_ > 0);

	n = nbytes / size_;

	if (nbytes == -1) {
		printf("Error:  sendmsg() for UDP should not be -1\n");
		return;
	}	

	// If they are sending data, then it must fit within a single packet.
	if (data && nbytes > size_) {
		printf("Error: data greater than maximum UDP packet size\n");
		return;
	}

	double local_time = Scheduler::instance().clock();
	while (n-- > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = size_;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 ==strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);
		target_->recv(p);
	}
	n = nbytes % size_;
	if (n > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		hdr_rtp* rh = hdr_rtp::access(p);
		rh->flags() = 0;
		rh->seqno() = ++seqno_;
		hdr_cmn::access(p)->timestamp() = 
		    (u_int32_t)(SAMPLERATE*local_time);
		// add "beginning of talkspurt" labels (tcl/ex/test-rcvr.tcl)
		if (flags && (0 == strcmp(flags, "NEW_BURST")))
			rh->flags() |= RTP_M;
		p->setdata(data);
		target_->recv(p);
	}
	idle();
}
void UdpAgent::recv(Packet* pkt, Handler*)
{
	if (app_ ) {
		// If an application is attached, pass the data to the app
		hdr_cmn* h = hdr_cmn::access(pkt);
		app_->process_data(h->size(), pkt->userdata());
	} else if (pkt->userdata() && pkt->userdata()->type() == PACKET_DATA) {
		// otherwise if it's just PacketData, pass it to Tcl
		//
		// Note that a Tcl procedure Agent/Udp recv {from data}
		// needs to be defined.  For example,
		//
		// Agent/Udp instproc recv {from data} {puts data}

		PacketData* data = (PacketData*)pkt->userdata();

		hdr_ip* iph = hdr_ip::access(pkt);
                Tcl& tcl = Tcl::instance();
		tcl.evalf("%s process_data %d {%s}", name(),
		          iph->src_.addr_ >> Address::instance().NodeShift_[1],
			  data->data());
	}
	Packet::free(pkt);
}


int UdpAgent::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "send") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "sendmsg") == 0) {
			PacketData* data = new PacketData(1 + strlen(argv[3]));
			strcpy((char*)data->data(), argv[3]);
			sendmsg(atoi(argv[2]), data, argv[4]);
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
