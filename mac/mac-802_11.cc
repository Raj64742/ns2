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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac-802_11.cc,v 1.19 1998/06/27 01:24:03 gnguyen Exp $ (UCB)";
#endif

#include "template.h"
#include "channel.h"
#include "random.h"
#include "mac-802_11.h"

#ifdef DEBUG
#include <iostream.h>
#define CHECK_PKT(p) \
	while (p->uid_ > 0) { \
		cerr << "p->uid_ = " << p->uid_ << "\n"; \
		p = p->copy();	\
	}
#else
#define CHECK_PKT(p)
#endif


static class Mac802_11Class : public TclClass {
public:
	Mac802_11Class() : TclClass("Mac/802_11") {}
	TclObject* create(int, const char*const*) {
		return (new Mac802_11);
	}
} class_mac_802_11;


Mac802_11::Mac802_11() : mode_(MM_RTS_CTS), rtxAck_(0), rtxRts_(0), sender_(-1), pkt_(0), pktTx_(0), hRts_(this), hData_(this), hIdle_(this)
{
	bind("bssId_", &bssId_);
	bind_time("sifs_", &sifs_);
	bind_time("pifs_", &pifs_);
	bind_time("difs_", &difs_);
	bind("rtxAckLimit_", &rtxAckLimit_);
	bind("rtxRtsLimit_", &rtxRtsLimit_);
}


int Mac802_11::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "mode") == 0) {
			mode_ = STR2MM(argv[2]);
			return (TCL_OK);
		}
	}
	return MacCsmaCa::command(argc, argv);
}


void Mac802_11::recv(Packet* p, Handler* h)
{
	if (mode_ == MM_RTS_CTS) {
		if (h != 0) {
			callback_ = h;
			RtsCts_send(p);
		}
		else
			RtsCts_recv(p);
	}
	else MacCsmaCa::recv(p, h);
}


void Mac802_11::transmit(Packet* p, double ifs)
{
	Scheduler& s = Scheduler::instance();
	if (pktTx_ && pktTx_->uid_ > 0) {
		s.cancel(pktTx_);
		Packet::free(pktTx_);
	}
	pktTx_ = p;
	ifs_ = ifs;
	double delay = channel_->txstop() + ifs - s.clock();
	if (delay > 0)
		s.schedule(&hSend_, p, delay);
	else
		send(p);
}


void Mac802_11::RtsCts_send(Packet* p)
{
	hdr_mac* mh = hdr_mac::access(p);
	mh->set(MF_DATA, addr_);
	mh->txtime() = txtime(p);
	pkt_ = p;
	if (state_ == MAC_IDLE && pktTx_ == 0) {
		rtxRts_ = 0;
		sendRts();
	}
}


void Mac802_11::RtsCts_recv(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	hdr_mac* mh = hdr_mac::access(p);

	// process received packets
	switch(mh->ftype()) {
	case MF_RTS:
		if (state_ != MAC_IDLE)	// XXX must be IDLE to send CTS
			break;
		if (eIdle_.uid_ > 0)
			s.cancel(&eIdle_);
		sendCts(p);
		return;
	case MF_CTS:
		if (state_ == MAC_RTS) {
			Packet::free(p);
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			sendData();
			return;
		}
		break;
	case MF_DATA:
		if (eIdle_.uid_ > 0)
			s.cancel(&eIdle_);
		sendAck(p);
		s.schedule(target_, p, delay_);
		return;
	case MF_ACK:
		if (state_ == MAC_SEND) {
			Packet::free(p);
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			Packet::free(pkt_);
			pkt_ = 0;
			MacCsmaCa::resume();
			return;
		}
		break;
	default:
		break;
	}
	drop(p);
}


void Mac802_11::resume(Packet* p)
{
	if (mode_ != MM_RTS_CTS) {
		MacCsmaCa::resume(p);
		return;
	}
	if (p != 0) {
		if (p == pktTx_) {
			Packet::free(p);
			pktTx_ = 0;
		}
		drop(pkt_);
		pkt_ = 0;
		MacCsmaCa::resume();
		return;
	}

	Scheduler& s = Scheduler::instance();
	pktTx_ = 0;
	if (state_ & MAC_ACK) {
		state(state_ & ~(MAC_ACK | MAC_RECV));
		if (callback_) {
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			if (state_ == MAC_SEND)
				sendData();
			else
				sendRts();
		}
	}
	else if (state_ == MAC_RTS)
		backoff(&hRts_, pkt_, difs_ + txtime(hlen_));
	else if (state_ == MAC_SEND)
		backoff(&hData_, pkt_, difs_ + txtime(hlen_));
}


void Mac802_11::backoff(Handler* h, Packet* p, double delay)
{
	if (delay == 0)
		delay = lengthNAV(channel_->pkt());
	MacCsmaCa::backoff(h, p, delay);
}


double Mac802_11::lengthNAV(Packet* p)
{
	if (p == 0)
		return 0;
	double delay = 0;
	hdr_mac* mh = hdr_mac::access(p);
	switch (mh->ftype()) {
	case MF_RTS:
		delay += sifs_ + txtime(hlen_);
	case MF_CTS:
		delay += sifs_ + mh->txtime();
	case MF_DATA:
		delay += sifs_ + txtime(hlen_);
	default:
		break;
	}
	return delay;
}


void Mac802_11::sendRts()
{
	Scheduler& s = Scheduler::instance();
	if (pktTx_ || (state_ & ~MAC_RTS) != MAC_IDLE
	    || channel_->txstop() - s.clock() > 0) {
		backoff(&hRts_, pkt_);
		return;
	}

	if (++rtxRts_ > rtxRtsLimit_) {
		resume(pkt_);
		// sendData();
		return;
	}
	Packet* p = pkt_->copy();
	CHECK_PKT(p);
	((hdr_cmn*)p->access(off_cmn_))->size() = 0;
	hdr_mac* mh = hdr_mac::access(p);
	mh->ftype() = MF_RTS;
	transmit(p, difs_);
	state(MAC_RTS);
}


void Mac802_11::sendCts(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	s.schedule(&hIdle_, &eIdle_, lengthNAV(p));
	hdr_mac* mh = hdr_mac::access(p);
	sender_ = mh->macSA();
	swap(mh->macSA(), mh->macDA());
	mh->ftype() = MF_CTS;
	transmit(p, sifs_);
	state(MAC_RECV);
}


void Mac802_11::sendData()
{
	if (++rtxAck_ > rtxAckLimit_) {
		resume(pkt_);
		return;
	}
	rtxAck_ = 0;
	Packet* p = pkt_->copy();
	CHECK_PKT(p);
	hdr_mac* mh = hdr_mac::access(p);
	mh->ftype() = MF_DATA;
	transmit(p, sifs_);
	state(MAC_SEND);
}


void Mac802_11::sendAck(Packet* p)
{
	p = p->copy();
	CHECK_PKT(p);
	((hdr_cmn*)p->access(off_cmn_))->size() = 0;
	hdr_mac* mh = hdr_mac::access(p);
	swap(mh->macSA(), mh->macDA());
	mh->ftype() = MF_ACK;
	transmit(p, sifs_);
	state((state_ & ~MAC_RECV) | MAC_ACK);
}


void MacHandlerRts::handle(Event*)
{
	mac_->sendRts();
}

void MacHandlerData::handle(Event*)
{
	mac_->sendData();
}

void MacHandlerIdle::handle(Event*)
{
	mac_->state(mac_->state() & ~MAC_RECV);
}
