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
 * 	This product includes software developed by the Daedalus Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/snoop.cc,v 1.5 1997/07/23 05:05:08 kfall Exp $ (UCB)";
#endif

#include "snoop.h"

static class SnoopClass : public TclClass {
 public:
        SnoopClass() : TclClass("LL/Snoop") {}
        TclObject* create(int, const char*const*) {
                return (new Snoop(SNOOP_MAKEHANDLER));
        }
} snoop_class;


Snoop::Snoop(int = 0) : LL(), 
	fstate_(0), lastWin_(0), lastSeen_(-1), lastSize_(0), lastAck_(-1), 
	expNextAck_(0), expDupacks_(0), bufhead_(0), buftail_(0),
	toutPending_(0)
{
	bind("off_snoop_", &off_snoop_);
	bind("off_tcp_", &off_tcp_);
	bind("snoopDisable_", &snoopDisable_);
	bind_time("srtt_", &srtt_);
	bind_time("rttvar_", &rttvar_);
	
	rxmitHandler_ = new SnoopRxmitHandler(this);
	persistHandler_ = new SnoopPersistHandler(this);
	
	for (int i = 0; i < SNOOP_MAXWIND; i++) 
		pkts_[i] = 0;
}


int 
Snoop::command(int argc, const char*const* argv)
{
	return LL::command(argc, argv); // for now, only this much
}

/*
 * Receive a packet from higher layer.  Call snoop_data() if TCP packet and
 * forward it on if it's an ack.
 */
void
Snoop::recv(Packet* p, Handler* h)
{
	if (h == 0) {		// from MAC classifier
		handle((Event *) p);
		return;
	}
	int type = ((hdr_cmn*)p->access(off_cmn_))->ptype();
        /* Put packet (if not ack) in cache after checking, and send it on */
	if (type == PT_TCP)
		snoop_data_(p);
	else if (type == PT_ACK)
		printf("---- %f sending ack %d\n", 
		       Scheduler::instance().clock(),
		       ((hdr_tcp*)p->access(off_tcp_))->seqno());
	callback_ = h;
	LL::recv(p, h);
}

/*
 * Handle a packet received from peer across wireless link.  Check first
 * for packet errors, then call snoop_ack() or pass it up as necessary.
 */
void
Snoop::handle(Event *e)
{
	Packet *p = (Packet *) e;
	int type = ((hdr_cmn*) p->access(off_cmn_))->ptype();
	int prop = SNOOP_PROPAGATE; // by default. propagate ack or packet
	Scheduler& s = Scheduler::instance();

	hdr_ll *llh = (hdr_ll*)p->access(off_ll_);
	if (llh->error()) {
		drop(p);        // drop packet if it's been corrupted
		return;
	}

	if (type == PT_ACK)
		prop = snoop_ack_(p);
	else {			// received data, send it up
		int seq = ((hdr_tcp*)p->access(off_tcp_))->seqno();
		printf("---- %f recd packet %d\n", s.clock(), seq);
	}
		
	if (prop == SNOOP_PROPAGATE)
		s.schedule(recvtarget_, e, delay_);
	else			// suppress ack
		Packet::free(p);
	
}

/*
 * Data packet processing.  p is guaranteed to be of type PT_TCP when 
 * this function is called.
 */
void
Snoop::snoop_data_(Packet *p)
{
	Scheduler &s = Scheduler::instance();
	int seq = ((hdr_tcp*)p->access(off_tcp_))->seqno();
	int size = ((hdr_cmn*)p->access(off_cmn_))->size();
	printf("snoop_data_: %f sending packet %d\n", s.clock(), seq);
	if (fstate_ & SNOOP_FULL) {
		if (seq > lastSeen_) {
			lastSeen_ = seq;
			lastSize_ = size;
		}
		return;
	}
	int resetPending = insert_(p);
	if (toutPending_ && resetPending == SNOOP_TAIL) {
		s.cancel(toutPending_);
		toutPending_ = 0;
	}
	if (!toutPending_ && !empty_()) {
		toutPending_ = (Event *) (pkts_[buftail_]);
		s.schedule(rxmitHandler_, toutPending_, timeout_());
	}
	return;
}

/* 
 * insert_() does all the hard work for snoop_data(). It traverses the 
 * snoop cache and looks for the right place to insert this packet (or
 * determines if its already been cached). It then decides whether
 * this is a packet in the normal increasing sequence, whether it
 * is a sender-rexmitted-but-lost-due-to-congestion (or network 
 * out-of-order) packet, or if it is a sender-rexmitted packet that
 * was buffered by us before.
 */
