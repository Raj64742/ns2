/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * rtProtoDV.h
 * Copyright (C) 1997 by USC/ISI
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/rtProtoDV.h,v 1.4 1998/08/12 23:41:13 gnguyen Exp $ (USC/ISI)
 */

#ifndef ns_rtprotodv_h
#define ns_rtprotodv_h

#include "packet.h"
#include "ip.h"

struct hdr_DV {
	u_int32_t mv_;			// metrics variable identifier

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_DV* access(Packet* p) {
		return (hdr_DV*) p->access(offset_);
	}

	// per field member functions
	u_int32_t& metricsVar() { return mv_; }
};

class rtProtoDV : public Agent {
public:
	rtProtoDV() : Agent(PT_RTPROTO_DV) { 
		bind("off_DV_", &off_DV_);
	}
	int command(int argc, const char*const* argv);
	void sendpkt(nsaddr_t dst, u_int32_t z, u_int32_t mtvar);
	void recv(Packet* p, Handler*);
protected:
	int off_DV_;
};

#endif
