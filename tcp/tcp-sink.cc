/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
 
#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-sink.cc,v 1.37 1999/11/24 22:20:09 hyunahpa Exp $ (LBL)";
#endif

#include "flags.h"
#include "ip.h"
#include "tcp-sink.h"

static class TcpSinkClass : public TclClass {
public:
	TcpSinkClass() : TclClass("Agent/TCPSink") {}
	TclObject* create(int, const char*const*) {
		return (new TcpSink(new Acker));
	}
} class_tcpsink;

Acker::Acker() : next_(0), maxseen_(0), ecn_unacked_(0), ts_to_echo_(0)
{
	memset(seen_, 0, sizeof(seen_));
}

void Acker::reset() 
{
	next_ = 0;
	maxseen_ = 0;
	memset(seen_, 0, sizeof(seen_));
}	

void Acker::update_ts(int seqno, double ts)
{
	if (ts >= ts_to_echo_ && seqno <= next_)
		ts_to_echo_ = ts;
}

// returns number of bytes that can be "delivered" to application
// also updates the receive window (i.e. next_, maxseen, and seen_ array)
int Acker::update(int seq, int numBytes)
{
	if (numBytes <= 0)
		printf("Error, received TCP packet size <= 0\n");
	int numToDeliver = 0;
	if (seq - next_ >= MWM) {
		// next_ is next packet expected; MWM is the maximum
		// window size minus 1; if somehow the seqno of the
		// packet is greater than the one we're expecting+MWM, 
		// then ignore it.
		return 0;
	}

	if (seq > maxseen_) {
		// the packet is the highest one we've seen so far
		int i;
		for (i = maxseen_ + 1; i < seq; ++i)
			seen_[i & MWM] = 0;
		// we record the packets between the old maximum and
		// the new max as being "unseen" i.e. 0 bytes of each
		// packet have been received
		maxseen_ = seq;
		seen_[maxseen_ & MWM] = numBytes;
		// store how many bytes have been seen for this packet
		seen_[(maxseen_ + 1) & MWM] = 0;
		// clear the array entry for the packet immediately 
		// after this one
	}
	int next = next_;
	if (seq >= next && seq <= maxseen_) {
		// next is the left edge of the recv window; maxseen_
		// is the right edge; execute this block if there are
		// missing packets in the recv window AND if current
		// packet falls within those gaps

		seen_[seq & MWM] = numBytes;
		// record the packet as being seen
		while (seen_[next & MWM]) {
			// this loop first gets executed if seq==next;
			// i.e., this is the next packet in order that
			// we've been waiting for.  the loop sets how
			// many bytes we can now deliver to the
			// application, due to this packet arriving
			// (and the prior arrival of any segments
			// immediately to the right)
			numToDeliver += seen_[next & MWM];
			++next;
		}
		next_ = next;
		// store the new left edge of the window
	}
	return numToDeliver;
}

TcpSink::TcpSink(Acker* acker) : Agent(PT_ACK), acker_(acker), save_(NULL)
{
#ifdef TCP_DELAY_BIND
#else       
	bind("packetSize_", &size_);
	bind("maxSackBlocks_", &max_sack_blocks_); // used only by sack
	bind_bool("ts_echo_bugfix_", &ts_echo_bugfix_);
#endif
}

#ifdef TCP_DELAY_BIND
void
TcpSink::delay_bind_init_all()
{
        delay_bind_init_one("packetSize_");
        delay_bind_init_one("maxSackBlocks_");
        delay_bind_init_one("ts_echo_bugfix_");

	Agent::delay_bind_init_all();
}
#endif

#ifdef TCP_DELAY_BIND
int
TcpSink::delay_bind_dispatch(const char *varName, const char *localName)
{
        DELAY_BIND_DISPATCH(varName, localName, "packetSize_", delay_bind, &size_);
        DELAY_BIND_DISPATCH(varName, localName, "maxSackBlocks_", delay_bind, &max_sack_blocks_);
        DELAY_BIND_DISPATCH(varName, localName, "ts_echo_bugfix_", delay_bind_bool, &ts_echo_bugfix_);

        return Agent::delay_bind_dispatch(varName, localName);
}
#endif

