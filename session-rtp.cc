/*
 * Copyright (c) @ Regents of the University of California.
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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/session-rtp.cc,v 1.1 1996/12/19 03:22:45 mccanne Exp $
 */

#include <Tcl.h>
#include "packet.h"
#include "rtp.h"

static class RTPSessionClass : public TclClass {
public:
	RTPSessionClass() : TclClass("Session/RTP") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new RTPSession());
	}
} class_rtp_session;

RTPSession::RTPSession() 
	: allsrcs_(0), last_np_(0)
{
	localsrc_ = new RTPSource(0);
}

RTPSession::~RTPSession() 
{
	while (allsrcs_ != 0) {
		RTPSource* p = allsrcs_;
		allsrcs_ = allsrcs_->next;
		delete p;
	}
	delete localsrc_;
}

void RTPSession::localsrc_update(int cc)
{
	localsrc_->np(1);
}

#define RTCP_HDRSIZE 8
#define RTCP_SR_SIZE 20
#define RTCP_RR_SIZE 48

int RTPSession::build_report(int bye)
{
	int nsrc = 0;
	int nrr = 0;
	int len = RTCP_HDRSIZE;
	int we_sent = 0;
	if (localsrc_->np() != last_np_) {
		last_np_ = localsrc_->np();
		we_sent = 1;
		len += RTCP_SR_SIZE;
	}
	for (RTPSource* sp = allsrcs_; sp != 0; sp = sp->next) {
		++nsrc;
	        int received = sp->np() - sp->snp();
		if (received == 0) {
			continue;
		}
		sp->snp(sp->np());
		len += RTCP_RR_SIZE;
		if (++nrr >= 31)
			break;
	}

	if (bye) 
		len += build_bye();
	else 
		len += build_sdes();

	Tcl::instance().evalf("%s adapt-timer %d %d %d", name(), 
			      nsrc, nrr, we_sent);
	Tcl::instance().evalf("%s sample-size %d", name(), len);

	return (len);
}


int RTPSession::build_bye() 
{
	return (8);
}

int RTPSession::build_sdes()
{
	/* XXX We'll get to this later... */
	return (20);
}


void RTPSession::recv(Packet* p)
{
	RTPSource* s = lookup(p->src_);
	if (s == 0) {
		s = new RTPSource(p->src_);
		enter(s);
	}
	s->np(1);
	s->ehsr(p->seqno_);
}

void RTPSession::recv_ctrl(Packet* p)
{
	Tcl::instance().evalf("%s sample-size %d", name(), p->size_);
}

/* XXX Should use hash this... */
RTPSource* RTPSession::lookup(nsaddr_t src)
{
	for (RTPSource *p = allsrcs_; p != 0; p = p->next)
		if (p->addr() == src)
			return (p);

	return (0);
}

void RTPSession::enter(RTPSource* s)
{
	s->next = allsrcs_;
	allsrcs_ = s;
}

int RTPSession::command(int argc, const char*const* argv)
{
	return (TclObject::command(argc, argv));
}

RTPSource::RTPSource(nsaddr_t addr)
	: next(0), addr_(addr), np_(0), snp_(0), ehsr_(-1)
{}
