/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/channel.cc,v 1.22.2.1 1998/10/08 04:22:42 yuriy Exp $ (UCB)";
#endif

#include "template.h"
#include "trace.h"
#include "mac.h"
#include "channel.h"

static class ChannelClass : public TclClass {
public:
	ChannelClass() : TclClass("Channel") {}
	TclObject* create(int, const char*const*) {
		return (new Channel);
	}
} class_channel;

static class DuplexChannelClass : public TclClass {
public:
	DuplexChannelClass() : TclClass("Channel/Duplex") {}
	TclObject* create(int, const char*const*) {
		return (new DuplexChannel);
	}
} class_channel_duplex;


Channel::Channel() : Connector(), txstop_(0), cwstop_(0), numtx_(0), pkt_(0), trace_(0)
{
	bind_time("delay_", &delay_);
}

int Channel::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "trace-target") == 0) {
			trace_ = (Trace*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		if (strcmp(argv[1], "trace-target") == 0) {
			tcl.resultf("%s", trace_->name());
			return (TCL_OK);
		}
	}
	return Connector::command(argc, argv);
}


void Channel::recv(Packet* p, Handler*)
{
	Scheduler& s = Scheduler::instance();
	s.schedule(target_, p, txstop_ + delay_ - s.clock());
}


// send():
//  The packet occupies the channel for the transmission time, txtime
//  If collision occur (>1 pkts overlap), corrupt all pkts involved
//	by setting the error bit or discard them

int Channel::send(Packet* p, double txtime)
{
	// without collision, return 0
	Scheduler& s = Scheduler::instance();
	double now = s.clock();

	// busy = time when the channel are still busy with earlier tx
	double busy = max(txstop_, cwstop_);

	// txstop = when the channel is no longer busy from this tx
	txstop_ = max(busy, now + txtime);

	// now < busy => collision
	//	mark the pkt error bit, EF_COLLISION
	//	drop if there is a drop target, drop_
	if (now < busy) {
		// if still transmit earlier packet, pkt_, then corrupt it
		if (pkt_ && pkt_->time_ > now) {
			hdr_cmn::access(pkt_)->error() |= EF_COLLISION;
			if (drop_) {
				s.cancel(pkt_);
				drop(pkt_);
				pkt_ = 0;
			}
		}

		// corrupts the current packet p, and drop if drop_ exists
		hdr_cmn::access(p)->error() |= EF_COLLISION;
		if (drop_) {
			drop(p);
			return 1;
		}
	}

	// if p was not dropped, call recv() or hand it to trace_ if present
	pkt_ = p;
	trace_ ? trace_->recv(p, 0) : recv(p, 0);
	return 0;
}


// contention():
//  The MAC calls this Channel::contention() to enter contention period
//  It determines when the contention window is over, cwstop_,
//	and schedule a callback to the MAC for the actual send()

void Channel::contention(Packet* p, Handler* h)
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


// jam():
//  Jam the channel for a period txtime
//  Some MAC protocols use this to let other MAC detect collisions

int Channel::jam(double txtime)
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


int DuplexChannel::send(Packet* p, double txtime)
{
	double now = Scheduler::instance().clock();
	txstop_ = now + txtime;
	trace_ ? trace_->recv(p, 0) : recv(p, 0);
	return 0;
}


void DuplexChannel::contention(Packet* p, Handler* h)
{
	Scheduler::instance().schedule(h, p, delay_);
	numtx_ = 1;
}
