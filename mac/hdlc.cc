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
 * Contributed by the Daedalus Research Group, http://daedalus.cs.berkeley.edu
 */

#include <hdlc.h>
#include <mac.h>


int HDLC::uidcnt_;

static class HDLCClass : public TclClass {
public:
	HDLCClass() : TclClass("LL/HDLC") {}
	TclObject* create(int, const char*const*) {
		return (new HDLC);
	}
} class_hdlc;

void HdlcTimer::expire(Event *)
{
	(*a_.*callback_)();
	
}


HDLC::HDLC() : LL(), highest_ack_(-1), t_seqno_(0), seqno_(0), maxseq_(0), closed_(0), nrexmit_(0), ntimeouts_(0), disconnect_(0), sentDISC_(0), recv_seqno_(-1), SABME_req_(0), sentREJ_(0),  save_(NULL),  rtx_timer_(this, &HDLC::timeout), delay_timer_(this, &HDLC::delayTimeout), next_(0), maxseen_(0)
{
	bind("window_size_", &wnd_);
	bind("queue_size_", &queueSize_);
	bind("timeout_", &timeout_);
	bind("max_timeouts_", &maxTimeouts_);
	bind("delAck_", &delAck_);
	bind("delAckVal_", &delAckVal_);
	bind("selRepeat_", &selRepeat_);
	
	wndmask_ = HDLC_MWM;
	seen_ = new Packet*[(HDLC_MWM+1)];
	memset(seen_, 0, (sizeof(Packet *) * (HDLC_MWM+1)));
}


void HDLC::recv(Packet* p, Handler* h)
{
	hdr_cmn *ch = HDR_CMN(p);

	/*
	 * Sanity Check
	 */
	assert(initialized());

	// If direction = UP, then HDLC is recv'ing pkt from the network
	// Otherwise, set direction to DOWN and pass it down the stack
	if (ch->direction() == hdr_cmn::UP) {
		if (ch->ptype_ == PT_ARP)
			arptable_->arpinput(p, this);
		else 
			recvIncoming(p);
		// uptarget_ ? sendUp(p) : drop(p);
		return;
	}

	ch->direction() = hdr_cmn::DOWN;
	recvOutgoing(p);
}

// Functions for sending packets out to network

void HDLC::recvOutgoing(Packet* p)
{
	//if (disconnect_) {
		
		// if (!sentDISC_) {
// 			sendUA(p, DISC);
// 			sentDISC_ = 1;
// 			set_rtx_timer();
// 			drop(p);
			
// 			return;
// 		}
// 		drop(p);
// 		return;
// 	}
	
	if (!SABME_req_ && t_seqno_ == 0) {
		// this is the first pkt being sent
		// send out SABME request to start connection
		
		sendUA(p, SABME);
		
		// resolve mac address and send
		//sendDown(np);
		SABME_req_ = 1;
		
		// set some timer for SABME?
		set_rtx_timer();
		
	}
        
        // place pkt in outgoing queue
	inSendBuffer(p);
	
	// skip sending the first pkt
	// which should be sent only after recving UA
	// in reply to SABME request
	if (t_seqno_ > 0) 
		sendMuch();
	
}


void HDLC::inSendBuffer(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	hdr_hdlc *hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);
	
	nsaddr_t src = (nsaddr_t)Address::instance().get_nodeaddr(ih->saddr());
	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());
	
	hh->fc_type_ = HDLC_I_frame;
	hh->saddr_ = src;
	hh->daddr_ = dst;
	
	ifr->send_seqno = seqno_++;
	
	ch->size() += HDLC_HDR_LEN;
	
	sendBuf_.enque(p);
	
}

