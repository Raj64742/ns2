/*
 * Copyright (c) 2001 University of Southern California.
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
 **
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This defines the Quick Start packet header
 * 
 * hdr_qs.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#ifndef _HDR_QS_H
#define _HDR_QS_H

#include <assert.h>
#include <packet.h>

//#define QS_DEBUG 1 // get some output on QS events -PS

enum QS_STATE { QS_DISABLE, QS_REQUEST, QS_RESPONSE };

struct hdr_qs {
 
	int flag_;	// 1 for request, 0 for response all others invalid	
	int ttl_; 
	int rate_; 

	static int offset_;	// offset for this header 
	inline int& offset() { return offset_; }

	inline static hdr_qs* access(Packet* p) {
		return (hdr_qs*) p->access(offset_);
	}

	int& flag() { return flag_; }
	int& ttl() { return ttl_; }
	int& rate() { return rate_; }

        static double rate_to_Bps(int rate);
        static int Bps_to_rate(double Bps);
};

// Local Variables:
// mode:c++
// c-basic-offset: 8
// End:
#endif

