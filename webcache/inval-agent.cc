// Copyright (c) Xerox Corporation 1998. All rights reserved.
//
// License is granted to copy, to use, and to make and to use derivative
// works for research and evaluation purposes, provided that Xerox is
// acknowledged in all documentation pertaining to any such copy or
// derivative work. Xerox grants no other licenses expressed or
// implied. The Xerox trade name should not be used in any advertising
// without its written permission. 
//
// XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
// MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
// FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
// express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// software. 
//
// Agents used to send and receive invalidation records
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/inval-agent.cc,v 1.9 1999/03/09 05:20:44 haoboy Exp $

#include "inval-agent.h"
#include "ip.h"
#include "http.h"

// Implementation 1: Invalidation via multicast heartbeat
static class HttpInvalHeaderClass : public PacketHeaderClass {
public:
        HttpInvalHeaderClass() : PacketHeaderClass("PacketHeader/HttpInval",
						   sizeof(hdr_inval)) {}
} class_httpinvalhdr;

static class HttpInvalClass : public TclClass {
public:
	HttpInvalClass() : TclClass("Agent/HttpInval") {}
	TclObject* create(int, const char*const*) {
		return (new HttpInvalAgent());
	}
} class_httpinval_agent;

HttpInvalAgent::HttpInvalAgent() : Agent(PT_INVAL)
{
	bind("off_inv_", &off_inv_);
	// It should be initialized to the same as tcpip_base_hdr_size_
	bind("inval_hdr_size_", &inval_hdr_size_);
}

void HttpInvalAgent::recv(Packet *pkt, Handler*)
{
	hdr_ip *ip = (hdr_ip *)pkt->access(off_ip_);
	if (ip->src() == addr_)
		// XXX Why do we need this?
		return;
	if (app_ == 0) 
		return;
	hdr_inval *ih = (hdr_inval *)pkt->access(off_inv_);
	((HttpApp*)app_)->process_data(ih->size(), (char *)pkt->accessdata());
	Packet::free(pkt);
}

// Send a list of invalidation records in user data area
// realsize: the claimed size
// datasize: the actual size of user data, used to allocate packet
void HttpInvalAgent::send(int realsize, const AppData* data)
{
	Packet *pkt = allocpkt(data->size());
	hdr_inval *ih = (hdr_inval *)pkt->access(off_inv_);
	ih->size() = data->size();
	char *p = (char *)pkt->accessdata();
	data->pack(p);

	// Set packet size proportional to the number of invalidations
	hdr_cmn *ch = (hdr_cmn*) pkt->access(off_cmn_);
	ch->size() = inval_hdr_size_ + realsize;
	Agent::send(pkt, 0);
}


// Implementation 2: Invalidation via TCP. 
static class HttpUInvalClass : public TclClass {
public:
	HttpUInvalClass() : TclClass("Application/TcpApp/HttpInval") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc != 5)
			return NULL;
		Agent *a = (Agent *)TclObject::lookup(argv[4]);
		a->set_pkttype(PT_INVAL); // It's TCP but used for invalidation
		if (a == NULL)
			return NULL;
		return (new HttpUInvalAgent(a));
	}
} class_httpuinval_agent;

void HttpUInvalAgent::process_data(int size, char* data) 
{
	target_->process_data(size, data);
}

int HttpUInvalAgent::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "set-app") == 0) {
		// Compatibility interface
		HttpApp* c = 
			(HttpApp*)TclObject::lookup(argv[2]);
		target_ = (Process *)c;
		return TCL_OK;
	}
	return TcpApp::command(argc, argv);
}
