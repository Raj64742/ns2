/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 *	Group at the University of California, Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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

#include "Tcl.h"
#include "net.h"
#include "packet.h"
#include "agent.h"

class TapAgent : public Agent, public IOHandler {
 public:
        TapAgent();
	int command(int, const char*const*);
	void recv(Packet* p, Handler*);
 protected:
	void dispatch(int);
	int off_tap_;
	Network* net_;
	void sendpkt(Packet*);
	void recvpkt();
};

static class TapAgentClass : public TclClass {
 public:
	TapAgentClass() : TclClass("Agent/Tap") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TapAgent());
	}
} class_tap_agent;

TapAgent::TapAgent() : Agent(/*XXX*/PT_MESSAGE)
{
	bind("off_tap_", &off_tap_);
}

int TapAgent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "network") == 0) {
			tcl.result(name());
			return(TCL_OK);
		} 
	}
	if (argc == 3) {
		if (strcmp(argv[1], "network") == 0) {
			net_ = (Network *)TclObject::lookup(argv[2]);
			unlink();
			if (net_ != 0)
				link(net_->rchannel(), TCL_READABLE);
			return(TCL_OK);
		}	
	}
	return (Agent::command(argc, argv));
}

struct hdr_tap {
	/*XXX*/
	u_char buf[1600];
};

class TapHeaderClass : public PacketHeaderClass {
public:
        TapHeaderClass() : PacketHeaderClass("PacketHeader/Tap",
					     sizeof(hdr_tap)) {}
} class_taphdr;

/*
 * Receive a packet off the network and inject into the simulation.
 */
void TapAgent::recvpkt()
{
	Packet* p = allocpkt();
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);
	u_int32_t addr;
	int cc = net_->recv(ht->buf, sizeof(ht->buf), addr);
	hdr_cmn* th = (hdr_cmn*)p->access(off_cmn_);
	th->size() = cc;
	target_->recv(p);
}

void TapAgent::dispatch(int)
{
	/*
	 * Just process one packet.  We could put a loop here
	 * but instead we allow the dispatcher to call us back
	 * if there is a queue in the socket buffer; this allows
	 * other events to get a chance to slip in...
	 */
	Scheduler::instance().sync();
	recvpkt();
}

/*
 * Receive a packet from the simulation and inject into the network.
 */
void TapAgent::sendpkt(Packet* p)
{
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);
	hdr_cmn* hc = (hdr_cmn*)p->access(off_cmn_);
	net_->send(ht->buf, hc->size());
}

void TapAgent::recv(Packet* p, Handler*)
{
	/*
	 * didn't expect packet (or we're a null agent?)
	 */
	sendpkt(p);
	Packet::free(p);
}
