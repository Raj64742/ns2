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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/packet.cc,v 1.3 1997/03/10 21:53:31 kfall Exp $ (LBL)";
#endif

#include "packet.h"

int Packet::hdrsize_ = 0;	/* size of a packet's header */
int Packet::uidcnt_;		/* running unique id */
Packet* Packet::free_;		/* free list */

/* manages active packet header types */
class PacketHeaderManager : public TclObject {
	int		nheaders_;
	int		cur_offset_;
	void allochdr(PacketHeader *p);
public:
	PacketHeaderManager() : nheaders_(0), cur_offset_(0) { }
	int nheaders() { return (nheaders_); }
	int PacketHeaderManager::command(int, const char*const*);
};

static class PacketHeaderManagerClass : public TclClass {
public:
        PacketHeaderManagerClass() : TclClass("PacketHeaderManager") {}
        TclObject* create(int argc, const char*const* argv) {
                return (new PacketHeaderManager);
        }
} class_packethdr_mgr;

void
PacketHeaderManager::allochdr(PacketHeader *p)
{
	/* round up to nearest NS_ALIGN bytes */
	int incr = (p->hdrsize() + (NS_ALIGN-1)) & ~(NS_ALIGN-1);
	cur_offset_ += incr;
	Packet::addhsize(incr);
	p->setbase(cur_offset_);
}

/*
 * $pm newheader $newh
 */
int PacketHeaderManager::command(int argc, const char*const* argv)
{
	PacketHeader *ph;
	if (argc == 3) {
		if (strcmp(argv[1], "newheader") == 0) {
			ph = (PacketHeader*)TclObject::lookup(argv[2]);
			if (ph == NULL) {
				return (TCL_ERROR);
			}
			allochdr(ph);
			return (TCL_OK);
		}
	}
}