void Acker::append_ack(hdr_cmn*, hdr_tcp*, int) const
{
}

void Acker::update_ecn_unacked(int value)
{
	ecn_unacked_ = value;
}

int TcpSink::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void TcpSink::reset() 
{
	acker_->reset();	
	save_ = NULL;
}

void TcpSink::ack(Packet* opkt)
{
	Packet* npkt = allocpkt();
	// opkt is the "old" packet that was received
	// npkt is the "new" packet being constructed (for the ACK)
	double now = Scheduler::instance().clock();
	hdr_flags *sf;

	hdr_tcp *otcp = hdr_tcp::access(opkt);
	hdr_tcp *ntcp = hdr_tcp::access(npkt);
	// get the tcp headers
	ntcp->seqno() = acker_->Seqno();
	// get the cumulative sequence number to put in the ACK; this
	// is just the left edge of the receive window - 1
	ntcp->ts() = now;
	// timestamp the packet

	if (ts_echo_bugfix_)  /* TCP/IP Illustrated, Vol. 2, pg. 870 */
		ntcp->ts_echo() = acker_->ts_to_echo();
	else
		ntcp->ts_echo() = otcp->ts();
	// echo the original's time stamp

	hdr_ip* oip = (hdr_ip*)opkt->access(off_ip_);
	hdr_ip* nip = (hdr_ip*)npkt->access(off_ip_);
	// get the ip headers
	nip->flowid() = oip->flowid();
	// copy the flow id
	
	hdr_flags* of = (hdr_flags*)opkt->access(off_flags_);
	hdr_flags* nf = (hdr_flags*)npkt->access(off_flags_);
	if (save_ != NULL) 
		sf = (hdr_flags*)save_->access(off_flags_);
		// Look at delayed packet being acked. 
	if ( (save_ != NULL && sf->cong_action()) || of->cong_action() ) 
		// Sender has responsed to congestion. 
		acker_->update_ecn_unacked(0);
	if ( (save_ != NULL && sf->ect() && sf->ce())  || 
			(of->ect() && of->ce()) )
		// New report of congestion.  
		acker_->update_ecn_unacked(1);
	if ( (save_ != NULL && sf->ect()) || of->ect() )
		// Set EcnEcho bit.  
		nf->ecnecho() = acker_->ecn_unacked();
	if (!of->ect() && of->ecnecho() ||
		(save_ != NULL && !sf->ect() && sf->ecnecho()) ) 
		 // This is the negotiation for ECN-capability.
		 // We are not checking for of->cong_action() also. 
		 // In this respect, this does not conform to the 
		 // specifications in the internet draft 
		nf->ecnecho() = 1;
	acker_->append_ack((hdr_cmn*)npkt->access(off_cmn_),
			   ntcp, otcp->seqno());
	add_to_ack(npkt);
	// the above function is used in TcpAsymSink
	
	send(npkt, 0);
	// send it
}

void TcpSink::add_to_ack(Packet*)
{
	return;
}

void TcpSink::recv(Packet* pkt, Handler*)
{
	int numToDeliver;
	int numBytes = ((hdr_cmn*)pkt->access(off_cmn_))->size();
	// number of bytes in the packet just received
	hdr_tcp *th = hdr_tcp::access(pkt);
	acker_->update_ts(th->seqno(),th->ts());
	// update the timestamp to echo
	
      	numToDeliver = acker_->update(th->seqno(), numBytes);
	// update the recv window; figure out how many in-order-bytes
	// (if any) can be removed from the window and handed to the
	// application
	if (numToDeliver)
		recvBytes(numToDeliver);
	// send any packets to the application
      	ack(pkt);
	// ACK the packet
	Packet::free(pkt);
	// remove it from the system
}

static class DelSinkClass : public TclClass {
public:
	DelSinkClass() : TclClass("Agent/TCPSink/DelAck") {}
	TclObject* create(int, const char*const*) {
		return (new DelAckSink(new Acker));
	}
} class_delsink;

DelAckSink::DelAckSink(Acker* acker) : TcpSink(acker), delay_timer_(this)
{
	bind_time("interval_", &interval_);
}

