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

/*
 * Functions invoked by TcpSessionAgent
 */
#include "nilist.h"
#include "chost.h"
#include "tcp-int.h"
#include "random.h"

#ifdef 0
static class ChostClass : public TclClass {
public:
	ChostClass() : TclClass("Agent/Chost") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new CorresHost(atoi(argv[4]),atoi(argv[5]),
				       atoi(argv[6]), atoi(argv[7]), 
				       atoi(argv[8])));
	}
} class_chost;
#endif

CorresHost::CorresHost() : slink(), TcpFsAgent()
/* CorresHost::CorresHost(int addr, int cwndinit = 0, int pathmtu = 1500, 
		       int maxcwnd = 999999, int wndOption = 0 ) : 
	TclObject(), slink(addr)*/
{
	nActive_ = nTimeout_ = nFastRec_ = 0;
	ownd_ = 0;
#ifdef 0
	recover_ = 0;
	maxcwnd_ = maxcwnd;
	ssthresh_ = 10000;	// XXX
	count_ = 0;
	fcnt_ = 0;
	wndOption_ = wndOption;
	pathmtu_ = pathmtu;
#endif
	wndOption_ = 0;
	pathmtu_ = 1500; /* XXX */
	closecwTS_ = 0;
	pending_ = 0;
	connIter_ = new Islist_iter<IntTcpAgent> (conns_);
	rtt_seg_ = NULL;
	//    segfirst_ = seglast_ = NULL;
}


/*
 * Open up the congestion window.
 */
void 
CorresHost::opencwnd(int size, IntTcpAgent *sender = 0)
{
	if (cwnd_ < ssthresh_) {
		/* slow-start (exponential) */
		cwnd_ += 1;
	} else {
		/* linear */
		double f;
		if (!proxyopt_) {
			switch (wndOption_) {
			case 0:
				if ((count_ = count_ + winInc_) >= cwnd_) {
					count_ = 0;
					cwnd_ += winInc_;
				}
				break;
			case 1:
				/* This is the standard algorithm. */
				cwnd_ += winInc_ / cwnd_;
				break;
			default:
#ifdef notdef
				/*XXX*/
				error("illegal window option %d", wndOption_);
#endif
				abort();
			}
		} else {	// proxy
			switch (wndOption_) {
			case 0:
				if ((sender->count_ = sender->count_+winInc_) >= 
				    cwnd_/nActive_) {
					sender->count_ = 0;
					cwnd_ += winInc_;
				}
				break;
			case 1:
				cwnd_ += winInc_ / (cwnd_/nActive_);
				break;
			default:
#ifdef notdef
				/*XXX*/
				error("illegal window option %d", wndOption_);
#endif
				abort();
			}
		}
	}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;
	
	return;
}


/*
 * Close down the congestion window.
 */
void 
CorresHost::closecwnd(int how, double ts, IntTcpAgent *sender=0)
{
	if (proxyopt_) {
		if (ts > sender->closecwTS_)
			closecwnd(how, sender);
	} else {
		if (ts > closecwTS_)
			closecwnd(how, sender);
	}
}
    
void 
CorresHost::closecwnd(int how, IntTcpAgent *sender=0)
{
	closecwTS_ = Scheduler::instance().clock();
	if (proxyopt_) {
		sender->closecwTS_ = closecwTS_;
		how += 10;
	}
	switch (how) {
	case 0:
		/* timeouts, Tahoe dup acks */
		ssthresh_ = int( ownd_ * winMult_ );
		/* Old code: ssthresh_ = int(cwnd_ / 2); */
		cwnd_ = int(wndInit_);
		break;
		
	case 10:
		/* timeouts, Tahoe dup acks */
		ssthresh_ = int( ownd_ - (ownd_/nActive_) * (1-winMult_));
		/* Old code: ssthresh_ = int(cwnd_ / 2); */
		cwnd_ = ownd_ - (ownd_/nActive_ - wndInit_);
		break;

	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
		cwnd_ = ownd_ * winMult_;
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 11:
	    /* Reno dup acks, or after a recent congestion indication. */
		cwnd_ = ownd_ - (ownd_/nActive_) * (1 - winMult_);
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 2:
	case 12:
		/* Tahoe dup acks after a recent congestion indication */
		cwnd_ = wndInit_;
		break;

	case 3:
	case 31:
		/* idle time >= t_rtxcur_ */
		cwnd_ = wndInit_;
		break;
		
	default:
		abort();
	}
	fcnt_ = 0.;
	count_ = 0;
	if (sender)
		sender->count_ = 0;
}


Segment* 
CorresHost::add_pkts(int size, int seqno, int sessionSeqno, int daddr, int dport, 
		     int sport, double ts, IntTcpAgent *sender)
{
	struct Segment *news;

	ownd_ += 1;
	news = new Segment;
	news->seqno_ = seqno;
	news->sessionSeqno_ = sessionSeqno;
	news->daddr_ = daddr;
	news->dport_ = dport;
	news->sport_ = sport;
	news->ts_ = ts;
	news->size_ = 1;
	news->sender_ = sender;
	seglist_.append(news);
	return news;
}

