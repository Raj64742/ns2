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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/tap.cc,v 1.7 1998/03/13 23:28:56 kfall Exp $ (UCB)";
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
	int linknet();
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

//
// link in a network to the agent.  Assumes net_ is non-zero
//
int
TapAgent::linknet()
{
	int mode = net_->mode();
	int rchan = net_->rchannel();
	int wchan = net_->schannel();

	unlink();
	if (mode == O_RDONLY || mode == O_RDWR) {
		// reading enabled?
		if (rchan < 0) {
			fprintf(stderr,
			"TapAgent(%s): network %s not open for reading\n",
			    name(), net_->name());
			return (TCL_ERROR);
		}
		link(rchan, TCL_READABLE);
	} else if (mode != O_WRONLY) {
		if (mode == -1) {
			fprintf(stderr,
			   "TapAgent(%s): Network(%s) not opened properly.\n",
				name(), net_->name());
			fprintf(stderr,
			   "(choose: readonly, readwrite, or writeonly)\n");
		} else {
			fprintf(stderr,
			    "TapAgent(%s): unknown mode %d in Network(%s)\n",
				name(), mode, net_->name());
		}
		return (TCL_ERROR);
	}

	if (mode == O_WRONLY || mode == O_RDWR) {
		// writing enabled?
		if (wchan < 0) {
			fprintf(stderr,
			"TapAgent(%s): network %s not open for writing\n",
			    name(), net_->name());
			return (TCL_ERROR);
		}
	}
	if (off_tap_ == 0) { 
		fprintf(stderr, "TapAgent(%s): warning: off_tap == 0: ",
		    name());
		fprintf(stderr, "is RealTime scheduler on?\n");
	}
	return (TCL_OK);
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
			if (net_ != 0) {
				return(linknet());
			} else {
				fprintf(stderr,
				"TapAgent(%s): unknown network %s\n",
				    name(), argv[2]);
				return (TCL_ERROR);
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

	if (net_->mode() != O_RDWR && net_->mode() != O_RDONLY) {
		fprintf(stderr,
		  "TapAgent(%s): recvpkt called while in write-only mode!\n",
		  name());
		return;
	}


	Packet* p = allocpkt();
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);

	int cc = net_->recv(ht->buf, sizeof(ht->buf), addr);
	if (cc <= 0) {
		if (cc < 0) {
			perror("recv");
		}
		return;
	}

	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	ch->size() = cc;
	target_->recv(p);
	return;
}

void TapAgent::dispatch(int)
{
	/*
	 * Just process one packet.  We could put a loop here
	 * but instead we allow the dispatcher to call us back
	 * if there is a queue in the socket buffer; this allows
	 * other events to get a chance to slip in...
	 */
	Scheduler::instance().sync();	// sim clock gets set to now

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
	if (net_->mode() != O_RDWR && net_->mode() != O_WRONLY) {
		fprintf(stderr,
		    "TapAgent(%s): recvpkt called while in write-only mode!\n",
		    name());
		return -1;
	}

	// send packet into the live network
	hdr_tap* ht = (hdr_tap*)p->access(off_tap_);
	hdr_cmn* hc = (hdr_cmn*)p->access(off_cmn_);
	if (net_ == NULL) {
		drop(p);
		return -1;
	}
	if (net_->send(ht->buf, hc->size()) < 0) {
		perror("send");
	}
	return 0;
}
