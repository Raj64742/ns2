/*
 * Copyright (c) 1997 The Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/tap.cc,v 1.2 1998/02/21 03:03:11 kfall Exp $ (UCB)";
#endif

#include "tclcl.h"
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
	int sendpkt(Packet*);
	void recvpkt();
};

static class TapAgentClass : public TclClass {
 public:
	TapAgentClass() : TclClass("Agent/Tap") {}
	TclObject* create(int, const char*const*) {
		return (new TapAgent());
	}
} class_tap_agent;

TapAgent::TapAgent() : Agent(PT_LIVE), net_(NULL)
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
			if (net_ != 0) {
				int chan = net_->rchannel();
				if (chan < 0) {
					fprintf(stderr,
					"TapAgent(%s): network %s not open\n",
					    name(), argv[2]);
					return (TCL_ERROR);
				}
				link(chan, TCL_READABLE);
			} else {
				fprintf(stderr,
				"TapAgent(%s): unknown network %s\n",
				    name(), argv[2]);
				return (TCL_ERROR);
			}
			if (off_tap_ == 0) {
				fprintf(stderr,
				"TapAgent(%s): warning: off_tap == 0: ",
				    name(), argv[2]);
				fprintf(stderr, "is RealTime scheduler on?\n");
			}
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
	sockaddr addr;

	Packet* p = allocpkt();
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);
	int cc = net_->recv(ht->buf, sizeof(ht->buf), addr);
	if (cc < 0) {
		perror("recv");
	}

	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
printf("TAP(%s): recvpkt of size %d\n", name(), cc);
	ch->size() = cc;
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
printf("TAP(%s): dispatch invoked\n", name());
	Scheduler::instance().sync();
	recvpkt();
}

/*
 * Receive a packet from the simulation and inject into the network.
 * if there is no network attached, call Connector::drop() to send
 * to drop target
 */

void TapAgent::recv(Packet* p, Handler*)
{
	if (sendpkt(p) == 0)
		Packet::free(p);

	return;
}

int
TapAgent::sendpkt(Packet* p)
{
	// send packet into the live network
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);
	hdr_cmn* hc = (hdr_cmn*)p->access(off_cmn_);
	if (net_ == NULL) {
		drop(p);
		return -1;
	}
printf("TAP(%s): sending pkt (net:0x%p)\n", name(), net_);
	if (net_->send(ht->buf, hc->size()) < 0) {
		perror("send");
	}
	return 0;
}
