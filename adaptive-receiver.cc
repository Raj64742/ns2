/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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

#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/adaptive-receiver.cc,v 1.7 2000/09/01 03:04:05 haoboy Exp $";
#endif

#include "config.h"
#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "rtp.h"
#include "adaptive-receiver.h"
#include <math.h>

#define myabs(r) (r<0)?-r:r

AdaptiveRcvr::AdaptiveRcvr() : Agent(PT_NTYPE)
{
	//bind("npkts_",&npkts_);
	//bind("ndelay_",&ndelay_);
	//bind("nvar_",&nvar_);
}

void AdaptiveRcvr::recv(Packet *pkt,Handler*)
{
	int delay;
	int seq_no;
	hdr_cmn* ch= hdr_cmn::access(pkt);
	hdr_rtp *rh = hdr_rtp::access(pkt);
	
	seq_no= rh->seqno();
	register u_int32_t send_time = (int)ch->timestamp();
	
	u_int32_t local_time= (u_int32_t)(Scheduler::instance().clock() * SAMPLERATE);
	delay=adapt(pkt,local_time);
	Tcl::instance().evalf("%s print-delay-stats %f %f %f",name(),send_time/SAMPLERATE,local_time/SAMPLERATE,(local_time+delay)/SAMPLERATE);
	Packet::free(pkt);
}

