
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
#ifdef HAVE_STL
#include "nix/hdr_nv.h"

#ifdef NIXVECTOR

// Define the TCL glue for the packet header
int hdr_nv::offset_;

static class NVHeaderClass : public PacketHeaderClass {
public:
        NVHeaderClass() : PacketHeaderClass("PacketHeader/NV",
					    sizeof(hdr_nv)) {
		bind_offset(&hdr_nv::offset_);
	}
} class_nvhdr;

#endif /* NIXVECTOR */
#endif
