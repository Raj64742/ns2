/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/delay.cc,v 1.20 1998/06/27 01:23:41 gnguyen Exp $ (LBL)";
#endif

#include "delay.h"
#include "prune.h"
#include "ctrMcast.h"

static class LinkDelayClass : public TclClass {
public:
	LinkDelayClass() : TclClass("DelayLink") {}
	TclObject* create(int /* argc */, const char*const* /* argv */) {
		return (new LinkDelay);
	}
} class_delay_link;

LinkDelay::LinkDelay() : Connector(), dynamic_(0), itq_(0), nextPacket_(0)
{
	bind_bw("bandwidth_", &bandwidth_);
	bind_time("delay_", &delay_);
	bind("off_ip_", &off_ip_);
	bind("off_prune_", &off_prune_);
	bind("off_CtrMcast_", &off_CtrMcast_);
}

int LinkDelay::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "dynamic") == 0) {
			dynamic_ = 1;
			itq_ = new PacketQueue();
			return TCL_OK;
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "pktintran") == 0) {
			int src = atoi(argv[2]);
			int grp = atoi(argv[3]);
			int from = atoi(argv[4]);
			int to = atoi(argv[5]);
			pktintran (src, grp);
			Tcl::instance().evalf("%s puttrace %d %d %d %d %d %d %d %d", name(), total_[0], total_[1], total_[2], total_[3], src, grp, from, to);
			return TCL_OK;
		}
	}

	return Connector::command(argc, argv);
}

void LinkDelay::recv(Packet* p, Handler* h)
{
	double txt = txtime(p);
	Scheduler& s = Scheduler::instance();
	if (dynamic_) {
		Event* e = (Event*)p;
		e->time_ = s.clock() + txt + delay_;
		itq_->enque(p);
		schedule_next();
	} else {
		s.schedule(target_, p, txt + delay_);
	}
	/*XXX only need one intr_ since upstream object should
	 * block until it's handler is called
	 *
	 * This only holds if the link is not dynamic.  If it is, then
	 * the link itself will hold the packet, and call the upstream
	 * object at the appropriate time.  This second interrupt is
	 * called inTransit_, and is invoked through schedule_next()
	 */
	s.schedule(h, &intr_, txt);
}

void LinkDelay::send(Packet* p, Handler*)
{
	target_->recv(p, (Handler*) NULL);
}

void LinkDelay::reset()
{
	if (nextPacket_) {
		Scheduler::instance().cancel(&inTransit_);
		drop(nextPacket_);
		nextPacket_ = (Packet*) NULL;
		Packet* np;
		while ((np = itq_->deque()) != 0)
			drop(np);
	}
}

void LinkDelay::handle(Event* e)
{
	assert(nextPacket_);
	Event* pe = (Event*) nextPacket_;
	assert(pe->time_ == e->time_);
	send(nextPacket_, (Handler*) NULL);
	nextPacket_ = (Packet*) NULL;
	schedule_next();
}

void LinkDelay::pktintran(int src, int group)
{
	int reg = 1;
	int prune = 30;
	int graft = 31;
	int data = 0;
	for (int i=0; i<4; i++) {
		total_[i] = 0;
	}

	if (! dynamic_)
		return;

	int len = itq_->length();
	while (len) {
		len--;
		Packet* p = itq_->lookup(len);
		hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
		if (iph->flowid() == prune) {
			hdr_prune* ph = (hdr_prune*)p->access(off_prune_);
			if (ph->src() == src && ph->group() == group) {
				total_[0]++;
			}
		} else if (iph->flowid() == graft) {
			hdr_prune* ph = (hdr_prune*)p->access(off_prune_);
			if (ph->src() == src && ph->group() == group) {
				total_[1]++;
			}
		} else if (iph->flowid() == reg) {
			hdr_CtrMcast* ch = (hdr_CtrMcast*)p->access(off_CtrMcast_);
			if (ch->src() == src+1 && ch->group() == group) {
				total_[2]++;
			}
		} else if (iph->flowid() == data) {
			if (iph->src() == src+1 && iph->dst() == group) {
				total_[3]++;
			}
		}
	}
        //printf ("%f %d %d %d %d\n", Scheduler::instance().clock(), total_[0], total_[1], total_[2],total_[3]);
}
