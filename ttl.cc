/*
 * Copyright (c) 1996-1997 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ttl.cc,v 1.7 1997/09/10 16:59:56 kannan Exp $";
#endif

#include "packet.h"
#include "ip.h"
#include "connector.h"

class TTLChecker : public Connector {
public:
	TTLChecker() {
		bind("off_ip_", &off_ip_);
	}
	//int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h) {
		hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
		int ttl = iph->ttl() - 1;
		if (ttl <= 0) {
			/* XXX should send to a drop object.*/
			// Yes, and now it does...
			// Packet::free(p);
			printf("ttl exceeded\n");
			drop(p);
			return;
		}
		iph->ttl() = ttl;
		send(p, h);
	}
protected:
	int off_ip_;
};

static class TTLCheckerClass : public TclClass {
public:
	TTLCheckerClass() : TclClass("TTLChecker") {}
	TclObject* create(int, const char*const*) {
		return (new TTLChecker);
	}
} ttl_checker_class;
