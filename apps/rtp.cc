/*
 * Copyright (c) 1997 Regents of the University of California.
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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/apps/rtp.cc,v 1.5 1997/02/27 04:39:05 kfall Exp $";
#endif


#include <stdlib.h>
#include <string.h>

#include "Tcl.h"
#include "agent.h"
#include "packet.h"
#include "cbr.h"
#include "random.h"
#include "rtp.h"

RTPHeader rtphdr;
RTPHeader* RTPHeader::myaddress_ = &rtphdr;
static class RTPHeaderClass : public TclClass {
public:
        RTPHeaderClass() : TclClass("PacketHeader/RTP") {}
        TclObject* create(int argc, const char*const* argv) {
                        return &rtphdr;
        }       
} class_rtphdr;


class RTPAgent : public CBR_Agent {
public:
	RTPAgent();
	virtual void timeout(int);
	virtual void recv(Packet* p, Handler*);
	virtual int command(int argc, const char*const* argv);
protected:
	virtual void sendpkt();
	RTPSession* session_;
};

static class RTPAgentClass : public TclClass {
public:
	RTPAgentClass() : TclClass("Agent/CBR/RTP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RTPAgent());
	}
} class_rtp_agent;


RTPAgent::RTPAgent() 
	: session_(0)
{
	type_ = PT_RTP;
}

void RTPAgent::timeout(int) 
{
	if (running_) {
		sendpkt();
		session_->localsrc_update(size_);
		double t = interval_;
		if (random_)
			/* add some zero-mean white noise */
			t += interval_ * Random::uniform(-0.5, 0.5);
		sched(t, 0);
	}
}

void RTPAgent::recv(Packet* p, Handler*)
{
	session_->recv(p);
}

int RTPAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "session") == 0) {
			session_ = (RTPSession*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (CBR_Agent::command(argc, argv));
}

void RTPAgent::sendpkt()
{
	Packet* p = allocpkt();
	RTPHeader *rh = RTPHeader::access(p->bits());

	/* Fill in srcid_ */
	rh->srcid() = session_->srcid();
	target_->recv(p);
}
