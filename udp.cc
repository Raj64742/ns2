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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/udp.cc,v 1.3 1997/08/10 07:50:04 mccanne Exp $ (Xerox)";
#endif

#include "udp.h"
#include "rtp.h"
#include "tclcl.h"
#include "packet.h"
#include "random.h"

static class UDP_AgentClass : public TclClass {
 public:
	UDP_AgentClass() : TclClass("Agent/CBR/UDP") {}
	TclObject* create(int, const char*const*) {
		return (new UDP_Agent());
	}
} class_source_agent;

UDP_Agent::UDP_Agent() : trafgen_(0)
{
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
	target_->recv(p);
}

void UDP_Agent::timeout(int)
{
        if (running_) {
	        /* send a packet */
	        sendpkt();
		/* figure out when to send the next one */
		double t = trafgen_->next_interval(size_);
		/* schedule it */
		sched(t, 0);
	}
}

void UDP_Agent::stop()
{
        cancel(0);
	running_ = 0;
}

void UDP_Agent::start()
{
        if (trafgen_) {
	        trafgen_->init();
	        running_ = 1;
		double t = trafgen_->next_interval(size_);
		sched(t, 0);
	}
}
