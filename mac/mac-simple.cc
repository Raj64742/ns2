//
// Copyright (c) 2003 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.

#include "ll.h"
#include "mac.h"
#include "mac-simple.h"
#include "random.h"

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "agent.h"
#include "basetrace.h"

#include "cmu-trace.h"

static class MacSimpleClass : public TclClass {
public:
	MacSimpleClass() : TclClass("Mac/Simple") {}
	TclObject* create(int, const char*const*) {
		return new MacSimple();
	}
} class_macsimple;


// Added by Sushmita to support event tracing (singal@nunki.usc.edu).
void MacSimple::trace_event(char *eventtype, Packet *p)
{
	if (et_ == NULL) return;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();

	hdr_ip *iph = hdr_ip::access(p);
	char *src_nodeaddr =
		Address::instance().print_nodeaddr(iph->saddr());
	char *dst_nodeaddr =
		Address::instance().print_nodeaddr(iph->daddr());

	if (wrk != 0) 
	{
		sprintf(wrk, "E -t "TIME_FORMAT" %s %s %s",
			et_->round(Scheduler::instance().clock()),
			eventtype,
			src_nodeaddr,
			dst_nodeaddr);
	}
	if (nwrk != 0)
	{
		sprintf(nwrk, "E -t "TIME_FORMAT" %s %s %s",
		et_->round(Scheduler::instance().clock()),
		eventtype,
		src_nodeaddr,
		dst_nodeaddr);
	}
	et_->dump();
}

MacSimple::MacSimple() : Mac() {
	rx_state_ = tx_state_ = MAC_IDLE;
	tx_active_ = 0;
	waitTimer = new MacSimpleWaitTimer(this);
	sendTimer = new MacSimpleSendTimer(this);
	recvTimer = new MacSimpleRecvTimer(this);
	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	et_ = new EventTrace();
	busy_ = 0;
}

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
int 
MacSimple::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if(strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return Mac::command(argc, argv);
}

void MacSimple::recv(Packet *p, Handler *h) {

	struct hdr_cmn *hdr = HDR_CMN(p);
	/* let MacSimple::send handle the outgoing packets */
	if (hdr->direction() == hdr_cmn::DOWN) {
		send(p,h);
		return;
	}

	/* handle an incoming packet */

	/*
	 * If we are transmitting, then set the error bit in the packet
	 * so that it will be thrown away
	 */

	if (tx_active_)
	{
		hdr->error() = 1;

	}

	

	/*
	 * check to see if we're already receiving a different packet
	 */
	
	if (rx_state_ == MAC_IDLE) {
		/*
		 * We aren't already receiving any packets, so go ahead
		 * and try to receive this one.
		 */
		rx_state_ = MAC_RECV;
		pktRx_ = p;
		/* schedule reception of the packet */
		recvTimer->start(txtime(p));
	} else {
		/*
		 * We are receiving a different packet, so decide whether
		 * the new packet's power is high enough to notice it.
		 */
		if (pktRx_->txinfo_.RxPr / p->txinfo_.RxPr
			>= p->txinfo_.CPThresh) {
			/* power too low, ignore the packet */
			Packet::free(p);
		} else {
			/* power is high enough to result in collision */
			rx_state_ = MAC_COLL;

			/*
			 * look at the length of each packet and update the
			 * timer if necessary
			 */

			if (txtime(p) > recvTimer->expire()) {
				recvTimer->stop();
				Packet::free(pktRx_);
				pktRx_ = p;
				recvTimer->start(txtime(pktRx_));
			} else {
				Packet::free(p);
			}
		}
	}
}


double
MacSimple::txtime(Packet *p)
 {
	 struct hdr_cmn *ch = HDR_CMN(p);
	 double t = ch->txtime();
	 if (t < 0.0)
	 	t = 0.0;
	 return t;
 }



void MacSimple::send(Packet *p, Handler *h)
{
	hdr_cmn* ch = HDR_CMN(p);

	/* store data tx time */
 	ch->txtime() = Mac::txtime(ch->size());

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	trace_event("SENSING_CARRIER",p);

	/* check whether we're idle */
	if (tx_state_ != MAC_IDLE) {
		// already transmitting another packet .. drop this one
		// Note that this normally won't happen due to the queue
		// between the LL and the MAC .. the queue won't send us
		// another packet until we call its handler in sendHandler()

		Packet::free(p);
		return;
	}

	pktTx_ = p;
	txHandler_ = h;
	// rather than sending packets out immediately, add in some
	// jitter to reduce chance of unnecessary collisions
	double jitter = Random::random()%40 * 100/bandwidth_;

	if(rx_state_ != MAC_IDLE) {
		trace_event("BACKING_OFF",p);
	}

	if (rx_state_ == MAC_IDLE ) {
		// we're idle, so start sending now
		waitTimer->start(jitter);
		sendTimer->start(jitter + ch->txtime());
	} else {
		// we're currently receiving, so schedule it after
		// we finish receiving
		waitTimer->start(jitter);
		sendTimer->start(jitter + ch->txtime()
				 + HDR_CMN(pktRx_)->txtime());
	}
}


void MacSimple::recvHandler()
{
	hdr_cmn *ch = HDR_CMN(pktRx_);
	Packet* p = pktRx_;
	MacState state = rx_state_;
	pktRx_ = 0;

	busy_ = 0;

	rx_state_ = MAC_IDLE;

	if (tx_active_) {
		// we are currently sending, so discard packet
		Packet::free(p);
	} else if (state == MAC_COLL) {
		// recv collision, so discard the packet
		drop(p, DROP_MAC_COLLISION);
		//Packet::free(p);
	} else if (ch->error()) {
		// packet has errors, so discard it
		Packet::free(p);
	} else {
		uptarget_->recv(p, (Handler*) 0);
	}
}

void MacSimple::waitHandler()
{
	tx_state_ = MAC_SEND;
	tx_active_ = 1;

	downtarget_->recv(pktTx_, txHandler_);
}

void MacSimple::sendHandler()
{
	Handler *h = txHandler_;
	Packet *p = pktTx_;

	pktTx_ = 0;
	txHandler_ = 0;
	tx_state_ = MAC_IDLE;
	tx_active_ = 0;

	busy_ = 1;

	// I have to let the guy above me know I'm done with the packet
	h->handle(p);
}




//  Timers


void MacSimpleTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);
	busy_ = 1;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}

void MacSimpleTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);
	s.cancel(&intr);
	
	busy_ = 0;
	stime = rtime = 0.0;
}


void MacSimpleWaitTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->waitHandler();
}

void MacSimpleSendTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->sendHandler();
}

void MacSimpleRecvTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->recvHandler();
}


