/*
 * Copyright (c) 1994-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/packet.cc,v 1.7 1997/08/22 00:09:33 gnguyen Exp $ (LBL)";
#endif

#include "packet.h"
#include "flags.h"

int Packet::hdrlen_ = 0;	/* size of a packet's header */
Packet* Packet::free_;		/* free list */

PacketHeaderClass::PacketHeaderClass(const char* className, int hdrlen) : 
	TclClass(className), hdrlen_(hdrlen)
{
}

void PacketHeaderClass::bind()
{
	TclClass::bind();
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s set hdrlen_ %d", classname_, hdrlen_);
}

void PacketHeaderClass::bind_offset(const char* fieldName, int offset)
{
	Tcl& tcl = Tcl::instance();
	tcl.evalf("%s set offset_(%s) %d", classname_, fieldName, offset);
}

TclObject* PacketHeaderClass::create(int, const char*const*)
{
	return (0);
}

class CommonHeaderClass : public PacketHeaderClass {
public:
        CommonHeaderClass() : PacketHeaderClass("PacketHeader/Common",
						sizeof(hdr_cmn)) {}
} class_cmnhdr;

class FlagsHeaderClass : public PacketHeaderClass {
public:
        FlagsHeaderClass() : PacketHeaderClass("PacketHeader/Flags",
						sizeof(hdr_flags)) {}
} class_flagshdr;

/* manages active packet header types */
class PacketHeaderManager : public TclObject {
public:
	PacketHeaderManager() {
		bind("hdrlen_", &Packet::hdrlen_);
	}
};

static class PacketHeaderManagerClass : public TclClass {
public:
        PacketHeaderManagerClass() : TclClass("PacketHeaderManager") {}
        TclObject* create(int, const char*const*) {
                return (new PacketHeaderManager);
        }
} class_packethdr_mgr;