//Packet *HDLC::dataToSend(Packet *p)
Packet *HDLC::dataToSend()
{
	Packet *dp;
	//hdr_ip *ih = HDR_IP(p);
	//nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->saddr());
	
	// if have any data to send
	if (t_seqno_ <= highest_ack_ + wnd_ && (dp = getPkt(sendBuf_, t_seqno_)) != 0) {
		
		// check if the destinations match for data and RR
		//if (((nsaddr_t)Address::instance().get_nodeaddr(HDR_IP(p)->daddr())) == dst) {
		Packet *np = dp->copy();
		return np;
		//}
	}
	return NULL;
}



// try to send as much as possible if have any pkts to send
void HDLC::sendMuch()
{
	Packet *p;
	
	while (t_seqno_ <= highest_ack_ + wnd_ && (p = getPkt(sendBuf_, t_seqno_)) != 0) {
		Packet *dp = p->copy();
		output(dp, t_seqno_);
		t_seqno_++;
		
	} 
	
}


void HDLC::output(Packet *p, int seqno)
{
	int force_set_rtx_timer = 0;
	
	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);

	// piggyback the last seqno recvd
	if (selRepeat_)
		ifr->recv_seqno = next_;
	else
		ifr->recv_seqno = recv_seqno_;

	// cancel the delay timer if pending
	if (delay_timer_.status() == TIMER_PENDING) 
		delay_timer_.cancel();
	if (save_ != NULL) {
                Packet::free(save_);
                save_ = NULL;
        }
	
	sendDown(p);
	
	// if no outstanding data set rtx timer again
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	if (seqno > maxseq_) 
		maxseq_ = seqno;
	else if (seqno < maxseq_)
		++nrexmit_;
	
	if (!(rtx_timer_.status() == TIMER_PENDING) || force_set_rtx_timer)

		// No timer pending.  Schedule one.
		set_rtx_timer();
}


void HDLC::sendUA(Packet *p, COMMAND_t cmd)
{
	Packet *np = Packet::alloc();
	
	hdr_cmn *ch = HDR_CMN(p);
	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *ih = HDR_IP(p);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc* nhh = HDR_HDLC(np);
	struct U_frame* uf = (struct U_frame *)&(nhh->hdlc_fc_);
	
	nsaddr_t src = (nsaddr_t)Address::instance().get_nodeaddr(ih->saddr());
	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());

        // setup common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	//nih->daddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_U_frame;

	switch(cmd) {
		
	case SABME:
	case DISC:
		// use same dst and src address
		nih->daddr() = ih->daddr();
		nih->saddr() = ih->saddr();
		nhh->daddr_ = dst;
		nhh->saddr_ = src;
		break;
		
	case UA:
		// use reply mode; src and dst get reversed
		nih->daddr() = ih->saddr();
		nih->saddr() = ih->daddr();
		nhh->daddr_ = src;
		nhh->saddr_ = dst;
		break;
		
	default:
		fprintf(stderr, "Unknown type of U frame\n");
		exit(1);
	}
	
	uf->utype = cmd;
	sendDown(np);
	
	//return (np);
}


void HDLC::sendDISC(Packet *p)
{
	sendUA(p, DISC);
	
}

void HDLC::delayTimeout()
{
	// The delay timer expired so we ACK the last pkt seen
	if (save_ != NULL) {
		Packet* pkt = save_;
		ack(pkt);
		save_ = NULL;
		Packet::free(pkt);
	}
}

void HDLC::ack(Packet *p)
//void HDLC::ack()
{
	Packet *dp;
	
	//if ((dp = dataToSend(p)) != NULL) {
	if ((dp = dataToSend()) != NULL) {
		output(dp, t_seqno_);	
		t_seqno_++;
		
	} else {
		
		sendRR(p);
	}
	
}


void HDLC::sendRR(Packet *p)
{
	Packet *np = Packet::alloc();
	
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame*)&(nhh->hdlc_fc_);
	

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;

	if (selRepeat_)
		sf->recv_seqno = next_;
	else
		sf->recv_seqno = recv_seqno_;
	
	sf->stype = RR;
	
	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);
	
}

void HDLC::sendRNR(Packet *p)
{}

