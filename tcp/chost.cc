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

CorresHost::CorresHost() : slink(), TcpFsAgent(), connWithPktBeforeFS_(NULL),
	dontAdjustOwnd_(0), pktReordered_(0), lastackTS_(0)
{
	nActive_ = nTimeout_ = nFastRec_ = 0;
	ownd_ = 0;
	owndCorrection_ = 0;
	closecwTS_ = 0;
	connIter_ = new Islist_iter<IntTcpAgent> (conns_);
	rtt_seg_ = NULL;
}


/*
 * Open up the congestion window.
 */
void 
CorresHost::opencwnd(int size, IntTcpAgent *sender)
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
CorresHost::closecwnd(int how, double ts, IntTcpAgent *sender)
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
CorresHost::closecwnd(int how, IntTcpAgent *sender)
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
		ssthresh_ = int(cwnd_ * winMult_);
/*		ssthresh_ = int( ownd_ * winMult_ );*/
		cwnd_ = int(wndInit_);
		break;
		
	case 1:
		/* Reno dup acks, or after a recent congestion indication. */
/*		cwnd_ = ownd_ * winMult_;*/
		cwnd_ *= winMult_;
		ssthresh_ = int(cwnd_);
		if (ssthresh_ < 2)
			ssthresh_ = 2;
		break;

	case 11:
		/* Reno dup acks, or after a recent congestion indication. */
		/* XXX fix this -- don't make it dependent on ownd */
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
	class Segment *news;

	ownd_ += 1;
	news = new Segment;
	news->seqno_ = seqno;
	news->sessionSeqno_ = sessionSeqno;
	news->daddr_ = daddr;
	news->dport_ = dport;
	news->sport_ = sport;
	news->ts_ = ts;
	news->size_ = 1;
	news->dupacks_ = 0;
	news->later_acks_ = 0;
	news->thresh_dupacks_ = 0;
	news->partialack_ = 0;
	news->rxmitted_ = 0;
	news->sender_ = sender;
	seglist_.append(news);
	return news;
}

void
CorresHost::adjust_ownd(int size) 
{
	if (double(owndCorrection_) < size)
		ownd_ -= min(double(ownd_), size - double(owndCorrection_));
	owndCorrection_ -= min(double(owndCorrection_),size);
	if (double(ownd_) < -0.5 || double(owndCorrection_ < -0.5))
		printf("In adjust_ownd(): ownd_ = %g  owndCorrection_ = %d\n", double(ownd_), double(owndCorrection_));
}


int
CorresHost::clean_segs(int size, Packet *pkt, IntTcpAgent *sender, int sessionSeqno, int amt_data_acked)
{ 
    Segment *cur, *prev=NULL, *newseg;
    int rval = -1;

    /* remove all acked pkts from list */
    int latest_susp_loss = rmv_old_segs(pkt, sender, amt_data_acked);
    /* 
     * XXX If no new data is acked and the last time we shrunk the window 
     * covers the most recent suspected loss, update the estimate of the amount
     * of outstanding data.
     */
    if (amt_data_acked == 0 && latest_susp_loss <= recover_ && 
	!dontAdjustOwnd_ && last_cwnd_action_ != CWND_ACTION_TIMEOUT) {
	    owndCorrection_ += min(double(ownd_),1);
	    ownd_ -= min(double(ownd_),1);
    }
    /*
     * A pkt is a candidate for retransmission if it is the leftmost one in the
     * unacked window for the connection AND has at least NUMDUPACKS
     * dupacks + later acks AND (at least one dupack OR a later packet also
     * with the threshold number of dup/later acks OR a packet sent 2*200 = 400ms
     * later has been acked). A pkt is also a candidate for immediate 
     * retransmission if it has partialack_ set, indicating that a partial ack
     * has been received for it.
     */
    Islist_iter<Segment> seg_iter(seglist_);
    while ((cur = seg_iter()) != NULL) {
	int remove_flag = 0;
	if (cur->seqno_ == cur->sender_->highest_ack_ + 1 &&
	    ((cur->dupacks_ + cur->later_acks_ >= NUMDUPACKS &&
	      (cur->dupacks_ > 0 || sender == cur->sender_ ||
	       cur->sender_->num_thresh_dupack_segs_ > 1 ||
	       lastackTS_-cur->ts_ >= 0.2*2)) || cur->partialack_)) {
		if (cur->sessionSeqno_ <= recover_ &&
		    last_cwnd_action_ != CWND_ACTION_TIMEOUT)
			dontAdjustOwnd_ = 1;
		if ((cur->sessionSeqno_ > recover_) ||
		    (last_cwnd_action_ == CWND_ACTION_TIMEOUT) ||
		    (proxyopt_ && cur->seqno_ > cur->sender_->recover_) ||
		    (proxyopt_ && cur->sender_->last_cwnd_action_ == CWND_ACTION_TIMEOUT)) { 
			/* new loss window */
			closecwnd(1, cur->ts_, cur->sender_);
			recover_ = sessionSeqno - 1;
			last_cwnd_action_ = CWND_ACTION_DUPACK;
			cur->sender_->recover_ = cur->sender_->maxseq_;
			cur->sender_->last_cwnd_action_ = CWND_ACTION_DUPACK;
			dontAdjustOwnd_ = 0;
		}
		if (newseg = cur->sender_->rxmit_last(TCP_REASON_DUPACK, 
			      cur->seqno_, cur->sessionSeqno_, cur->ts_)) {
			newseg->rxmitted_ = 1;
			adjust_ownd(cur->size_);
			if (!dontAdjustOwnd_) {
				owndCorrection_ += min(double(ownd_),cur->dupacks_);
				ownd_ -= min(double(ownd_),cur->dupacks_);
			}
			seglist_.remove(cur, prev);
			remove_flag = 1;
			delete cur;
		}
	}
	if (!remove_flag)
		prev = cur;
    }
    return(0);
}




