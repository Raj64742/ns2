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
	int off_resv_;
};

SAack_Agent::SAack_Agent(): Agent(PT_TCP)
{
	bind("off_resv_",&off_resv_);
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
	hdr_cmn *ch = (hdr_cmn*)p->access(off_cmn_);
	
	if (ch->ptype() == PT_REQUEST) {
		Packet *newp =allocpkt();
		hdr_cmn *newch=(hdr_cmn*)newp->access(off_cmn_);
		newch->size()=ch->size();
		// turn the packet around by swapping src and dst
		hdr_ip * iph = (hdr_ip*)p->access(off_ip_);
		hdr_ip * newiph = (hdr_ip*)newp->access(off_ip_);
		newiph->dst()=iph->src();
		newiph->flowid()=iph->flowid();
		newiph->prio()=iph->prio();
		
		hdr_resv* rv=(hdr_resv*)p->access(off_resv_);
		hdr_resv* newrv=(hdr_resv*)newp->access(off_resv_);
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
  
  

