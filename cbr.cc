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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/cbr.cc,v 1.8 1997/03/29 01:42:45 mccanne Exp $ (LBL)";
#endif

#include "cbr.h"
#include "rtp.h"
#include "Tcl.h"
#include "packet.h"
#include "random.h"

static class CBRClass : public TclClass {
public:
	CBRClass() : TclClass("Agent/CBR") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CBR_Agent());
	}
} class_cbr;

CBR_Agent::CBR_Agent() : Agent(PT_CBR), random_(0)
{
	Tcl& tcl = Tcl::instance();
	bind_time("interval_", &interval_);
	bind("packetSize_", &size_);
	bind("random_", &random_);
	bind("off_rtp_", &off_rtp_);
	running_ = 0;
}

void CBR_Agent::start()
{
	running_ = 1;
	sendpkt();
	sched(interval_, 0);
}

void CBR_Agent::stop()
{
	cancel(0);
	running_ = 0;
}

void CBR_Agent::timeout(int)
{
	if (running_) {
		sendpkt();
		double t = interval_;
		if (random_)
			/* add some zero-mean white noise */
			t += interval_ * Random::uniform(-0.5, 0.5);
		sched(t, 0);
	}
}

void CBR_Agent::sendpkt()
{
	Packet* p = allocpkt();
	hdr_rtp* rh = (hdr_rtp*)p->access(off_rtp_);
	rh->seqno() = ++seqno_;
	target_->recv(p);
}

/*
 * $proc interval $interval
 * $proc size $size
 */
int CBR_Agent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}
