/*
 * Copyright (c) 2000 University of Southern California.
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
 *
 * NixVector classifier for stateless routing
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 *
 */

#ifndef __HDR_NV_H__
#define __HDR_NV_H__

#include "packet.h"
#include "nix/nixvec.h"

struct hdr_nv {
        NixVec* pNv;
        Nixl_t  h_used;
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_nv* access(Packet* p) {
		return (hdr_nv*) p->access(offset_);
	}

	/* per-field member acces functions */
	NixVec*& nv()   { return (pNv); }
        Nixl_t*  used() { return &h_used;}
};


#endif

