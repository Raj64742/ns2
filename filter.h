/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * filter.h
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/filter.h,v 1.2 1998/09/23 03:31:12 yuriy Exp $ (USC/ISI)
 */

#ifndef ns_filter_h
#define ns_filter_h

#include "connector.h"

class Filter : public Connector {
public:
	Filter();
	inline NsObject* filter_target() { return filter_target_; }
protected:
	enum filter_e { DROP, PASS, FILTER, DUPLICATE };
	virtual filter_e filter(Packet* p);

	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler* h= 0);
	NsObject* filter_target_; // target for the matching packets
};

class FieldFilter : public Filter {
public:
	FieldFilter(); 
protected:
	filter_e filter(Packet *p);
	int offset_; // offset of the field
	int match_;
};
#endif
