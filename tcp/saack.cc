/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/saack.cc,v 1.7 2000/09/01 03:04:07 haoboy Exp $
 */

//SignalAckClass
//Null receiver agent which merely acknowledges a request

#include "agent.h"
#include "ip.h"
#include "resv.h"

class SAack_Agent : public Agent {
public:
	SAack_Agent();
	
protected:
	void recv(Packet *, Handler *);
	int command(int,const char* const*);
};

SAack_Agent::SAack_Agent(): Agent(PT_TCP)
{
}

static class SAackClass : public TclClass {
public:
	SAackClass() : TclClass("Agent/SAack") {}
	TclObject* create(int, const char*const*) {
		return (new SAack_Agent());
	}
} class_saack;


void SAack_Agent::recv(Packet *p, Handler *h)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	
	if (ch->ptype() == PT_REQUEST) {
		Packet *newp =allocpkt();
		hdr_cmn *newch=hdr_cmn::access(newp);
		newch->size()=ch->size();
		// turn the packet around by swapping src and dst
		hdr_ip * iph = hdr_ip::access(p);
		hdr_ip * newiph = hdr_ip::access(newp);
		newiph->dst()=iph->src();
		newiph->flowid()=iph->flowid();
		newiph->prio()=iph->prio();
		
		hdr_resv* rv=hdr_resv::access(p);
		hdr_resv* newrv=hdr_resv::access(newp);
		newrv->decision()=rv->decision();
		newrv->rate()=rv->rate();
		newrv->bucket()=rv->bucket();
		if (rv->decision())
		        newch->ptype() = PT_ACCEPT;
		else
		        newch->ptype() = PT_REJECT;
		target_->recv(newp);
		Packet::free(p);
		return;
	}
	Agent::recv(p,h);
}


int SAack_Agent::command(int argc, const char*const* argv)
{
	return (Agent::command(argc,argv));
	
}
  
  

