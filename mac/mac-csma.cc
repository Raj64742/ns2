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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 *
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac-csma.cc,v 1.8 1997/07/22 22:19:09 kfall Exp $ (UCB)";
#endif

#include "template.h"
#include "random.h"
#include "channel.h"
#include "mac-csma.h"


static class CsmaMacClass : public TclClass {
public:
	CsmaMacClass() : TclClass("Mac/Csma") {}
	TclObject* create(int, const char*const*) {
		return (new CsmaMac);
	}
} class_mac_csma;

static class CsmaCdMacClass : public TclClass {
public:
	CsmaCdMacClass() : TclClass("Mac/Csma/Cd") {}
	TclObject* create(int, const char*const*) {
		return (new CsmaCdMac);
	}
} class_mac_csma_cd;

static class CsmaCaMacClass : public TclClass {
public:
	CsmaCaMacClass() : TclClass("Mac/Csma/Ca") {}
	TclObject* create(int, const char*const*) {
		return (new CsmaCaMac);
	}
} class_mac_csma_ca;


CsmaMac::CsmaMac() : Mac(), txstart_(0), rtx_(0), mhEoc_(this)
{
	bind_time("delay_", &delay_);
	bind_time("ifs_", &ifs_);
	bind_time("slotTime_", &slotTime_);
	bind("cwmin_", &cwmin_);
	bind("cwmax_", &cwmax_);
	bind("rtxLimit_", &rtxLimit_);
	cw_ = cwmin_;
}

CsmaCdMac::CsmaCdMac() : CsmaMac()
{
}

CsmaCaMac::CsmaCaMac() : CsmaMac()
{
}


void
CsmaMac::resume(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	s.schedule(callback_, &intr_, ifs_ + slotTime_ * cw_);
	if (p != 0)
		drop(p);
	callback_ = 0;
	state(MAC_IDLE);
	rtx_ = 0;
	cw_ = cwmin_;
}


void
CsmaMac::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	// if channel is not ready, then wait
	// else content for the channel
	if (channel_->txstop() + ifs_ > now)
		s.schedule(&mhSend_, p, channel_->txstop() + ifs_ - now);
	else {
		txstart_ = now;
		channel_->contention(p, &mhEoc_);
	}
}


void
CsmaMac::backoff(Handler* h, Packet* p, double delay)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	// if retransmission time within limit, do exponential backoff
	// else drop the packet and resume
	if (++rtx_ < rtxLimit_) {
		delay += max(channel_->txstop() + ifs_ - now, 0);
		int slot = Random::integer(cw_);
		s.schedule(h, p, delay + slotTime_ * slot);
		cw_ = min(2 * cw_, cwmax_);
	}
	else
		resume(p);
}


void
CsmaMac::endofContention(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p) - (s.clock() - txstart_);
	channel_->send(p, txt);
	s.schedule(&mh_, &intrEoc_, txt);
	rtx_ = 0;
	cw_ = cwmin_;
}


void
CsmaCdMac::endofContention(Packet* p)
{
	// If there is a collision, backoff
	if (channel_->collision()) {
		channel_->hold(0);
		backoff(&mhSend_, p);
	}
	else
		CsmaMac::endofContention(p);
}


void
CsmaCaMac::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	if (channel_->txstop() + ifs_ > now)
		backoff(&mhSend_, p);
	else {
		txstart_ = now;
		channel_->contention(p, &mhEoc_);
	}
}


void
MacHandlerEoc::handle(Event* e)
{
	mac_->endofContention((Packet*)e);
}
