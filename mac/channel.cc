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

#include "channel.h"


static class ChannelClass : public TclClass {
public:
	ChannelClass() : TclClass("Channel") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Channel);
	}
} class_channel;


static class DuplexChannelClass : public TclClass {
public:
	DuplexChannelClass() : TclClass("Channel/Duplex") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DuplexChannel);
	}
} class_channel_duplex;


Channel::Channel() : BiConnector(), txstop_(0), cwstop_(0), numtx_(0)
{
	bind_time("delay_", &delay_);
}


int
Channel::command(int argc, const char*const* argv)
{
	return BiConnector::command(argc, argv);
}


void
Channel::content(Packet* p, Handler* h)
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
Channel::send(Packet* p, Handler* target, double txtime, double txstart)
{
	// without collision, return 0
	Scheduler& s = Scheduler::instance();
	double now = s.clock();
	double busy = max(txstop_, cwstop_);
	txstart = (txstart > 0) ? txstart : now;
	txstop_ = txstart + txtime;
	if (txstart < busy) {
		if (pkt_ && pkt_->time_ > now) {
			s.cancel(pkt_);
			drop(pkt_);
			pkt_ = 0;
		}
		drop(p);
	}
	else if (p->error())
		drop(p);
	else {
		pkt_ = p;
		s.schedule(target, p, txstop_ + delay_ - txstart);
		return 0;
	}
	return 1;
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
DuplexChannel::content(Packet* p, Handler* h)
{
	Scheduler& s = Scheduler::instance();
	s.schedule(h, p, delay_);
}


int
DuplexChannel::send(Packet* p, Handler* target, double txtime, double txstart)
{
	Scheduler& s = Scheduler::instance();

	if (p->error())
		drop(p);
	else {
		pkt_ = p;
		s.schedule(target, p, txstop_ + delay_ - txstart);
		return 0;
	}
	return 1;
}


int
DuplexChannel::hold(double txtime)
{
	return 0;
}
