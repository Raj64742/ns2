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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/inval-agent.cc,v 1.3 1998/12/16 21:10:57 haoboy Exp $

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

HttpInvalAgent::HttpInvalAgent() : 
	Agent(PT_INVAL), a_(0)
{
	bind("off_inv_", &off_inv_);
}

void HttpInvalAgent::recv(Packet *pkt, Handler*)
{
	hdr_ip *ip = (hdr_ip *)pkt->access(off_ip_);
	if (ip->src() == addr_)
		// XXX Why do we need this?
		return;
	if (a_ == 0) 
		return;
	hdr_inval *ih = (hdr_inval *)pkt->access(off_inv_);
	char *data = (char *)pkt->accessdata();
	a_->recv_pkt(ih->size(), data);
	Packet::free(pkt);
}

// Send a list of invalidation records in user data area
// realsize: the claimed size
// datasize: the actual size of user data, used to allocate packet
void HttpInvalAgent::send_data(int realsize, int datasize, const char* data)
{
	Packet *pkt = allocpkt(datasize);
	hdr_inval *ih = (hdr_inval *)pkt->access(off_inv_);
	ih->size() = datasize;
	char *p = (char *)pkt->accessdata();
	memcpy(p, data, datasize);

	// Set packet size proportional to the number of invalidations
	hdr_cmn *ch = (hdr_cmn*) pkt->access(off_cmn_);
	ch->size() = sizeof(hdr_inval) + realsize;
	send(pkt, 0);
}


// Implementation 2: Invalidation via TCP. 
static class HttpUInvalClass : public TclClass {
public:
	HttpUInvalClass() : TclClass("Application/TcpApp/HttpInval") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc != 5)
			return NULL;
		Agent *a = (Agent *)TclObject::lookup(argv[4]);
		if (a == NULL)
			return NULL;
		return (new HttpUInvalAgent(a));
	}
} class_httpuinval_agent;

void HttpUInvalAgent::process_data(int size, char *data) 
{
	a_->recv_pkt(size, data);
}

int HttpUInvalAgent::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "set-app") == 0) {
		HttpMInvalCache* c = 
			(HttpMInvalCache*)TclObject::lookup(argv[2]);
		set_app(c);
		return TCL_OK;
	}
	return TcpApp::command(argc, argv);
}