void HDLC::sendREJ(Packet *p)
{
	Packet *np = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame *)&(nhh->hdlc_fc_);
	

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;
	sf->recv_seqno = recv_seqno_;
	sf->stype = REJ;
	
	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);
	
}

void HDLC::sendSREJ(Packet *p, int seq)
{
	Packet *np = Packet::alloc();

	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	struct hdr_hdlc *hh = HDR_HDLC(p);

	hdr_cmn *nch = HDR_CMN(np);
	hdr_ip *nih = HDR_IP(np);
	struct hdr_hdlc *nhh = HDR_HDLC(np);
	struct S_frame *sf = (struct S_frame *)&(nhh->hdlc_fc_);

	// common hdr
	nch->addr_type() = ch->addr_type();
	nch->uid() = uidcnt_++;
	nch->ptype() = PT_HDLC;
	nch->size() = HDLC_HDR_LEN;
	nch->error() = 0;
	nch->iface() = -2;
	
	nih->daddr() = ih->saddr();
	nih->saddr() = ih->daddr();
	
	nhh->fc_type_ = HDLC_S_frame;

	sf->recv_seqno = seq;
	sf->stype = SREJ;

	nhh->saddr_ = hh->daddr();
	nhh->daddr_ = hh->saddr();

	sendDown(np);

}

void HDLC::sendDown(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	hdr_ip *ih = HDR_IP(p);
	
	nsaddr_t dst = (nsaddr_t)Address::instance().get_nodeaddr(ih->daddr());
	char *mh = (char*)p->access(hdr_mac::offset_);
	
	mac_->hdr_src(mh, mac_->addr());
	mac_->hdr_type(mh, ETHERTYPE_IP);
	int tx = 0;
	
	switch(ch->addr_type()) {

	case NS_AF_ILINK:
		mac_->hdr_dst((char*) HDR_MAC(p), ch->next_hop());
		break;

	case NS_AF_INET:
		dst = ch->next_hop();
		/* FALL THROUGH */
		
	case NS_AF_NONE:
		
		if (IP_BROADCAST == (u_int32_t) dst)
		{
			mac_->hdr_dst((char*) HDR_MAC(p), MAC_BROADCAST);
			break;
		}
		
                /* Assuming arptable is present, send query */
		if (arptable_) {
			tx = arptable_->arpresolve(dst, p, this);
			break;
		}
		
		/* FALL THROUGH */

	default:
		
		int IPnh = (lanrouter_) ? lanrouter_->next_hop(p) : -1;
		if (IPnh < 0)
			mac_->hdr_dst((char*) HDR_MAC(p),macDA_);
		else if (varp_)
			tx = varp_->arpresolve(IPnh, p);
		else
			mac_->hdr_dst((char*) HDR_MAC(p), IPnh);
		break;
	}
	
	if (tx == 0) {
		Scheduler& s = Scheduler::instance();
		// let mac decide when to take a new packet from the queue.
		s.schedule(downtarget_, p, delay_);
	}
}



// functions for receiving pkts form network

void HDLC::recvIncoming(Packet* p)
{
	// This pkt is coming from the network
	// check if pkt has error

	if (hdr_cmn::access(p)->error() > 0)
		drop(p);
	else {
		hdr_hdlc* hh = HDR_HDLC(p);
		switch (hh->fc_type_) {
		case HDLC_I_frame:
			recvIframe(p);
			break;
		case HDLC_S_frame:
			recvSframe(p);
			break;
		case HDLC_U_frame:
			recvUframe(p);
			break;
		default:
			fprintf(stderr, "Invalid HDLC control type\n");
			exit(1);
		}
	}
	
}

