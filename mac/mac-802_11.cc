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
 */

#include "template.h"
#include "channel.h"
#include "mac-802_11.h"


static class Mac802_11Class : public TclClass {
public:
	Mac802_11Class() : TclClass("Mac/802_11") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Mac802_11);
	}
} class_mac_802_11;


Mac802_11::Mac802_11() : CsmaCaMac(), mode_(MM_RTS_CTS), sender_(-1), rtxAck_(0), rtxRts_(0), pkt_(0), pktTx_(0), mhRts_(this), mhData_(this), mhIdle_(this)
{
	bind("bssid_", &bssid_);
	bind_time("sifs_", &sifs_);
	bind_time("pifs_", &pifs_);
	bind_time("difs_", &difs_);
	bind("rtxAckLimit_", &rtxAckLimit_);
	bind("rtxRtsLimit_", &rtxRtsLimit_);
}


int
Mac802_11::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "mode") == 0) {
			if (strcmp(argv[2], "DCF") == 0)
				mode_ = MM_DCF;
			else if (strcmp(argv[2], "RTS_CTS") == 0)
				mode_ = MM_RTS_CTS;
			else if (strcmp(argv[2], "PCF") == 0)
				mode_ = MM_PCF;
			return (TCL_OK);
		}
	}
	return CsmaCaMac::command(argc, argv);
}


void
Mac802_11::recv(Packet* p, Handler* h)
{
	if (mode_ == MM_RTS_CTS)
		processRtsCts(p, h);
	else CsmaCaMac::recv(p, h);
}


void
Mac802_11::send(Packet* p, double ifs)
{
	Scheduler& s = Scheduler::instance();
	if (pktTx_ && pktTx_->uid_ > 0) {
		s.cancel(pktTx_);
		Packet::free(pktTx_);
	}
	pktTx_ = p;
	ifs_ = ifs;
	double delay = channel_->txstop() + ifs_ - s.clock();
	if (delay > 0)
		s.schedule(&mhSend_, p, delay);
	else
		CsmaCaMac::send(p);
}


void
Mac802_11::processRtsCts(Packet* p, Handler* h)
{
	Scheduler& s = Scheduler::instance();
	hdr_mac* mh = (hdr_mac*) p->access(off_mac_);
	if (h != 0) {
		callback_ = h;
		mh->macSA() = label_;
		mh->ftype() = MF_DATA;
		mh->duration() = txtime(p);
		pkt_ = p;
		if (state_ == MAC_IDLE && pktTx_ == 0) {
			rtxRts_ = 0;
			sendRts();
		}
		return;
	}

	// process received packets
	switch(mh->ftype()) {
	case MF_RTS:
		if (state_ == MAC_IDLE || state_ == MAC_RTS ||
		    (state_ == MAC_RECV && sender_ == mh->macSA())) {
			sendCts(p);
			return;
		}
		break;
	case MF_CTS:
		if (state_ == MAC_RTS_END) {
			Packet::free(p);
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			sendData();
			return;
		}
		break;
	case MF_DATA:
		sendAck(p);
		target_->recv(p);
		return;
	case MF_ACK:
		if (state_ == MAC_SEND) {
			Packet::free(p);
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			Packet::free(pkt_);
			pkt_ = 0;
			CsmaCaMac::resume();
			return;
		}
		break;
	}
	drop(p);
}


void
Mac802_11::resume()
{
	if (mode_ != MM_RTS_CTS) {
		CsmaCaMac::resume();
		return;
	}

	Scheduler& s = Scheduler::instance();
	pktTx_ = 0;
	if (state_ & MAC_ACK) {
		if (callback_) {
			state(state_ & ~(MAC_ACK | MAC_RECV));
			if (pkt_->uid_ > 0)
				s.cancel(pkt_);
			if (state_ == MAC_SEND)
				sendData();
			else
				sendRts();
		}
		else
			state(MAC_IDLE);
	}
	else if (state_ == MAC_RTS) {
		backoff(&mhRts_, pkt_, difs_ + txtime(hlen_));
		state(MAC_RTS_END);
	}
	else if (state_ == MAC_SEND) {
		backoff(&mhData_, pkt_, difs_ + txtime(hlen_));
		rtxAck_ = 0;
	}
}


double
Mac802_11::lengthNAV(Packet* p)
{		
	if (p == 0)
		return 0;
	hdr_mac* mh = (hdr_mac*)p->access(off_mac_);
	if (mh->ftype() == MF_RTS || mh->ftype() == MF_CTS) {
		double delay = txtime(p);
		delay += (mh->ftype()==MF_RTS) ? delay + 3*sifs_ : 2*sifs_;
		return delay + mh->duration();
	}
	return 0;
}


void
Mac802_11::sendRts()
{
	Scheduler& s = Scheduler::instance();
	state(state_ & ~MAC_RTS_END);
	double delay = lengthNAV(channel_->pkt())+channel_->txstop()-s.clock();
	if (pktTx_ || state_ != MAC_IDLE || delay > 0) {
		backoff(&mhRts_, pkt_, delay);
		return;
	}

	if (++rtxRts_ > rtxRtsLimit_) {
		drop(pkt_);
		CsmaCaMac::resume();
		return;
	}
	Packet* p = pkt_->copy();
	((hdr_cmn*)p->access(off_cmn_))->size() = 0;
	hdr_mac* mh = (hdr_mac*)p->access(off_mac_);
	mh->ftype() = MF_RTS;
	send(p, difs_);
	state(MAC_RTS);
}


void
Mac802_11::sendCts(Packet* p)
{
	Scheduler& s = Scheduler::instance();
//	s.schedule(&mhIdle_, &evIdle_, lengthNAV(p));
	hdr_mac* mh = (hdr_mac*)p->access(off_mac_);
	sender_ = mh->macSA();
	swap(mh->macSA(), mh->macDA());
	mh->ftype() = MF_CTS;
	send(p, sifs_);
	state(MAC_RECV);
}


void
Mac802_11::sendData()
{
	Scheduler& s = Scheduler::instance();
	if (++rtxAck_ > rtxAckLimit_) {
		drop(pkt_);
		CsmaCaMac::resume();
		return;
	}
	Packet* p = pkt_->copy();
	hdr_mac* mh = (hdr_mac*)p->access(off_mac_);
	mh->ftype() = MF_DATA;
	send(p, sifs_);
	state(MAC_SEND);
}


void
Mac802_11::sendAck(Packet* p)
{
	Scheduler& s = Scheduler::instance();
	do {
		p = p->copy();
	} while (p->uid_ > 0);
	((hdr_cmn*)p->access(off_cmn_))->size() = 0;
	hdr_mac* mh = (hdr_mac*)p->access(off_mac_);
	swap(mh->macSA(), mh->macDA());
	mh->ftype() = MF_ACK;
	send(p, sifs_);
	state((state_ & ~MAC_RECV) | MAC_ACK);
}


void MacHandlerRts::handle(Event* e)
{
	mac_->sendRts();
}

void MacHandlerData::handle(Event* e)
{
	mac_->sendData();
}

void MacHandlerIdle::handle(Event* e)
{
	mac_->state(mac_->state() & ~MAC_RECV);
}