void DelAckSink::recv(Packet* pkt, Handler*)
{
	int numToDeliver;
	int numBytes = ((hdr_cmn*)pkt->access(off_cmn_))->size();
	hdr_tcp *th = hdr_tcp::access(pkt);
	acker_->update_ts(th->seqno(),th->ts());
	numToDeliver = acker_->update(th->seqno(), numBytes);
	if (numToDeliver)
		recvBytes(numToDeliver);
        // If there's no timer and the packet is in sequence, set a timer.
        // Otherwise, send the ack and update the timer.
        if (delay_timer_.status() != TIMER_PENDING && 
				th->seqno() == acker_->Seqno()) {
               	// There's no timer, so set one and delay this ack.
		save_ = pkt;
		delay_timer_.resched(interval_);
                return;
        }
        // If there was a timer, turn it off.
	if (delay_timer_.status() == TIMER_PENDING) 
		delay_timer_.cancel();
	ack(pkt);
        if (save_ != NULL) {
                Packet::free(save_);
                save_ = NULL;
        }

	Packet::free(pkt);
}

void DelAckSink::timeout(int)
{
	// The timer expired so we ACK the last packet seen.
	Packet* pkt = save_;
	ack(pkt);
	save_ = NULL;
	Packet::free(pkt);
}

void DelayTimer::expire(Event* /*e*/) {
	a_->timeout(0);
}

/* "sack1-tcp-sink" is for Matt and Jamshid's implementation of sack. */

class SackStack {
protected:
	int size_;
	int cnt_;
	struct Sf_Entry {
		int left_;
		int right_;
	} *SFE_;
public:
	SackStack(int); 	// create a SackStack of size (int)
	~SackStack();
	int& head_right(int n = 0) { return SFE_[n].right_; }
	int& head_left(int n = 0) { return SFE_[n].left_; }
	int cnt() { return cnt_; }  	// how big is the stack
	void reset() {
		register int i;
		for (i = 0; i < cnt_; i++)
			SFE_[i].left_ = SFE_[i].right_ = -1;

		cnt_ = 0;
	}

	inline void push(int n = 0) {
 		if (cnt_ >= size_) cnt_ = size_ - 1;  // overflow check
		register int i;
		for (i = cnt_-1; i >= n; i--)
			SFE_[i+1] = SFE_[i];	// not efficient for big size
		cnt_++;
	}

	inline void pop(int n = 0) {
		register int i;
		for (i = n; i < cnt_-1; i++)
			SFE_[i] = SFE_[i+1];	// not efficient for big size
		SFE_[i].left_ = SFE_[i].right_ = -1;
		cnt_--;
	}
};

SackStack::SackStack(int sz)
{
	register int i;
	size_ = sz;
	SFE_ = new Sf_Entry[sz];
	for (i = 0; i < sz; i++)
		SFE_[i].left_ = SFE_[i].right_ = -1;
	cnt_ = 0;
}

SackStack::~SackStack()
{
	delete SFE_;
}

static class Sack1TcpSinkClass : public TclClass {
public:
        Sack1TcpSinkClass() : TclClass("Agent/TCPSink/Sack1") {}
	TclObject* create(int, const char*const*) {
		Sacker* sacker = new Sacker;
		TcpSink* sink = new TcpSink(sacker);
		sacker->configure(sink);
		return (sink);
        }
} class_sack1tcpsink;

static class Sack1DelAckTcpSinkClass : public TclClass {
public:
	Sack1DelAckTcpSinkClass() : TclClass("Agent/TCPSink/Sack1/DelAck") {}
	TclObject* create(int, const char*const*) {
		Sacker* sacker = new Sacker;
		TcpSink* sink = new DelAckSink(sacker);
		sacker->configure(sink);
		return (sink);
	}
} class_sack1delacktcpsink;

void Sacker::configure(TcpSink *sink)
{
	if (sink == NULL) {
		fprintf(stderr, "warning: Sacker::configure(): no TCP sink!\n");
		return;
	}

	TracedInt& nblocks = sink->max_sack_blocks_;
	if (int(nblocks) > NSA) {
		fprintf(stderr, "warning: TCP header limits number of SACK blocks to %d\n", NSA);
		nblocks = NSA;
	}
	sf_ = new SackStack(int(nblocks));
	nblocks.tracer(this);
	base_nblocks_ = int(nblocks);
}

