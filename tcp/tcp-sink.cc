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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-sink.cc,v 1.20 1997/12/25 21:36:23 padmanab Exp $ (LBL)";
#endif

#include "tcp-sink.h"

static class TcpSinkClass : public TclClass {
public:
	TcpSinkClass() : TclClass("Agent/TCPSink") {}
	TclObject* create(int, const char*const*) {
		return (new TcpSink(new Acker));
	}
} class_tcpsink;

Acker::Acker() : next_(0), maxseen_(0), ts_to_echo_(0)
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

void Acker::update(int seq)
{

	if (seq - next_ >= MWM) {
		// Protect seen_ array from overflow.
		// The missing ACK will ultimately cause sender timeout
		// and retransmission of packet next_.
		return;
	}

	if (seq > maxseen_) {
		int i;
		for (i = maxseen_ + 1; i < seq; ++i)
			seen_[i & MWM] = 0;
		maxseen_ = seq;
		seen_[maxseen_ & MWM] = 1;
		seen_[(maxseen_ + 1) & MWM] = 0;
	}
	int next = next_;
	if (seq >= next && seq <= maxseen_) {
		/*
		 * setting the sequence number.
		 * should be the last in sequence packet seen
		 */
		seen_[seq & MWM] = 1;
		while (seen_[next & MWM])
			++next;
		next_ = next;
	}
}

TcpSink::TcpSink(Acker* acker) : Agent(PT_ACK), acker_(acker)
{
	bind("packetSize_", &size_);
	bind("off_tcp_", &off_tcp_);
	bind("maxSackBlocks_", &max_sack_blocks_); // used only by sack
	bind_bool("ts_echo_bugfix_", &ts_echo_bugfix_);
}

void Acker::append_ack(hdr_cmn*, hdr_tcp*, int) const
{
}

void TcpSink::reset() 
{
	acker_->reset();	
}

void TcpSink::ack(Packet* opkt)
{
	Packet* npkt = allocpkt();
	double now = Scheduler::instance().clock();

	hdr_tcp *otcp = (hdr_tcp*)opkt->access(off_tcp_);
	hdr_tcp *ntcp = (hdr_tcp*)npkt->access(off_tcp_);
	ntcp->seqno() = acker_->Seqno();
	ntcp->ts() = now;

	if (ts_echo_bugfix_)  /* TCP/IP Illustrated, Vol. 2, pg. 870 */
		ntcp->ts_echo() = acker_->ts_to_echo();
	else
		ntcp->ts_echo() = otcp->ts();

	hdr_ip* oip = (hdr_ip*)opkt->access(off_ip_);
	hdr_ip* nip = (hdr_ip*)npkt->access(off_ip_);
	nip->flowid() = oip->flowid();

	hdr_flags* of = (hdr_flags*)opkt->access(off_flags_);
	hdr_flags* nf = (hdr_flags*)npkt->access(off_flags_);
	nf->ecn_ = of->ecn_to_echo_;

	acker_->append_ack((hdr_cmn*)npkt->access(off_cmn_),
			   ntcp, otcp->seqno());
	add_to_ack(npkt);
	send(npkt, 0);
}

void TcpSink::add_to_ack(Packet*)
{
	return;
}

void TcpSink::recv(Packet* pkt, Handler*)
{
	hdr_tcp *th = (hdr_tcp*)pkt->access(off_tcp_);
	acker_->update_ts(th->seqno(),th->ts());
      	acker_->update(th->seqno());
      	ack(pkt);
	Packet::free(pkt);
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
	hdr_tcp *th = (hdr_tcp*)pkt->access(off_tcp_);
	acker_->update_ts(th->seqno(),th->ts());
	acker_->update(th->seqno());
        /*
         * If there's no timer and the packet is in sequence, set a timer.
         * Otherwise, send the ack and update the timer.
         */
        if (!(delay_timer_.status() == TIMER_PENDING) && 
				th->seqno() == acker_->Seqno()) {
                /*
                 * There's no timer, so set one and delay this ack.
                 */
		save_ = pkt;
		delay_timer_.resched(interval_);
                return;
        }
        /*
         * If there was a timer, turn it off.
         */
	if (delay_timer_.status() == TIMER_PENDING) {
		delay_timer_.cancel();
		Packet::free(save_);
		save_ = 0;
	}
	ack(pkt);
	Packet::free(pkt);
}

