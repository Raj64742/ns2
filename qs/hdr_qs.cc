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
 * Packet header definition for Quick Start implementation
 * hdr_qs.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#include <stdio.h>
#include "hdr_qs.h"

int hdr_qs::offset_;

static class QSHeaderClass : public PacketHeaderClass {
public:
	QSHeaderClass() : PacketHeaderClass("PacketHeader/TCP_QS", sizeof(hdr_qs)) {
		bind_offset(&hdr_qs::offset_);
	}

} class_QShdr;