int
Snoop::insert_(Packet *p)
{
	int i, seq = ((hdr_tcp*)p->access(off_tcp_))->seqno(), retval=0;
	int size = ((hdr_cmn*)p->access(off_cmn_))->size();

	if (seq <= lastAck_)
		return retval;
	if (seq > lastSeen_) {	// fast path in common case
		i = bufhead_;
		bufhead_ = next(bufhead_);
	} else if (seq < ((hdr_snoop*)pkts_[buftail_]->access
			  (off_snoop_))->seqno()) {
		buftail_ = prev(buftail_);
		i = buftail_;
	} else {
		for (i = buftail_; i != bufhead_; i = next(i)) {
			hdr_snoop *sh = ((hdr_snoop*)pkts_[i]->access
					 (off_snoop_));
			if (sh->seqno() == seq) {
				sh->numRxmit() = 0;
				sh->senderRxmit() = 1;
				sh->sndTime() = Scheduler::instance().clock();
				return SNOOP_TAIL;
			} else if (sh->seqno() > seq) {
				Packet *temp = pkts_[prev(buftail_)];
				for (int j = buftail_; j != i; j = next(j)) 
					pkts_[prev(j)] = pkts_[j];
				i = prev(i);
				pkts_[i] = temp;
				buftail_ = prev(buftail_);
				break;
			}
		}
	}
	savepkt_(p, seq, i);
	if (bufhead_ == buftail_)
		fstate_ |= SNOOP_FULL;
	/* 
	 * If we have one of the following packets:
	 * 1. a network-out-of-order packet, or
	 * 2. a fast rxmit packet, or 3. a sender retransmission 
	 * AND it hasn't already been buffered, 
	 * then seq will be < lastSeen_. 
	 * We mark this packet as having been due to a sender rexmit 
	 * and use this information in snoop_ack(). We let the dupacks
	 * for this packet go through according to expDupacks_.
	 */
	if (seq < lastSeen_) { /* not in-order -- XXX should it be <= ? */
		if (buftail_ == i) {
			hdr_snoop *sh = ((hdr_snoop*)pkts_[i]->access
					 (off_snoop_));
			sh->senderRxmit() = 1;
			sh->numRxmit() = 0;
		}
		expNextAck_ = buftail_;
		retval = SNOOP_TAIL;
	} else {
		lastSeen_ = seq;
		lastSize_ = size;
	}
	return retval;
}

void
Snoop::savepkt_(Packet *p, int seq, int i)
{
	pkts_[i] = p->copy();
	Packet *pkt = pkts_[i];
	hdr_snoop *sh = (hdr_snoop *) pkt->access(off_snoop_);
	sh->seqno() = seq;
	sh->numRxmit() = 0;
	sh->senderRxmit() = 0;
	sh->sndTime() = Scheduler::instance().clock();
}

/*
 * Ack processing in snoop protocol.  We know for sure that this is an ack.
 * Return SNOOP_SUPPRESS if ack is to be suppressed and SNOOP_PROPAGATE o.w.
 */
int
Snoop::snoop_ack_(Packet *p)
{
	Packet *pkt;

	int ack = ((hdr_tcp*)p->access(off_tcp_))->seqno();
//	int win = ((hdr_tcp*)p->access(off_tcp_))->win();
printf("snoop_ack_: %f got ack %d\n", Scheduler::instance().clock(), ack);
	/*
	 * There are 3 cases:
	 * 1. lastAck_ > ack.  In this case what has happened is
	 *    that the acks have come out of order, so we don't
	 *    do any local processing but forward it on.
	 * 2. lastAck_ == ack.  This is a duplicate ack. If we have
	 *    the packet we resend it, and drop the dupack.
	 *    Otherwise we never got it from the fixed host, so we
	 *    need to let the dupack get through.
	 *    Set expDupacks_ to number of packets already sent
	 *    This is the number of dup acks to ignore.
	 * 3. lastAck_ < ack.  Set lastAck_ = ack, and update
	 *    the head of the buffer queue. Also clean up buffers of ack'd
	 *    packets.
	 */
	if (fstate_ & SNOOP_CLOSED || lastAck_ > ack)
		return SNOOP_PROPAGATE;	// send ack onward
	if (lastAck_ == ack) {	
		/* A duplicate ack; pure window updates don't occur in ns. */
		pkt = pkts_[buftail_];
		if (pkt == 0)
			return SNOOP_PROPAGATE;
		hdr_snoop *sh = (hdr_snoop *)pkt->access(off_snoop_);
		if (pkt == 0 || sh->seqno() > ack + 1) 
			/* don't have packet, letting thru' */
			return SNOOP_PROPAGATE;
		/* 
		 * We have the packet: one of 3 possibilities:
		 * 1. We are not expecting any dupacks (expDupacks_ == 0)
		 * 2. We are expecting dupacks (expDupacks_ > 0)
		 * 3. We are in an inconsistent state (expDupacks_ == -1)
		 */
		if (expDupacks_ == 0) {	// not expecting it
#define RTX_THRESH 1
			static int thresh = 0;
			if (thresh++ < RTX_THRESH) {
				/* no action if under RTX_THRESH */
				return SNOOP_PROPAGATE;
			}
			thresh = 0;
			if (sh->senderRxmit()) 
				return SNOOP_PROPAGATE;
			/*
			 * Otherwise, not triggered by sender.  If this is
			 * the first dupack recd., we must determine how many
			 * dupacks will arrive that must be ignored, and also
			 * rexmit the desired packet.  Note that expDupacks_
			 * will be -1 if we miscount for some reason.
			 */
			expDupacks_ = bufhead_ - expNextAck_;
			if (expDupacks_ < 0)
				expDupacks_ += SNOOP_MAXWIND;
			expDupacks_ -= RTX_THRESH + 1;
			expNextAck_ = next(buftail_);
			if (sh->numRxmit() == 0)
				snoop_rxmit(pkt);
			return SNOOP_SUPPRESS;
		} else if (expDupacks_ > 0) {
			expDupacks_--;
			return SNOOP_SUPPRESS;
		} else if (expDupacks_ == -1) {
			if (sh->numRxmit() < 2) {
				snoop_rxmit(pkt);
				return SNOOP_SUPPRESS;
			}
		} else		// let sender deal with it
			return SNOOP_PROPAGATE;
	} else {		// a new ack
		fstate_ &= ~SNOOP_NOACK; // have seen at least 1 new ack
		/* free buffers */
		double sndTime = snoop_cleanbufs_(ack);
		if (sndTime != -1)
			snoop_rtt_(sndTime);
		expDupacks_ = 0;
		expNextAck_ = buftail_;
		lastAck_ = ack;
	}
	return SNOOP_PROPAGATE;
}

