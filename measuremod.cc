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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/measuremod.cc,v 1.4 2000/09/01 03:04:06 haoboy Exp $
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


MeasureMod::MeasureMod() : nbits_(0),npkts_(0)
{
}


void MeasureMod::recv(Packet *p,Handler *h)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	nbits_ += ch->size()<<3;
	npkts_++;
	send(p,h);
}
