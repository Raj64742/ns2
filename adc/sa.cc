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

//packets after it succeeds in a3-way handshake from the receiver
// should be connected with Agent/SignalAck class

#include "udp.h"
#include "sa.h"
#include "ip.h"
#include "tclcl.h"
#include "packet.h"
#include "random.h"

SA_Agent::SA_Agent() {
	bind ("off_resv_",&off_resv_);
	bind_bw("rate_",&rate_);
	bind("bucket_",&bucket_);
}

static class SA_AgentClass : public TclClass {
public:
	SA_AgentClass() : TclClass("Agent/CBR/UDP/SA") {}
	TclObject* create(int, const char*const*) {
		return (new SA_Agent());
	}
} class_signalsource_agent;

int SA_Agent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc==3) {
		if (strcmp(argv[1], "target") == 0) {
			target_ = (NsObject*)TclObject::lookup(argv[2]);
			if (target_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			ctrl_target_=target_;
			return (TCL_OK);
		} 
		else if (strcmp(argv[1],"ctrl-target")== 0) {
			ctrl_target_=(NsObject*)TclObject::lookup(argv[2]);
			if (ctrl_target_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (UDP_Agent::command(argc,argv));
}


void SA_Agent::start()
{
	//send the request packet
	if (trafgen_) {
		trafgen_->init();
		//running_=1;
		sendreq();
	}
}

void SA_Agent::stop()
{
	sendteardown();
	if (running_ != 0) {
		cbr_timer_.cancel();
		running_ =0;
	}
}

void SA_Agent::sendreq()
{
	Packet *p = allocpkt();
	hdr_cmn* ch= (hdr_cmn*)p->access(off_cmn_);
	ch->ptype()=PT_REQUEST;
	ch->size()=20;
	//also put in the r,b parameters for the flow in the packet
	hdr_resv* rv=(hdr_resv*)p->access(off_resv_);
	rv->decision() =1;
	rv->rate()=rate_;
	rv->bucket()=bucket_;
	ctrl_target_->recv(p);
}

void SA_Agent::sendteardown()
{
	Packet *p = allocpkt();
	hdr_cmn* ch= (hdr_cmn*)p->access(off_cmn_);
	ch->ptype()=PT_TEARDOWN;
	//also put in the r,b parameters for the flow in the packet
	hdr_resv* rv=(hdr_resv*)p->access(off_resv_);
	rv->decision() =1;
	rv->rate()=rate_;
	rv->bucket()=bucket_;
	ctrl_target_->recv(p);
}


void SA_Agent::recv(Packet *p, Handler *) 
{
	hdr_cmn *ch= (hdr_cmn *)p->access(off_cmn_);
	hdr_resv *rv=(hdr_resv *)p->access(off_resv_);
	hdr_ip * iph = (hdr_ip*)p->access(off_ip_);
	if ( ch->ptype() == PT_ACCEPT || ch->ptype() == PT_REJECT ) {
		ch->ptype() = PT_CONFIRM;
		// turn the packet around by swapping src and dst
		nsaddr_t tmp;
		tmp = iph->src();
		iph->src()=iph->dst();
		iph->dst()=tmp;
		ctrl_target_->recv(p);
		
	}
	
	// put an additional check here to see if admission was granted
	if (rv->decision()) {
		//printf("Flow %d accepted @ %f\n",iph->flowid(),Scheduler::instance().clock());
		fflush(stdout);
		double t = trafgen_->next_interval(size_);
		running_=1;
		cbr_timer_.resched(t);
	}
	else {
		//printf("Flow %d rejected @ %f\n",iph->flowid(),Scheduler::instance().clock());
		fflush(stdout);
		//Currently the flow is stopped if rejected
		running_=0;
	}
	//make an upcall to sched a stoptime for this flow from now
	Tcl::instance().evalf("%s sched-stop %d",name(),rv->decision());
}