double
Snoop::snoop_cleanbufs_(int ack)
{
	Scheduler &s = Scheduler::instance();
	double sndTime = -1;

	if (toutPending_)
		s.cancel(toutPending_);
	toutPending_ = 0;
	if (empty_())
		return sndTime;
	int i = buftail_;
	do {
		Packet *p = pkts_[i];
		hdr_snoop *sh = (hdr_snoop *)p->access(off_snoop_);
		int seq = ((hdr_tcp*)p->access(off_tcp_))->seqno();
		if (seq <= ack) {
			sndTime = sh->sndTime();
			Packet::free(pkts_[i]);
			pkts_[i] = 0;
			fstate_ &= ~SNOOP_FULL;	// xxx redundant?
		} else if (seq > ack)
			break;
		i = next(i);
	} while (i != bufhead_);

	if ((i != buftail_) || (bufhead_ != buftail_)) {
		fstate_ &= ~SNOOP_FULL;
		buftail_ = i;
	}
	if (!empty_()) {
		toutPending_ = (Event *) (pkts_[buftail_]);
		s.schedule(rxmitHandler_, toutPending_, timeout_());
	}
	return sndTime;
}

void
Snoop::snoop_rtt_(double sndTime)
{
	double diff = Scheduler::instance().clock() - sndTime;
	if (diff > 0) {
		srtt_ = 0.75*srtt_ + 0.25*diff;
	}

}

/*
 * Ideally, would like to schedule snoop retransmissions at higher priority.
 */
void
Snoop::snoop_rxmit(Packet *pkt)
{
	Scheduler& s = Scheduler::instance();
	if (pkt != 0) {
		hdr_snoop *sh = (hdr_snoop *)pkt->access(off_snoop_);
		if (sh->numRxmit() < SNOOP_MAX_RXMIT) {
			printf("%f Rxmitting packet %d\n", s.clock(), ((hdr_tcp*)pkt->access(off_tcp_))->seqno());
			sh->sndTime() = s.clock();
			sh->numRxmit() = sh->numRxmit() + 1;
			Packet *p = pkt->copy();
			LL::recv(p, callback_);
		}
	}
	/* Reset timeout for later time. */
	if (toutPending_)
		s.cancel(toutPending_);
	toutPending_ = (Event *)pkt;
	s.schedule(rxmitHandler_, toutPending_, timeout_());
}

void
SnoopRxmitHandler::handle(Event *)
{
	Packet *p = snoop_->pkts_[snoop_->buftail_];
	snoop_->toutPending_ = 0;
	if (p == 0)
		return;
	hdr_snoop *sh = (hdr_snoop *) p->access(snoop_->off_snoop_);
	if (sh->seqno() != snoop_->lastAck_ + 1)
		return;
	if ((snoop_->bufhead_ != snoop_->buftail_) || 
	    (snoop_->fstate_ & SNOOP_FULL)) {
		snoop_->snoop_rxmit(p);
		snoop_->expNextAck_ = snoop_->next(snoop_->buftail_);
	}
}

void
SnoopPersistHandler::handle(Event *) 
{
}
