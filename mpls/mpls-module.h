/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
 * Copyright (C) 2000 by USC/ISI
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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mpls/mpls-module.h,v 1.2 2001/02/22 19:45:40 haldar Exp $
 *
 * Definition of MPLS node plugin module
 */

#ifndef ns_mpls_module_h
#define ns_mpls_module_h

#include <tclcl.h>

#include "lib/bsd-list.h"
#include "rtmodule.h"
#include "mpls/ldp.h"

class MPLSModule : public RoutingModule {
public:
	MPLSModule() : RoutingModule(), last_inlabel_(0), last_outlabel_(1000) {
		LIST_INIT(&ldplist_);
	}
	virtual ~MPLSModule();
	virtual const char* module_name() const { return "MPLS"; }
	virtual void add_route(char *dst, NsObject *target);
	virtual int command(int argc, const char*const* argv);
protected:
	// Do we have a LDP agent that peers with <nbr> ?
	LDPAgent* exist_ldp(int nbr);
	inline void attach_ldp(LDPAgent *a) {
		LDPListElem *e = new LDPListElem(a);
		LIST_INSERT_HEAD(&ldplist_, e, link);
	}
	void detach_ldp(LDPAgent *a);

	int last_inlabel_;
	int last_outlabel_;
	struct LDPListElem {
		LDPListElem(LDPAgent *a) : agt_(a) {
			link.le_next = NULL;
			link.le_prev = NULL;
		}
		LDPAgent* agt_;
		LIST_ENTRY(LDPListElem) link;
	};
	LIST_HEAD(LDPList, LDPListElem);
	LDPList ldplist_;
};

#endif // ns_mpls_module_h
