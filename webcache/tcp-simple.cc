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
// SimpleTcp: Only share the same interface as FullTcp.
// It's inherited from FullTcp solely for interface reason... :(
//
// If we have interface declaration independent from class type definition,
// we'll be better off.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/tcp-simple.cc,v 1.2 1998/12/16 21:11:01 haoboy Exp $

#include <stdlib.h>
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "app.h"
#include "tcp-simple.h"

static class SimpleTcpClass : public TclClass {
public:
	SimpleTcpClass() : TclClass("Agent/TCP/SimpleTcp") {}
	TclObject* create(int, const char*const*) {
		return (new SimpleTcpAgent());
	}
} class_simple_tcp_agent;

SimpleTcpAgent::SimpleTcpAgent() : TcpAgent(), seqno_(0)
{
}

// XXX Do *NOT* support infinite send of TCP (bytes == -1).
void SimpleTcpAgent::sendmsg(int bytes, const char* /*flags*/)
{
	if (bytes == -1) {
		fprintf(stderr, 
"SimpleTcp doesn't support infinite send. Do not use FTP::start(), etc.\n");
		return;
	}
	// Simply sending out bytes out to target_
	curseq_ += bytes;
	seqno_ ++;

	Packet *p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	tcph->seqno() = seqno_;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
        hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
	th->size() = bytes + tcpip_base_hdr_size_;
	send(p, 0);
}

void SimpleTcpAgent::recv(Packet *pkt, Handler *)
{
        hdr_cmn *th = (hdr_cmn*)pkt->access(off_cmn_);
	int datalen = th->size() - tcpip_base_hdr_size_;
	if (app_)
		app_->recv(datalen);
	// No lastbyte_ callback, because no packet fragmentation.
	Packet::free(pkt);
}

int SimpleTcpAgent::command(int argc, const char*const* argv)
{
	// Copy FullTcp's tcl interface

	if (argc == 2) {
		if (strcmp(argv[1], "listen") == 0) {
			// Do nothing
			return (TCL_OK);
		}
		if (strcmp(argv[1], "close") == 0) {
			// Call done{} to match tcp-full's syntax
			Tcl::instance().evalf("%s done", name());
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc, argv));
}