int
CorresHost::rmv_old_segs(Packet *pkt, IntTcpAgent *sender, int amt_data_acked)
{
	Islist_iter<Segment> seg_iter(seglist_);
	Segment *cur, *prev=0;
	int found = 0;
	int head = 1;
	int new_data_acked = 0;
	int partialack = 0;
	int latest_susp_loss = -1;
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	if (tcph->ts_echo() > lastackTS_)
		lastackTS_ = tcph->ts_echo();

	while ((!found) && ((cur = seg_iter()) != NULL)) {
		int remove_flag = 0;
		/* ack for older pkt of another connection */
		if (sender != cur->sender_ && tcph->ts_echo() > cur->ts_) {
			cur->later_acks_++;
			latest_susp_loss = 
				max(latest_susp_loss,cur->sessionSeqno_);
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
			/* partial new ack => rexmt immediately */
			else if (amt_data_acked > 0 && 
				 tcph->seqno() == cur->seqno_-1 &&
				 cur->seqno_ <= sender->recover_ &&
				 sender->last_cwnd_action_ == CWND_ACTION_DUPACK) {
				cur->partialack_ = 1;
				partialack = 1;
				latest_susp_loss = 
					max(latest_susp_loss,cur->sessionSeqno_);
			}
			/* 
			 * If no new data has been acked AND this segment has
			 * not been retransmitted before AND the ack indicates 
			 * that this is the next segment to be acked, then
			 * increment dupack count.
			 */
			else if (!new_data_acked && !cur->rxmitted_ &&
				 tcph->seqno() == cur->seqno_-1) {

				/* cur->later_acks_++;*/
				cur->dupacks_++;
				latest_susp_loss = 
					max(latest_susp_loss,cur->sessionSeqno_);
			} 
		}
		if (cur->dupacks_+cur->later_acks_ >= NUMDUPACKS &&
		    !cur->thresh_dupacks_) {
			cur->thresh_dupacks_ = 1;
			cur->sender_->num_thresh_dupack_segs_++;
		}
		/* check if there is packet reordering */
		if (head && !new_data_acked)
			pktReordered_ = 1;
		/* XXX we could check for rexmt candidates here if we ignore 
		   the num_thresh_dupack_segs_ check */
		if (!remove_flag)
			prev = cur;
	}
	/* partial ack => terminate fast start mode */
	if (partialack && fs_enable_ && fs_mode_) 
		timeout(TCP_TIMER_RESET);
/*	return new_data_acked;*/
	return latest_susp_loss;
}
	
void
CorresHost::add_agent(IntTcpAgent *agent, int size, double winMult, 
		      int winInc, int ssthresh)
{
	nActive_++;
	if ((!fixedIw_ && nActive_ > 1) || cwnd_ == 0)
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
		printf("In ok_to_snd(): ownd_ = %g  owndCorrection_ = %g\n", double(ownd_), double(owndCorrection_));
	return (cwnd_ >= ownd_+1);
}



