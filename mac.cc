/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/mac.cc,v 1.35 1999/10/14 22:19:26 yuriy Exp $ (UCB)";
#endif

//#include "classifier.h"

#include <channel.h>
#include <mac.h>


int hdr_mac::offset_;

static class MacHeaderClass : public PacketHeaderClass {
public:
	MacHeaderClass() : PacketHeaderClass("PacketHeader/Mac",
					     sizeof(hdr_mac)) {
		bind_offset(&hdr_mac::offset_);
	}
	void export_offsets() {
		field_offset("macSA_", OFFSET(hdr_mac, macSA_));
		field_offset("macDA_", OFFSET(hdr_mac, macDA_));
	}
} class_hdr_mac;

static class MacClass : public TclClass {
public:
	MacClass() : TclClass("Mac") {}
	TclObject* create(int, const char*const*) {
		return (new Mac);
	}
} class_mac;


void
MacHandlerResume::handle(Event*)
{
	mac_->resume();
}

void
MacHandlerSend::handle(Event* e)
{
	mac_->sendDown((Packet*)e);
}

/* =================================================================
   Mac Class Functions
  ==================================================================*/
static int MacIndex = 0;

Mac::Mac() : BiConnector(), netif_(0), tap_(0), ll_(0), channel_(0), callback_(0), hRes_(this), hSend_(this), state_(MAC_IDLE), pktRx_(0), pktTx_(0)
{
	index_ = MacIndex++;
//	bandwidth_ = 2.0 * 1e6;
	bind_bw("bandwidth_", &bandwidth_);
	off_mac_ = hdr_mac::offset_;
	bind_time("delay_", &delay_);
	//bind("off_mac_", &off_mac_);
}


int Mac::command(int argc, const char*const* argv)
{
	if(argc == 2) {
		Tcl& tcl = Tcl::instance();
		
		if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", addr());
			return TCL_OK;
		}
		else if (strcmp(argv[1], "channel") == 0) {
			tcl.resultf("%s", channel_->name());
			return (TCL_OK);
		}
		/*else if (strcmp(argv[1], "classifier") == 0) {
		  tcl.resultf("%s", mcl_->name());
		  return (TCL_OK);
		  }
		  else if (strcmp(argv[1], "maclist") == 0) {
		  tcl.resultf("%s", macList_->name());
		  return (TCL_OK);
		  }*/
	}
	else if (argc == 3) {
		TclObject *obj;
		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		// if (strcmp(argv[1], "channel") == 0) {
// 			channel_ = (Channel*) obj;
// 			return (TCL_OK);
// 		}
		else if (strcmp(argv[1], "netif") == 0) {
			netif_ = (Phy*) obj;
			return TCL_OK;
		}
		// else if (strcmp(argv[1], "up-target") == 0) {
		// uptarget_ = (NsObject*) obj;
		// return TCL_OK;
		//}
		/* else if (strcmp(argv[1], "down-target") == 0) {
		   downtarget_ = (NsObject*) obj;
		   return TCL_OK;
		   }*/
		/*else if (strcmp(argv[1], "classifier") == 0) {
			mcl_ = (Classifier*) obj;
			return (TCL_OK);
			}*/
		/*if (strcmp(argv[1], "maclist") == 0) {
		  macList_ = (Mac*) obj;
		  return (TCL_OK);
		  }*/
	}
	
	return BiConnector::command(argc, argv);
}


void Mac::recv(Packet* p, Handler* h)
{
	if (hdr_cmn::access(p)->direction() == hdr_cmn::UP) {
		sendUp(p);
		return;
	}

	callback_ = h;
	hdr_mac* mh = HDR_MAC(p);
	mh->set(MF_DATA, index_);
	state(MAC_SEND);
	sendDown(p);
}

void Mac::sendUp(Packet* p) 
{
	char* mh = (char*)p->access(hdr_mac::offset_);
	int dst = this->hdr_dst(mh);
	
	state(MAC_IDLE);
	if (((u_int32_t)dst != MAC_BROADCAST) && (dst != index_)) {
		drop(p);
		return;
	}
	Scheduler::instance().schedule(uptarget_, p, \
				       delay_);
}



void Mac::sendDown(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	double txt = txtime(p);
	downtarget_->recv(p, this);
	s.schedule(&hRes_, &intr_, txt);
}


void Mac::resume(Packet* p)
{
	if (p != 0)
		drop(p);
	state(MAC_IDLE);
	callback_->handle(&intr_);
}


//Mac* Mac::getPeerMac(Packet* p)
//{
//return (Mac*) mcl_->slot(hdr_mac::access(p)->macDA());
//}