// recv data pkt
void HDLC::recvIframe(Packet *p) 
{
	struct I_frame* ifr = (struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_);
	int rseq = ((struct I_frame*)&(HDR_HDLC(p)->hdlc_fc_))->recv_seqno;
	
	// if (closed_) {
// 		drop(p);
// 		return;
// 	}

	// check if any ack is piggybacking
	// and update highest_ack_
	if (rseq > -1)  // valid ack
		handlePiggyAck(p);
	
	// recvd first data pkt
	if (ifr->send_seqno == 0 && recv_seqno_ == -1) 
		recv_seqno_ = 0;

	if (selRepeat_) // do selective repeat
	
		selectiveRepeatMode(p);
	else
		// default to go back N
		goBackNMode(p);
		
	
	// if we had got a valid piggy ack
	// see if we can send more
	if (rseq > -1)  
		sendMuch();
	
}

void HDLC::recvSframe(Packet* p)
{
	hdr_hdlc* hh = HDR_HDLC(p);
	struct S_frame *sf = (struct S_frame*)&(hh->hdlc_fc_);
	
	
	switch(sf->stype)
	{
	case RR:
		handleRR(p);
		break;
		
	case REJ:
		handleREJ(p);
		break;
		
	case RNR:
		handleRNR(p);
		break;
		
	case SREJ:
		handleSREJ(p);
		break;
		
	default:
		fprintf(stderr, "Unknown type of S frame\n");
		exit(0);
		
	}
	
}

void HDLC::recvUframe(Packet* p)
{
	// U frames supported for now are ABME/UA/DISC
	hdr_hdlc* hh = HDR_HDLC(p);
	struct U_frame *uf = (struct U_frame*)&(hh->hdlc_fc_);
	
	switch(uf->utype) 
	{
	case SABME:
		handleSABMErequest(p);
		break;
		
	case UA:
		handleUA(p);
		break;
		
	case DISC:
		handleDISC(p);
		break;
		
	default:
		fprintf(stderr, "Unknown type of U frame\n");
		exit(1);
	}
	
}

void HDLC::handleSABMErequest(Packet *p)
{
        // got a request to open a connection
        // ack back an UA
	if (recv_seqno_ == -1) {
		sendUA(p, UA);
		//closed_ = 0;
	}

	Packet::free(p);
}


void HDLC::handleDISC(Packet *p)
{
	// got request for disconnect
	// send UA 
	reset();
	sendUA(p, UA);
	Packet::free(p);

}

void HDLC::handleUA(Packet *p)
{
	// recv ok for connection
	// if waiting to send, start sending
	// ?? shouldn't I match the addresses here??
	if (t_seqno_ == 0) {
		// cancel the SABME timer??
		sendMuch();
		//closed_ = 0;
		
	} else { // have I sent a DISCONNECT?
		if (disconnect_) {
			// recvd confirmation on disconnect
			reset();
			// cancel if timer is running
			cancel_rtx_timer();
		}
	}
	Packet::free(p);
	
}


void HDLC::handlePiggyAck(Packet *p)
{
	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame *ifr =  (struct I_frame*)&(hh->hdlc_fc_);

	int seqno = ifr->recv_seqno - 1;
	if (seqno > highest_ack_) {  // recvd a new ack
		
		Packet *datapkt = getPkt(sendBuf_, seqno);
		sendBuf_.remove(datapkt);
		Packet::free(datapkt);

		// update highest_ack_
		highest_ack_ = seqno;
		
		// set retx_timer
		if (t_seqno_ > seqno || seqno < maxseq_)
			set_rtx_timer();
		
		else
			cancel_rtx_timer();
		
	} else { // got duplicate acks
		// do nothing as piggyback ack
	}
	
	// if had timeouts, should reset it now
	ntimeouts_ = 0;
	
}


