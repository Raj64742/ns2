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

#include "mac.h"
#include "channel.h"


static class ChannelClass : public TclClass {
public:
	ChannelClass() : TclClass("Channel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Channel);
	}
} class_channel;


Channel::Channel() : txstop_(0), cwstop_(0), numtx_(0)
{
	bind_time("delay_", &delay_);
}


int
Channel::command(int argc, const char*const* argv)
{
	return (Connector::command(argc, argv));
}


void
Channel::recv(Packet* p, Handler* h)
{
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	if (now > cwstop_) {
		cwstop_ = now + delay_;
		numtx_ = 0;
	}
	numtx_++;
	s.schedule(h, p, cwstop_ - now);
}


int
Channel::send(Packet* p, NsObject* target, double txtime, double txstart)
{
	// without collision, return 0
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	double busy = max(txstop_, cwstop_);
	txstart = (txstart > 0) ? txstart : now;
	txstop_ = txstart + txtime;
	if (p->error() || txstart < busy) {
		drop(p);
		return 1;
	}
	s.schedule(target, p, txstop_ - s.clock());
	return 0;
}


int
Channel::hold(double txtime)
{
	// without collision, return 0
	double now = Scheduler::instance().clock();
	if (txstop_ > now) {
		txstop_ = max(txstop_, now + txtime);
		return 1;
	}
	txstop_ = now + txtime;
	return (now < cwstop_);
}


void
Channel::drop(Packet* p)
{
	// XXX: log dropped packet
	Packet::free(p);
}
