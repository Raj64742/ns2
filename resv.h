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

/* define a simple reservation header service */

#ifndef ns_resv_h
#define ns_resv_h

#include "config.h"
#include "packet.h"

struct hdr_resv {
	double rate_; 
	int bucket_;
	int decision_; //decision bit put in by intermediate routers

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_resv* access(Packet* p) {
		return (hdr_resv*) p->access(offset_);
	}

	//provide per field member functions
	double& rate() { return rate_; }
	int& bucket() { return bucket_; }
	int& decision() { return decision_; }
};

#endif