// receiver ready - ack for a pkt successfully recvd by receiver, send next pkt pl
void HDLC::handleRR(Packet *p)
{
	// recvd an ack for a pkt
	// remove data pkt from outgoing buffer, if applicable
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(p)->hdlc_fc_);
	int seqno = sf->recv_seqno - 1;
	
	//if (closed_) {
	//drop(p);
	//return;
	//}

	if (seqno > highest_ack_) {  // recvd a new ack
		
		Packet *datapkt = getPkt(sendBuf_, seqno);
		if (datapkt != NULL) {
			sendBuf_.remove(datapkt);
			Packet::free(datapkt);
			
			// update highest_ack_
			highest_ack_ = seqno;
		}
		
		// set retx_timer
		if (t_seqno_ > seqno || seqno < maxseq_)
			set_rtx_timer();
		
		else
			cancel_rtx_timer();
		
	} else { // got duplicate acks
		
		drop(p);
	}
	
	// if had timeouts, should reset it now
	ntimeouts_ = 0;
	
	// try to send more
	Packet::free(p);
	sendMuch();
	
}


Packet *HDLC::getPkt(PacketQueue buffer, int seqno)
{
	Packet *p;
	
	buffer.resetIterator();
	for (p = buffer.getNext(); p != 0; p = buffer.getNext()) {
		
		if (((struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_))->send_seqno == seqno)
			return p;
	}

	return(NULL);
}

// receiver not ready
void HDLC::handleRNR(Packet *p)
{
	// stop sending pkts and wait until RR is recved.
	// wait for how long?
}

// handle reject or a go-back-N request from receiver
void HDLC::handleREJ(Packet *rejp)
{
	// set seqno_ = seqno in REJ
	// and start sending
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(rejp)->hdlc_fc_);
	int seqno = sf->recv_seqno;
	
	t_seqno_ = seqno;
	sendMuch();
	
	Packet::free(rejp);
}


// selective reject or a NACK for a data pkt not recvd from the receiver
void HDLC::handleSREJ(Packet *rejp)
{
	
	struct S_frame *sf = (struct S_frame *)&(HDR_HDLC(rejp)->hdlc_fc_);
	int seqno = sf->recv_seqno;

	// resend only the pkt that was requested
	Packet *p = getPkt(sendBuf_, seqno);
	Packet *dp = p->copy();
	
	output(dp, seqno);
	
	Packet::free(rejp);
	
}





void HDLC::reset()
{
	Packet *p;
	
	// drop pkts in sendBuf
	while ((p = getPkt(sendBuf_, highest_ack_+ 1)) != NULL) 
	{
		sendBuf_.remove(p);
		Packet::free(p);
		maxseq_++;
	}
	       
	SABME_req_ = 0; // allowing newer connection to set up
	t_seqno_ = 0;
	highest_ack_ = -1;
	seqno_ = 0;
	maxseq_ = 0;
	//closed_ = 1;
	nrexmit_ = 0;
	ntimeouts_ = 0;
	recv_seqno_ = -1;
	sentREJ_ = 0;
	sentDISC_ = 0;
	//disconnect_ = 0;
	save_ = NULL;
	 
}

// void HDLC::set_ack_timer()
// {
// 	ack_timer_.sched(DELAY_ACK_VAL);
// }


void HDLC::reset_rtx_timer(int backoff)
{
	if (backoff)
		rtt_backoff();
	set_rtx_timer();
	t_seqno_ = highest_ack_ + 1;
}

void HDLC::set_rtx_timer()
{
	rtx_timer_.resched(rtt_timeout());
}


void HDLC::timeout()
{
	//if (disconnect_) {
	//sentDISC_ = 0;
	//return;
	//}

	ntimeouts_++;
	if (ntimeouts_ > maxTimeouts_) {
		//disconnect_ = 1;
		//sendDISC();
		//reset();
		return;
	}
	
	if (highest_ack_ == maxseq_) {
		// no outstanding data
		// then don't do  anything
		reset_rtx_timer(0);

	} else {
		t_seqno_ = highest_ack_ + 1;
		
		sendMuch();
		
	}
}

double HDLC::rtt_timeout()
{
	return timeout_;
}

void HDLC::rtt_backoff()
{
	// no backoff for now
}


