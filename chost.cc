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
	owndCorrection_ = 0;
#ifdef 0
	recover_ = 0;
	maxcwnd_ = maxcwnd;
	ssthresh_ = 10000;	// XXX
	count_ = 0;
	fcnt_ = 0;
	wndOption_ = wndOption;
	pathmtu_ = pathmtu;
#endif
/*	wndOption_ = 0;*/
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
			switch (wnd_option_) {
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
				error("illegal window option %d", wnd_option_);
#endif
				abort();
			}
		} else {	// proxy
			switch (wnd_option_) {
			case 0: 
			case 1:
				if (sender->highest_ack_ >= sender->wndIncSeqno_) {
					cwnd_ += winInc_;
					sender->wndIncSeqno_ = 0;
				}
				break;
			default:
#ifdef notdef
				/*XXX*/
				error("illegal window option %d", wnd_option_);
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

void 
CorresHost::closecwnd(int how, double ts, IntTcpAgent *sender=0)
{
	if (proxyopt_) {
		if (!sender || ts > sender->closecwTS_)
			closecwnd(how, sender);
	}
	else {
		if (ts > closecwTS_)
			closecwnd(how, sender);
	}
}

void 
CorresHost::closecwnd(int how, IntTcpAgent *sender=0)
{
	int sender_ownd = 0;
	if (sender)
		sender_ownd = sender->maxseq_ - sender->highest_ack_;
	closecwTS_ = Scheduler::instance().clock();
	if (proxyopt_) {
		if (sender)
			sender->closecwTS_ = closecwTS_;
		how += 10;
	}
	switch (how) {
	case 0:
	case 10:
		/* timeouts */
		ssthresh_ = int( ownd_ * winMult_ );
		cwnd_ = int(wndInit_);
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
		cwnd_ = ownd_ - sender_ownd*(1-winMult_);
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 3:
	case 13:
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

void
CorresHost::adjust_ownd(int size) 
{
	if (owndCorrection_ > 0) {
		owndCorrection_ -= size;
		if (owndCorrection_ < 0)
			printf("In adjust_ownd(): ownd_ = %g  owndCorrection_ = %d\n", double(ownd_), owndCorrection_);
	}
	else
		ownd_ -= size;
}

int
CorresHost::clean_segs(int size, Packet *pkt, IntTcpAgent *sender, int sessionSeqno,
		       int clean_dup = 1, int uniq_ts = 0)
{ 
    Segment *cur, *prev=NULL;
    int rval = -1;

    /* remove all acked pkts from list */
    int acked_new_data = rmv_old_segs(pkt, sender, clean_dup, uniq_ts);
    /* XXX if matching segment not found, increment ownd correction factor */
    if (!acked_new_data) {
	    owndCorrection_++;
	    ownd_--;
    }
    /*
     * find a candidate for rxmit - either 200 * 2 ms old or 3 late acks for 
     * two packet on same conn
     */
    Islist_iter<Segment> seg_iter(seglist_);
    while ((cur = seg_iter()) != NULL) {
        if (cur->later_acks_ < NUMDUPACKS) {
	    prev = cur;
	    continue;
        }
	int remove_flag = 0;
	Segment *start = cur, *match;
	Islist_iter<Segment> match_iter(seglist_);

	match_iter.set_cur(cur);
	
	if ( (cur->ts_ < lastackTS_ - (0.2 * 2) ) || sender == cur->sender_) {
		/* delack interval or current packet a dupack from sender */
		if ((cur->sessionSeqno_ > recover_) ||
		    (proxyopt_ && cur->seqno_ > cur->sender_->recover_)) { 
			/* new loss window */
			closecwnd(1, cur->ts_, cur->sender_);
			recover_ = sessionSeqno - 1;
			cur->sender_->recover_ = cur->sender_->maxseq_;
		}
		if (cur->sender_->rxmit_last(TCP_REASON_DUPACK, cur->seqno_, 
					     cur->sessionSeqno_, cur->ts_)) {
			adjust_ownd(cur->size_);
			seglist_.remove(cur, prev);
			remove_flag = 1;
		}
	}

	while (((match = match_iter()) != NULL) && 
	       (match->ts_ <= lastackTS_)) {
		/* XXX we need to slide "start" through the list in order
		   to find the first pkt of each connection */
		if (start->sender_ == match->sender_){
			if ((start->sessionSeqno_ > recover_) || 
			    (proxyopt_ && start->seqno_ > start->sender_->recover_)) { 
				/* new loss window */
				closecwnd(1, start->ts_, start->sender_);
				recover_ = sessionSeqno - 1;
				cur->sender_->recover_ = cur->sender_->maxseq_;
			}
			if (start->sender_->rxmit_last(TCP_REASON_DUPACK, 
						       start->seqno_, 
						       start->sessionSeqno_, 
						       start->ts_)) {
				adjust_ownd(start->size_);
				seglist_.remove(start, prev); 
				remove_flag = 1;
				// can send one for every other DUPACK XXXX
			} else
				break;
		}		
	}
	if (!remove_flag)
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
	int new_data_acked = 0;
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	if (tcph->ts_echo() > lastackTS_)
		lastackTS_ = tcph->ts_echo();

	while ((!found) && ((cur = seg_iter()) != NULL)) {
		int remove_flag = 0;
		/* ack for older pkt of another connection */
		if (sender != cur->sender_ && tcph->ts_echo() >= cur->ts_) {
			cur->later_acks_++;
		} 
		/* ack for same connection */
		else if (sender == cur->sender_) {
			/* found packet acked */
			if (tcph->seqno() == cur->seqno_ && 
			    tcph->ts_echo() == cur->ts_)
				found = 1;
			/* higher ack => clean up acked packets */
			if (tcph->seqno() >= cur->seqno_) {
				adjust_ownd(cur->size_);
				seglist_.remove(cur, prev);
				remove_flag = 1;
				new_data_acked = 1;
				if (!prev)
					prev = seg_iter.get_last();
				if (prev == cur) 
					prev = NULL;
				if (cur == rtt_seg_)
					rtt_seg_ = NULL;
				delete cur;
				seg_iter.set_cur(prev);
			} 
			/* 
			 * ack with later timestamp (XXX shouldn't happen if
			 * the receiver is using the correct echo algorithms) OR
			 * ack for ack for seqno that is one less (=> dupack)
			 */
			else if (tcph->ts_echo() >= cur->ts_ ||
				 tcph->seqno() == cur->seqno_-1) {
				cur->later_acks_++;
			} 
		}
		if (!remove_flag)
			prev = cur;
	}
	return new_data_acked;
}
	
void
CorresHost::add_agent(IntTcpAgent *agent, int size, double winMult, 
		      int winInc, int ssthresh)
{
	nActive_++;
	if (!fixedIw_ || cwnd_ == 0)
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
	if (ownd_ <= -0.5)
		printf("In ok_to_snd(): ownd_ = %g  owndCorrection_ = %d\n", double(ownd_), owndCorrection_);
	return (cwnd_ >= ownd_+1);
}



