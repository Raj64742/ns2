/*
 * Copyright (c) 1991-1995 Regents of the University of California.
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
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-sink.cc,v 1.2 1997/01/26 22:32:37 mccanne Exp $ (LBL)";
#endif

#include <math.h>
#include "packet.h"
#include "agent.h"

/* max window size */
#define MWS 1024
#define MWM (MWS-1)
/* For Tahoe TCP, the "window" parameter, representing the receiver's
 * advertised window, should be less than MWM.  For Reno TCP, the
 * "window" parameter should be less than MWM/2.
 */

class Acker {
public:
	Acker();
	virtual ~Acker() {}
	void update(int seqno);
	inline int Seqno() const { return (next_ - 1); }
	virtual void build_ack(Packet* newpkt, const Packet *pkt) const;
protected:
	int next_;		/* next packet expected  */
	int maxseen_;		/* max packet number seen */
	int seen_[MWS];		/* array of packets seen  */
};

class TcpSink : public Agent {
public:
	TcpSink(Acker*);
	void recv(Packet* pkt, Handler*);
protected:
	void ack(Packet*);
	Acker* acker_;
};

static class TcpSinkClass : public TclClass {
public:
	TcpSinkClass() : TclClass("agent/tcp-sink") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpSink(new Acker));
	}
} class_tcpsink;

Acker::Acker() : next_(0), maxseen_(0)
{
	memset(seen_, 0, sizeof(seen_));
}

void Acker::update(int seq)
{
	if (seq - next_ >= MWM)
		// Protect seen_ array from overflow.
		// The missing ACK will ultimately cause sender timeout
		// and retransmission of packet next_.
		return;

	if (seq > maxseen_) {
		for (int i = maxseen_ + 1; i < seq; ++i)
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
	bind("packet-size", &size_);
}

void Acker::build_ack(Packet* newpkt, const Packet *pkt) const
{
	newpkt->seqno_ = Seqno();
	newpkt->flags_ = pkt->flags_;
	newpkt->bd_.tcp_.ts_ = pkt->bd_.tcp_.ts_;
	newpkt->class_ = pkt->class_;
}

void TcpSink::ack(Packet *pkt)
{
	Packet* newpkt = allocpkt();
	acker_->build_ack(newpkt, pkt);
	send(newpkt, 0);
}

void TcpSink::recv(Packet* pkt, Handler*)
{
      	acker_->update(pkt->seqno_);
      	ack(pkt);
	Packet::free(pkt);
}

class DelAckSink : public TcpSink {
public:
	DelAckSink(Acker* acker);
	void recv(Packet* pkt, Handler*);
protected:
	virtual void timeout(int tno);
	Packet* save_;		/* place to stash packet while delaying */
	double interval_;
};

static class DelSinkClass : public TclClass {
public:
	DelSinkClass() : TclClass("agent/tcp-sink-da") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DelAckSink(new Acker));
	}
} class_delsink;

DelAckSink::DelAckSink(Acker* acker) : TcpSink(acker)
{
	bind_time("interval", &interval_);
}

#define DELAY_TIMER 0

void DelAckSink::recv(Packet* pkt, Handler*)
{
	acker_->update(pkt->seqno_);
        /*
         * If there's no timer and the packet is in sequence, set a timer.
         * Otherwise, send the ack and update the timer.
         */
        if (!pending_[DELAY_TIMER] && pkt->seqno_ == acker_->Seqno()) {
                /*
                 * There's no timer, so set one and delay this ack.
                 */
		save_ = pkt;
		sched(interval_, DELAY_TIMER);
                return;
        }
        /*
         * If there was a timer, turn it off.
         */
	if (pending_[DELAY_TIMER]) {
		cancel(DELAY_TIMER);
		Packet::free(save_);
		save_ = 0;
	}
	ack(pkt);
	Packet::free(pkt);
}

void DelAckSink::timeout(int tno)
{
	/*
	 * The timer expired so we ACK the last packet seen.
	 */
	Packet* pkt = save_;
	ack(pkt);
	save_ = 0;
	Packet::free(pkt);
}

/* "sack1-tcp-sink" is for Matt and Jamshid's implementation of sack. */

#define MAX_OLD_SACK 100

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
		for (register i = cnt_-1; i >= n; i--)
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
	size_ = sz;
	SFE_ = new Sf_Entry[sz];
	for (register i = 0; i < sz; i++)
		SFE_[i].left_ = SFE_[i].right_ = -1;
	cnt_ = 0;
}

SackStack::~SackStack()
{
	delete SFE_;
}

class Sacker : public Acker {
public: 
	Sacker();
	~Sacker();
	void build_ack(Packet* newpkt, const Packet *pkt) const;
protected:
        int max_sack_blocks_;
	SackStack *sf_;
};

static class Sack1TcpSinkClass : public TclClass {
public:
        Sack1TcpSinkClass() : TclClass("agent/sack1-tcp-sink") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new TcpSink(new Sacker));
        }
} class_sack1tcpsink;

static class Sack1DelAckTcpSinkClass : public TclClass {
public:
	Sack1DelAckTcpSinkClass() : TclClass("agent/sack1-tcp-sink-da") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DelAckSink(new Sacker));
	}
} class_sack1delacktcpsink;

Sacker::Sacker()
{
#ifdef notdef
        bind("max-sack-blocks", &max_sack_blocks_);
#endif
	/*XXX what's the point of binding if this can change?*/
	sf_ = new SackStack(max_sack_blocks_);
}

Sacker::~Sacker()
{
	delete sf_;
}

void Sacker::build_ack(Packet* newpkt, const Packet *pkt) const
{
        int sack_index, i, sack_right, sack_left;
	int recent_sack_left, recent_sack_right;
          
	Acker::build_ack(newpkt, pkt);
	int seqno = Seqno();

        sack_index = 0;

	bd_tcp* bd = &newpkt->bd_.tcp_;

        if (pkt->seqno_<0) {
                printf("Error: invalid packet number %d\n", pkt->seqno_);
        }
	else if (seqno >= maxseen_ && (sf_->cnt() != 0))
		sf_->reset();
	else if ((seqno < maxseen_) && (max_sack_blocks_ > 0)) {
                /*  Build FIRST SACK block  */
                sack_right=-1;

		/* look rightward for first hole */
                for (i=pkt->seqno_; i<=maxseen_; i++) {
                              if (!seen_[i & MWM]) {
                                                sack_right=i;
                                                break;
		}
		}

                if (sack_right == -1) {
                                sack_right = maxseen_+1;
                }

		if (pkt->seqno_ <= seqno) {
			sack_left = 0;
		} else {
			/* look leftward from right edge for first hole */
	                for (i = sack_right-1; i > seqno; i--) {
	                                if (!seen_[i & MWM]) {
	                                        sack_left = i+1;
	                                        break;
	                                }
	                }
			bd->sa_left_[sack_index] = sack_left;
			bd->sa_right_[sack_index] = sack_right;
			sack_index++;
		}

		recent_sack_left = sack_left;
		recent_sack_right = sack_right;

		/* first sack block is built, check the others */
		int k = 0;
                while (sack_index < max_sack_blocks_) {

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

			bd->sa_left_[sack_index] = sack_left;
			bd->sa_right_[sack_index] = sack_right;
			sack_index++;
			k++;

                }

		if (pkt->seqno_ > seqno) {
		 	/* put most recent block onto stack */
			sf_->push();
			sf_->head_left() = recent_sack_left;
			sf_->head_right() = recent_sack_right;
		}
                
        }
	bd->sa_length_ = sack_index;
	newpkt->size_ += sack_index * 8;
}