// doing GoBAckN error recovery
void HDLC::goBackNMode(Packet *p)
{
	hdr_cmn *ch = HDR_CMN(p);
	hdr_hdlc* hh = HDR_HDLC(p);
	struct I_frame* ifr = (struct I_frame *)&(hh->hdlc_fc_);
	
	// recv data in order
	if (ifr->send_seqno == recv_seqno_) {
		
		if (sentREJ_)
			sentREJ_ = 0;
		
		recv_seqno_++;
		
                // strip off hdlc hdr
		ch->size() -= HDLC_HDR_LEN;

		// send ack back
		// start a timer to delay the ack so that
		// we can try and piggyback the ack in some data pkt
		if (delAck_) {
			
			if (delay_timer_.status() != TIMER_PENDING) {
				assert(save_ == NULL);
				save_ = p->copy();
				delay_timer_.sched(delAckVal_);
			} 
			
		} else {
			ack(p);
			
		}
		
		uptarget_ ? sendUp(p) : drop(p);

		
	} else if (ifr->send_seqno > recv_seqno_) {

		if (!sentREJ_) {
			sentREJ_ = 1;
			// since GoBackN send REJ for the first
			// out of order pkt
			sendREJ(p);
		}
		
		drop(p, "Pkt out of order");
		
	} else {
		// send_seqno < recv_seqno; duplicate data pkts
		// send ack back as previous RR maybe lost
		ack(p);
		//ack();
		drop(p, "Duplicate pkt");
	}
	
}


// Selective Repeat mode of error recovery
// in case of a missing pkt, send SREJ for that pkt only

void HDLC::selectiveRepeatMode(Packet* p)
{
	hdr_cmn *ch = HDR_CMN(p);
	int seq =  ((struct I_frame *)&(HDR_HDLC(p)->hdlc_fc_))->send_seqno;
	bool just_marked_as_seen = FALSE;
	
	// resize buffers
	// while (seq + 1 - next >= wndmask_) {
	
// 		resize_buffers((wndmask_+1)*2);
// 	}
	
	// strip off hdlc hdr
	ch->size() -= HDLC_HDR_LEN;
	
	if (seq > maxseen_) {
		// the packet is the highest we've seen so far
		int i;
		for (i = maxseen_ + 1; i < seq; i++) {
			sendSREJ(p, i);
		}

		// we record the packets between the old maximum and
		// the new max as being "unseen" i.e. 0 
		maxseen_ = seq;
		
		// place pkt in buffer
		seen_[maxseen_ & wndmask_] = p;
		
		// necessary so this packet isn't confused as being a duplicate
		just_marked_as_seen = TRUE;
		
	}
	
	if (seq < next_) {
		// Duplicate packet case 1: the packet is to the left edge of
		// the receive window; therefore we must have seen it
		// before
		printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
		ack(p);
		drop(p);
		return;
		
	}
	
	int next = next_;
	
	if (seq >= next_ && seq <= maxseen_) {
		// next is the left edge of the recv window; maxseen_
		// is the right edge; execute this block if there are
		// missing packets in the recv window AND if current
		// packet falls within those gaps

		if (seen_[seq & wndmask_] && !just_marked_as_seen) {
		// Duplicate case 2: the segment has already been
		// recorded as being received (AND not because we just
		// marked it as such)
			
			printf("%f\t Received duplicate packet %d\n",Scheduler::instance().clock(),seq);
			ack(p);
			drop(p);
			return;
			
		}

		// record the packet as being seen
		seen_[seq & wndmask_] = p;

		while ( seen_[next & wndmask_] != NULL ) {
			// this loop first gets executed if seq==next;
			// i.e., this is the next packet in order that
			// we've been waiting for.  the loop sets how
			// many pkt we can now deliver to the
			// application, due to this packet arriving
			// (and the prior arrival of any pkts
			// immediately to the right)

			Packet* rpkt = seen_[next & wndmask_];
			seen_[next & wndmask_] = 0;
			
			uptarget_ ? sendUp(rpkt) : drop(rpkt);
			
			++next;
		}

		// store the new left edge of the window
		next_ = next;

		// send ack 
		ack(p);
	}
	
}

			
