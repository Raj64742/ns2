/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * lanRouter.h
 * Copyright (C) 1998 by USC/ISI
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
 * @(#) $Header: /usr/src/mash/repository/vint/ns-2/lanRouter.h
 */

#ifndef ns_lanRouter_h
#define ns_lanRouter_h

#include "config.h"
#include "object.h"
#include "route.h"
#include "classifier.h"

class LanRouter : public NsObject {
public:
	LanRouter() : routelogic_(0), switch_(0), enableHrouting_(false) {};
	virtual int next_hop(Packet *p);
protected:
	int command(int argc, const char*const* argv);
	void recv(Packet *, Handler * = 0) {} //not used

	RouteLogic *routelogic_;  // for lookups of the next hop
	Classifier *switch_;      // to tell mcast from ucast
	char lanaddr_[SMALL_LEN]; // address of the lan
	bool enableHrouting_;     // type of routing used: flat or hierarchical
	
};

#endif