void DelAckSink::timeout(int)
{
	/*
	 * The timer expired so we ACK the last packet seen.
	 * (shouldn't this check for a particular time out#?  -kf)
	 */
	Packet* pkt = save_;
	ack(pkt);
	save_ = 0;
	Packet::free(pkt);
}

void DelayTimer::expire(Event *e) {
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
	SackStack(int);
	~SackStack();
	int& head_right(int n = 0) { return SFE_[n].right_; }
	int& head_left(int n = 0) { return SFE_[n].left_; }
	int cnt() { return cnt_; }
	void reset() {
		register i;
		for (i = 0; i < cnt_; i++)
			SFE_[i].left_ = SFE_[i].right_ = -1;

		cnt_ = 0;
	}

	inline void push(int n = 0) {
 		if (cnt_ >= size_) cnt_ = size_ - 1;  // overflow check
		register i;
		for (i = cnt_-1; i >= n; i--)
			SFE_[i+1] = SFE_[i];	// not efficient for big size
		cnt_++;
	}

	inline void pop(int n = 0) {
		register i;
		for (i = n; i < cnt_-1; i++)
			SFE_[i] = SFE_[i+1];	// not efficient for big size
		SFE_[i].left_ = SFE_[i].right_ = -1;
		cnt_--;
	}
};

SackStack::SackStack(int sz)
{
	register i;
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
        int sack_index, i, sack_right, sack_left;
	int recent_sack_left, recent_sack_right;
          
	int seqno = Seqno();

        sack_index = 0;
	sack_left = sack_right = -1;

        if (old_seqno < 0) {
                printf("Error: invalid packet number %d\n", old_seqno);
        } else if (seqno >= maxseen_ && (sf_->cnt() != 0))
		sf_->reset();
	else if ((seqno < maxseen_) && (base_nblocks_ > 0)) {
                /*  Build FIRST SACK block  */
                sack_right=-1;

		/* look rightward for first hole */
                for (i=old_seqno; i<=maxseen_; i++) {
			if (!seen_[i & MWM]) {
				sack_right=i;
				break;
			}
		}

                if (sack_right == -1) {
			sack_right = maxseen_+1;
                }

		if (old_seqno <= seqno) {
			sack_left = 0;
		} else {
			/* look leftward from right edge for first hole */
	                for (i = sack_right-1; i > seqno; i--) {
				if (!seen_[i & MWM]) {
					sack_left = i+1;
					break;
				}
	                }
			h->sa_left()[sack_index] = sack_left;
			h->sa_right()[sack_index] = sack_right;
			sack_index++;
		}

		recent_sack_left = sack_left;
		recent_sack_right = sack_right;

		/* first sack block is built, check the others */
		/*
		 * make sure that if max_sack_blocks has been made
		 * large from tcl we don't over-run the stuff we
		 * allocated in Sacker::Sacker()
		 */

		int k = 0;
                while (sack_index < base_nblocks_) {

			sack_left = sf_->head_left(k);
			sack_right = sf_->head_right(k);

			/* no more history */
			if (sack_left < 0 || sack_right < 0 ||
				sack_right > maxseen_ + 1)
				break;

			/* newest ack "covers up" this one */
			if (recent_sack_left <= sack_left &&
			    recent_sack_right >= sack_right) {
				sf_->pop(k);
				continue;
			}

			h->sa_left()[sack_index] = sack_left;
			h->sa_right()[sack_index] = sack_right;
			sack_index++;
			k++;

                }

		if (old_seqno > seqno) {
		 	/* put most recent block onto stack */
			sf_->push();
			sf_->head_left() = recent_sack_left;
			sf_->head_right() = recent_sack_right;
		}
                
        }
	h->sa_length() = sack_index;
	ch->size() += sack_index * 8;
}
