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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/udp.cc,v 1.8 1998/06/27 01:03:43 gnguyen Exp $ (Xerox)";
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

UdpAgent::UdpAgent() : Agent(PT_UDP), seqno_(0)
{
}

void UdpAgent::sendmsg(int nbytes, const char* flags)
{
	Packet *p;
	int n = nbytes / size_;

	if (nbytes == -1) {
		printf("Error:  sendmsg() for UDP should not be -1\n");
		return;
	}	
	while (n-- > 0) {
		p = allocpkt();
		target_->recv(p);
	}
	n = nbytes % size_;
	if (n > 0) {
		p = allocpkt();
		hdr_cmn::access(p)->size() = n;
		target_->recv(p);
	}
	idle();
}


/*
 * UDP_Agent below is a both an Agent and an application
 */

//"rtp timestamp" needs the samplerate
#define SAMPLERATE 8000

static class UDP_AgentClass : public TclClass {
 public:
	UDP_AgentClass() : TclClass("Agent/CBR/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UDP_Agent());
	}
} class_source_agent;

UDP_Agent::UDP_Agent() : trafgen_(0),nextPkttime_(-1), rtd_(0), callback_(0)
{
}

UDP_Agent::~UDP_Agent()
{
	if (callback_)
		delete[] callback_;

}

int UDP_Agent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return(TCL_OK);
		} else if (strcmp(argv[1], "stop") == 0) {
			stop();
			return(TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "attach-traffic") == 0) {
			trafgen_ = (TrafficGenerator *)TclObject::lookup(argv[2]);
			if (trafgen_ == 0) {
				tcl.resultf("no such node %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}	
	}
	return(Agent::command(argc, argv));
}

void UDP_Agent::sendpkt()
{
	Packet* p = allocpkt();
	hdr_rtp* rh = (hdr_rtp*)p->access(off_rtp_);
	rh->seqno() = ++seqno_;
	rh->flags()=0;
	
	double local_time=Scheduler::instance().clock();
	/*put in "rtp timestamps" and begining of talkspurt labels */
	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	ch->timestamp()=(u_int32_t)(SAMPLERATE*local_time);
	if ((nextPkttime_ != trafgen_->interval()) || (nextPkttime_ == -1))
		rh->flags() |= RTP_M;
	
	target_->recv(p);
}

void UDP_Agent::timeout(int)
{
	if (running_) {
		/* send a packet */
		sendpkt();
		/* figure out when to send the next one */
		nextPkttime_ = trafgen_->next_interval(size_);
		/* schedule it */
		cbr_timer_.resched(nextPkttime_);

		/* hack: if we are waiting for a current burst to end
		 * before stopping . . .
		 */
		if (rtd_) {
			if (trafgen_->on() == 0) {
				stop();
				//Tcl::instance().evalf("puts \"%s burst over at %f\"", 
				// name(), Scheduler::instance().clock());
				Tcl::instance().evalf("%s sched-stop %d", name(), 0);
			}
		}
	}
}

void UDP_Agent::stop()
{
	cbr_timer_.cancel();
	running_ = 0;
}


void UDP_Agent::stoponidle(const char *s)
{
	callback_ = new char[strlen(s)+1];
	strcpy(callback_, s);

	if (trafgen_->on()) {
		// Tcl::instance().evalf("puts \"%s waiting for burst at %f\"", name(), Scheduler::instance().clock());
		rtd_ = 1;
	}
	else {
		stop();
		Tcl::instance().evalf("%s %s", name(), callback_);
	}

}

void UDP_Agent::start()
{
	if (trafgen_) {
		trafgen_->init();
		running_ = 1;
		double t = trafgen_->next_interval(size_);
		cbr_timer_.resched(t);
	}
}
