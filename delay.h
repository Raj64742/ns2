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

#ifndef ns_delay_h
#define ns_delay_h

#include "packet.h"
#include "queue.h"
#include "ip.h"

class InTransitQ;

class LinkDelay : public Connector {
 public:
	LinkDelay();
	void recv(Packet* p, Handler*);
	void send(Packet* p, Handler*);
	inline void drop(Packet* p) { Packet::free(p); }
	double delay() { return delay_; }
	inline double txtime(Packet* p) {
		hdr_cmn *hdr = (hdr_cmn*)p->access(off_cmn_);
		return (hdr->size() * 8. / bandwidth_);
	}

 protected:
	int command(int argc, const char*const* argv);
	void reset();
	double bandwidth_;	/* bandwidth of underlying link (bits/sec) */
	double delay_;		/* line latency */
	Event intr_;
	int dynamic_;		/* indicates whether or not link is ~ */
	InTransitQ* intq_;
};


class InTransitQ : public PacketQueue , public Handler {
public:
	InTransitQ(LinkDelay* ld) : ld_(ld), nextPacket_(0) {}

	void schedule_next() {
		Packet* np;
		if (np = deque()) {
			Scheduler& s = Scheduler::instance();
			double txt = ld_->txtime(np);
			if (nextPacket_)	// just got delivered
				s.schedule(this, &itq_, txt);
			else
				s.schedule(this, &itq_, txt + ld_->delay());
		}
		nextPacket_ = np;
	}

	void handle(Event* e) {
		ld_->send(nextPacket_, (Handler*) NULL);
		schedule_next();
	}

	void hold_in_transit(Packet* p) {
		enque(p);
		if (! nextPacket_)
			schedule_next();
	}

	void reset() {
		if (nextPacket_) {
			Scheduler::instance().cancel(&itq_);
			Packet::free(nextPacket_);
			nextPacket_ = (Packet*) NULL;
			while (Packet* np = deque())
				Packet::free(np);
		}
	}

private:
	LinkDelay* ld_;
	Packet* nextPacket_;
	Event itq_;
};

#endif
