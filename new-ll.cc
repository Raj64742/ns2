/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 *
 * $Id: new-ll.cc,v 1.2 1998/12/10 18:34:41 haldar Exp $
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/new-ll.cc,v 1.2 1998/12/10 18:34:41 haldar Exp $ (UCB)";
#endif

#include <delay.h>
// #include <object.h>
#include <packet.h>

//#include <debug.h>
#include <list.h>
#include <arp.h>
#include <topography.h>
#include <trace.h>
//#include <node.h>
#include <new-mac.h>
#include <new-ll.h>
#include <random.h>

int newhdr_ll::offset_;
static class newLLHeaderClass : public PacketHeaderClass {
public:
	newLLHeaderClass()
		: PacketHeaderClass("PacketHeader/newLL", sizeof(newhdr_ll)) { 
		bind_offset(&newhdr_ll::offset_);
}
} class_llhdr;

static class newLLClass : public TclClass {
public:
	newLLClass() : TclClass("newLL") {}
	TclObject* create(int, const char*const*) {
		return (new newLL);
	}
} class_ll;



newLL::newLL() : LinkDelay(), seqno_(0)
{
	mac_ = 0;
	ifq_ = 0;
	downtarget_ = 0;
	uptarget_ = 0;

	bind("off_newll_", &off_newll_);
	bind("off_newmac_", &off_newmac_);

	bind_time("mindelay_", &mindelay_);
}


void
newLL::handle(Event *e)
{
	/* Handler must be non-zero for outgoing packets. */

	recv((Packet*) e, (Handler*) 0);
}

int
newLL::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
                if(strcmp(argv[1], "arptable") == 0) {
                        arptable_ = (ARPTable*) TclObject::lookup(argv[2]);
                        assert(arptable_);
                        return TCL_OK;
                }
		if (strcmp(argv[1], "mac") == 0) {
			mac_ = (newMac*) TclObject::lookup(argv[2]);
                        assert(mac_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ifq") == 0) {
			ifq_ = (Queue*) TclObject::lookup(argv[2]);
                        assert(ifq_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "up-target") == 0) {
			uptarget_ = (NsObject*) TclObject::lookup(argv[2]);
                        assert(uptarget_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "down-target") == 0) {
			downtarget_ = (NsObject*) TclObject::lookup(argv[2]);
                        assert(downtarget_);
			return (TCL_OK);
		}
	}
	else if (argc == 2) {
#if !defined(MONARCH)
		//if (strcmp(argv[1], "peerLL") == 0) {
		//tcl.resultf("%s", peerLL_->name());
		//return (TCL_OK);
		//}
#endif
		if (strcmp(argv[1], "mac") == 0) {
			tcl.resultf("%s", mac_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ifq") == 0) {
			tcl.resultf("%s", ifq_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "down-target") == 0) {
			tcl.resultf("%s", downtarget_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "up-target") == 0) {
			tcl.resultf("%s", uptarget_->name());
			return (TCL_OK);
		}
	}
	return LinkDelay::command(argc, argv);
}



void
newLL::recv(Packet* p, Handler*)
{
	char *mh = (char*) HDR_MAC(p);
	hdr_cmn *ch = HDR_CMN(p);
	/*
	 * Sanity Check
	 */
	assert(initialized());

	if(ch->direction() == 1) {
		//p->incoming = 0;
		if(mac_->hdr_type(mh) == ETHERTYPE_ARP)
			arptable_->arpinput(p, this);
		else
			sendUp(p);
	}
	else {
#if 0
                /*
		 * Stamp the packet with the "optimal" number of hops.
		 */
		if(ch->num_forwards() == 0)
			God::instance()->sentPacket(p);
#endif
		ch->direction() = -1;
		sendDown(p);
	}
}


void
newLL::sendUp(Packet* p)
{
	Scheduler& s = Scheduler::instance();

        /*
         * No errored packets should be be passed up by the MAC
         * Layer.
         */
        assert(HDR_CMN(p)->error() == 0);

	if (HDR_CMN(p)->error() > 0) {
		drop(p);
	}
	else {
		s.schedule(uptarget_, p,
			   mindelay_ / 2 + delay_ * Random::uniform());
	}
	return;
}



void
newLL::sendDown(Packet* p)
{	
	Scheduler& s = Scheduler::instance();
	char *mh = (char*)p->access(off_newmac_);
	newhdr_ll *llh = (newhdr_ll*)p->access(off_newll_);

	llh->seqno_ = ++seqno_;
	llh->lltype() = LL_DATA;

	mac_->hdr_src(mh, mac_->addr());
	mac_->hdr_type(mh, ETHERTYPE_IP);

	if(arptable_->arpresolve(p, this) == 0) {
              s.schedule(downtarget_, p,
			 mindelay_ / 2 + delay_ * Random::uniform());
	}
}


void newLL::dump()
{
}