int
CorresHost::clean_segs(int size, Packet *pkt, IntTcpAgent *sender, int sessionSeqno,
		       int clean_dup = 1, int uniq_ts = 0)
{ 
    Segment *cur, *prev=NULL;
    int rval = -1;

    // remove all acked pkts from list
    int found = rmv_old_segs(pkt, sender, clean_dup, uniq_ts);

    // find a candidate for rxmit - either 200 * 2 ms old or 3 late acks for 
    // two packet on same conn
    Islist_iter<Segment> seg_iter(seglist_);
    while ((cur = seg_iter()) != NULL && cur->later_acks_ >= NUMDUPACKS) {
	    Segment *start = cur, *match;
	Islist_iter<Segment> match_iter(seglist_);

	match_iter.set_cur(cur);
	
	if ( (cur->ts_ < lastackTS_ - (0.2 * 2) ) || 
	     sender == cur->sender_) {
		// delack interval or current packet a dupack from sender
		if (cur->sessionSeqno_ > recover_) { /* new loss window */
			closecwnd(1, cur->ts_, cur->sender_);
			recover_ = sessionSeqno - 1;
		}
		if (cur->sender_->rxmit_last(TCP_REASON_DUPACK, cur->seqno_, 
					     cur->sessionSeqno_, cur->ts_)) {
			ownd_ -= start->size_;
			seglist_.remove(cur, prev);
			return 0;	// XXX should I still check other senders
		}
	}

	while (((match = match_iter()) != NULL) && 
	       (match->ts_ <= lastackTS_)) {
		/* XXX we need to slide "start" through the list in order
		   to find the first pkt of each connection */
		if (start->sender_ == match->sender_){
			if (start->sessionSeqno_ > recover_) { /* new loss window */
				closecwnd(1, start->ts_, start->sender_);
				recover_ = sessionSeqno - 1;
			}
			if (start->sender_->rxmit_last(TCP_REASON_DUPACK, 
						       start->seqno_, 
						       start->sessionSeqno_, 
						       start->ts_)) {
				ownd_ -= start->size_;
				seglist_.remove(start, prev); 
				// can send one for every other DUPACK XXXX
				return 0;	// should I still check other senders
			} else
				break;
		}		
	}
	
	prev = cur;
    }
    return(0);
}

int
CorresHost::rmv_old_segs(Packet *pkt, IntTcpAgent *sender, int clean_dup = 1,
			 int uniq_ts = 0)
{
	Islist_iter<Segment> seg_iter(seglist_);
	Segment *cur, *prev=0;
	int found = 0;
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	if (tcph->ts_echo() > lastackTS_)
		lastackTS_ = tcph->ts_echo();

	while ((!found) &&
	       ((cur = seg_iter()) != NULL) && 
	       (tcph->ts_echo() >= cur->ts_)) {

		/* older pkt */
		if (sender != cur->sender_) {
			cur->later_acks_++;
			prev = cur;
		} else {
			if (tcph->seqno() == cur->seqno_ && 
			    tcph->ts_echo() == cur->ts_)
				/* found it */
				found = 1;
			if (tcph->seqno() >= cur->seqno_) {
				/* clean up acked pkts */
				ownd_ -= cur->size_;
				seglist_.remove(cur, prev);
				if (!prev)
					prev = seg_iter.get_last();
				if (prev == cur) 
					prev = NULL;
				if (cur == rtt_seg_)
					rtt_seg_ = NULL;
				delete cur;
				seg_iter.set_cur(prev);
			} else {
				if (tcph->ts_echo() > cur->ts_) {
					cur->later_acks_++;
					prev = cur;
					ownd_ -= cur->size_;
					cur->size_ = 0;
				} else /* pkt.ts == cur.ts */
					if (cur->size_ != 0 && clean_dup) {
						ownd_ -= cur->size_;
						if (uniq_ts) {
							seglist_.remove(cur, 
									prev);
							if (!prev)
								prev = seg_iter.get_last();
							if (prev == cur) 
								prev = NULL;
							if (cur == rtt_seg_)
								rtt_seg_ = NULL;
							delete cur;
							seg_iter.set_cur(prev);
							found = 1;
						} else 
							cur->size_ = 0;
						break; // most likely a dupack do first only
					}
			}
		}
	}
	return found;
}
	
void
CorresHost::add_agent(IntTcpAgent *agent, int size, double winMult, 
		      int winInc, int ssthresh)
{
	nActive_++; 
	cwnd_ += 1; /* XXX should this be done? */
	wndInit_ = 1;
	winMult_ = winMult;
	winInc_ = winInc;
/*	ssthresh_ = ssthresh;*/
	conns_.append(agent);
}

int
CorresHost::ok_to_snd(int size)
{
	return (cwnd_ >= ownd_ + 1);
}

IntTcpAgent *
CorresHost::who_to_snd(int how)
{
	int i = 0;
	switch (how) {
	case ROUND_ROBIN: {
		IntTcpAgent *next;
		
		do {
			if ((next = (*connIter_)()) == NULL) {
				connIter_->set_cur(connIter_->get_last());
				next = (*connIter_)();
			}
			i++;
		} while (next && !next->data_left_to_send() && 
			 (i < connIter_->count()));
		if (!next->data_left_to_send())
			next = NULL;
		return next;
	}
	case RANDOM: {
		IntTcpAgent *next;
		
		do {
			int foo = int(random() * nActive_ + 1);
			
			connIter_->set_cur(connIter_->get_last());
			
			for (;foo > 0; foo--)
				(*connIter_)();
			next = (*connIter_)();
		} while (next && !next->data_left_to_send());
		return(next);
	}
	default:
		return NULL;
	}
}
