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

//Basic Measurement block derived from a connector
//measures bits sent and average packet delay in a measurement interval

#include "packet.h"
#include "connector.h"
#include "measuremod.h"

static class MeasureModClass : public TclClass {
public:
	MeasureModClass() : TclClass("MeasureMod") {}
	TclObject* create(int, const char*const*) {
		return (new MeasureMod());
	}
}class_measuremod;


MeasureMod::MeasureMod() : nbits_(0),ndelay_(0),npkts_(0),avdelay_(0)
{
}


void MeasureMod::recv(Packet *p,Handler *h)
{
	hdr_cmn *ch=(hdr_cmn*)p->access(off_cmn_);
	nbits_ += ch->size()<<3;
	npkts_++;
	ndelay_+= Scheduler::instance().clock()-ch->timestamp();
	avdelay_=ndelay_/npkts_;
	send(p,h);
}
