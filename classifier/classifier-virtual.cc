/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/classifier/classifier-virtual.cc,v 1.1 1998/10/14 20:31:10 polly Exp $";
#endif

#include "config.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

class VirtualClassifier : public Classifier {
protected:
	NsObject* next_;
	int classify(Packet *const p) {
		hdr_ip* iph = hdr_ip::access(p);
		return mshift(iph->dst());
	}
	void recv(Packet* p, Handler* h) {
		int dst = classify(p);
		next_ = NULL;
		Tcl::instance().evalf("%s find %d", name(), dst);
		if (next_ == NULL) {
			/*
			 * XXX this should be "dropped" somehow.  Right now,
			 * these events aren't traced.
			 */
			Packet::free(p);
			return;
		}
		next_->recv(p,h);
	}
	int command(int argc, const char*const* argv) {
		if (argc == 3) {
			if (strcmp(argv[1], "set-target") == 0) {
				next_ = (NsObject*)TclObject::lookup(argv[2]);
				return(TCL_OK);
			}
		}
		return (NsObject::command(argc, argv));
	}
};

static class VirtualClassifierClass : public TclClass {
public:
	VirtualClassifierClass() : TclClass("Classifier/Virtual") {}
	TclObject* create(int, const char*const*) {
		return (new VirtualClassifier());
	}
} class_virtual_classifier;