void
Sacker::trace(TracedVar *v)
{
	// we come here if "nblocks" changed
	TracedInt* ti = (TracedInt*) v;

	if (int(*ti) > NSA) {
		fprintf(stderr, "warning: TCP header limits number of SACK blocks to %d\n", NSA);
		*ti = NSA;
	}

	int newval = int(*ti);
	delete sf_;
	sf_ = new SackStack(newval);
	base_nblocks_ = newval;
}

void Sacker::reset() 
{
	sf_->reset();
	Acker::reset();
}

Sacker::~Sacker()
{
	delete sf_;
}

void Sacker::append_ack(hdr_cmn* ch, hdr_tcp* h, int old_seqno) const
{
	// ch and h are the common and tcp headers of the Ack being constructed
	// old_seqno is the sequence # of the packet we just got
	
        int sack_index, i, sack_right, sack_left;
	int recent_sack_left, recent_sack_right;
          
	int seqno = Seqno();

        sack_index = 0;
	sack_left = sack_right = -1;
	// initialization; sack_index=0 and sack_{left,right}= -1

        if (old_seqno < 0) {
                printf("Error: invalid packet number %d\n", old_seqno);
        } else if (seqno >= maxseen_ && (sf_->cnt() != 0))
		sf_->reset();
	// if the Cumulative ACK seqno is at or beyond the right edge
	// of the window, and if the SackStack is not empty, reset it
	// (empty it)
	else if ((seqno < maxseen_) && (base_nblocks_ > 0)) {
		// Otherwise, if the received packet is to the left of
		// the right edge of the receive window (but not at
		// the right edge), OR if it is a duplicate, AND we
		// can have 1 or more Sack blocks, then execute the
		// following, which computes the most recent Sack
		// block
		
                // Build FIRST SACK block  
                sack_right=-1;

		// look rightward for first hole 
		// start at the current packet 
                for (i=old_seqno; i<=maxseen_; i++) {
			if (!seen_[i & MWM]) {
				sack_right=i;
				break;
			}
		}

		// if there's no hole set the right edge of the sack
		// to be the next expected packet
                if (sack_right == -1) {
			sack_right = maxseen_+1;
                }

		// if the current packet's seqno is smaller than the
		// left edge of the window, set the sack_left to 0
		if (old_seqno <= seqno) {
			sack_left = 0;
			// don't record/send the block
		} else {
			// look leftward from right edge for first hole 
	                for (i = sack_right-1; i > seqno; i--) {
				if (!seen_[i & MWM]) {
					sack_left = i+1;
					break;
				}
	                }
			h->sa_left(sack_index) = sack_left;
			h->sa_right(sack_index) = sack_right;
			// record the block
			sack_index++;
		}

		recent_sack_left = sack_left;
		recent_sack_right = sack_right;

		// first sack block is built, check the others 
		// make sure that if max_sack_blocks has been made
		// large from tcl we don't over-run the stuff we
		// allocated in Sacker::Sacker()
		int k = 0;
                while (sack_index < base_nblocks_) {

			sack_left = sf_->head_left(k);
			sack_right = sf_->head_right(k);

			// no more history 
			if (sack_left < 0 || sack_right < 0 ||
				sack_right > maxseen_ + 1)
				break;

			// newest ack "covers up" this one 

			if (recent_sack_left <= sack_left &&
			    recent_sack_right >= sack_right) {
				sf_->pop(k);
				continue;
			}

			h->sa_left(sack_index) = sack_left;
			h->sa_right(sack_index) = sack_right;
			// store the old sack (i.e. move it down one)
			sack_index++;
			k++;

                }

		if (old_seqno > seqno) {
		 	/* put most recent block onto stack */
			sf_->push();
			// this just moves things down 1 from the
			// beginning, but it doesn't push any values
			// on the stack
			sf_->head_left() = recent_sack_left;
			sf_->head_right() = recent_sack_right;
			// this part stores the left/right values at
			// the top of the stack (slot 0)
		}
                
        }
	h->sa_length() = sack_index;
	// set the Length of the sack stack in the header
	ch->size() += sack_index * 8;
	// change the size of the common header to account for the
	// Sack strings (2 4-byte words for each element)
}
