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
#ifndef lint
static const char  rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/consrcvr.cc,v 1.7 2000/09/01 03:04:05 haoboy Exp $";
#endif


#include "agent.h"
#include "config.h"
#include "tclcl.h"
#include "packet.h"
#include "rtp.h"
#include "adaptive-receiver.h"
#ifndef WIN32
// VC 5.0 doesn't have this
#include <sys/time.h>
#endif

//#define CONS_OFFSET 0.025*SAMPLERATE
#define CONS_OFFSET 200

class ConsRcvr : public AdaptiveRcvr {
public:
	ConsRcvr();
protected:
	int adapt(Packet *pkt, u_int32_t time);
	int offset_;
};

static class ConsRcvrClass : public TclClass {
public:
	ConsRcvrClass() : TclClass("Agent/ConsRcvr") {}
	TclObject* create(int, const char*const*) {
		return (new ConsRcvr());

	}
} class_cons_rcvr;


ConsRcvr::ConsRcvr() : offset_(CONS_OFFSET)
{
}

int ConsRcvr::adapt(Packet *pkt, u_int32_t local_clock)
{
	
	int delay;
	hdr_cmn* ch = hdr_cmn::access(pkt);
	register u_int32_t tstamp = (int)ch->timestamp();
	
	if (((tstamp+offset_) < local_clock) || (offset_ == -1)) {
		/*increase the offset */
		if (offset_ < (int)(local_clock-(tstamp+offset_)))
			offset_ += local_clock -(tstamp+offset_);
		else
			offset_ += offset_;
	}
	
	delay=offset_-(local_clock-tstamp);
	
	return delay;
}
