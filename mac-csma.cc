/*
 * Copyright (c) 1996 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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

#include "random.h"
#include "mac-csma.h"


static class CsmaMacClass : public TclClass {
public:
	CsmaMacClass() : TclClass("Mac/Csma") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CsmaMac);
	}
} class_mac_csma;


static class CsmaCdMacClass : public TclClass {
public:
	CsmaCdMacClass() : TclClass("Mac/Csma/Cd") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CsmaCdMac);
	}
} class_mac_csma_cd;


static class CsmaCaMacClass : public TclClass {
public:
	CsmaCaMacClass() : TclClass("Mac/Csma/Ca") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CsmaCaMac);
	}
} class_mac_csma_ca;


void
MacHandlerEoc::handle(Event* e)
{
	mac_.endofContention((Packet*)e);
}


CsmaMac::CsmaMac() : Mac(), rtx_(0), mhEoc_(*this)
{
	bind_time("ifs_", &ifs_);
	bind_time("slotTime_", &slotTime_);
	bind("cwmin_", &cwmin_);
	bind("cwmax_", &cwmax_);
	bind("rtxmax_", &rtxmax_);
	cw_ = cwmin_;
}


void
CsmaMac::send(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	double txstop = channel_->txstop();

	// if there is an ongoing transmission, then backoff
	// else if within the ifs, then wait until the end of ifs
	// else do carrier sense
	if (txstop > now)
		backoff(p);
	else if (txstop + ifs_ > now)
		s.schedule(&mh_, p, txstop + ifs_ - now);
	else {
		txstart_ = now;
		channel_->senseCarrier(p, &mhEoc_);
	}
}


void
CsmaMac::backoff(Packet* p, double delay)
{
	Scheduler& s = Scheduler::instance();

	// if retransmission time within limit, do exponential backoff
	// else drop the packet and resume
	if (++rtx_ < rtxmax_) {
		double txstart = channel_->txstop() + ifs_;
		delay += max(txstart - s.clock(), 0);
		cw_ = min(2 * cw_, cwmax_);
		int slot = Random::integer(cw_) + 1;
		s.schedule(&mh_, p, delay + slotTime_ * slot);
	}
	else {
		callback_->handle(&intr_);
		drop(p);
		rtx_ = 0;
		cw_ = cwmin_;
	}
}


void
CsmaMac::endofContention(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p) - (s.clock() - txstart_);
	channel_->send(p, p->target(), txt);
	s.schedule(callback_, &intr_, txt);
}


void
CsmaCdMac::endofContention(Packet* p)
{
	// check for collision
	if (channel_->numtx() == 1)
		CsmaMac::endofContention(p);
	else {
		// XXX jamming the channel
		channel_->hold(0);
		backoff(p);
	}
}


void
CsmaCaMac::recv(Packet* p, Handler* h)
{
	callback_ = h;
	backoff(p);
}
