/*
 * Copyright (c) 1997, 1998 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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

/*
 * This code below was motivated in part by code contributed by
 * Kathie Nichols (nichols@baynetworks.com).  The code below is based primarily
 * on the 4.4BSD TCP implementation. -KF [kfall@ee.lbl.gov]
 *
 * Kathie Nichols and Van Jacobson have contributed significant bug fixes,
 * especially with respect to the the handling of sequence numbers during
 * connection establishment/clearin.  Additional fixes have followed
 * theirs.
 *
 * Some warnings and comments:
 *	this version of TCP will not work correctly if the sequence number
 *	goes above 2147483648 due to sequence number wrap
 *
 *	this version of TCP by default sends data at the beginning of a
 *	connection in the "typical" way... That is,
 *		A   ------> SYN ------> B
 *		A   <----- SYN+ACK ---- B
 *		A   ------> ACK ------> B
 *		A   ------> data -----> B
 *
 *	there is no dynamic receiver's advertised window.   The advertised
 *	window is simulated by simply telling the sender a bound on the window
 *	size (wnd_).
 *
 *	in real TCP, a user process performing a read (via PRU_RCVD)
 *		calls tcp_output each time to (possibly) send a window
 *		update.  Here we don't have a user process, so we simulate
 *		a user process always ready to consume all the receive buffer
 *
 * Notes:
 *	wnd_, wnd_init_, cwnd_, ssthresh_ are in segment units
 *	sequence and ack numbers are in byte units
 *
 * Futures:
 *	there are different existing TCPs with respect to how
 *	ack's are handled on connection startup.  Some delay
 *	the ack for the first segment, which can cause connections
 *	to take longer to start up than if we be sure to ack it quickly.
 *
 *	SACK
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-full.cc,v 1.30 1998/01/22 02:32:53 kfall Exp $ (LBL)";
#endif

#include "tclcl.h"
#include "ip.h"
#include "tcp-full.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#define	TRUE 	1
#define	FALSE 	0

static class FullTcpClass : public TclClass { 
public:
	FullTcpClass() : TclClass("Agent/TCP/FullTcp") {}
	TclObject* create(int, const char*const*) { 
		return (new FullTcpAgent());
	}
} class_full;

/*
 * Tcl bound variables:
 *	segsperack: for delayed ACKs, how many to wait before ACKing
 *	segsize: segment size to use when sending
 */
FullTcpAgent::FullTcpAgent() : delack_timer_(this), flags_(0),
	state_(TCPS_CLOSED), last_state_(TCPS_CLOSED),
	rq_(rcv_nxt_), last_ack_sent_(-1),
	last_send_time_(0.0), irs_(-1), delay_growth_(0)
{
	bind("segsperack_", &segs_per_ack_);
	bind("segsize_", &maxseg_);
	bind("tcprexmtthresh_", &tcprexmtthresh_);
	bind("iss_", &iss_);
	bind_bool("nodelay_", &nodelay_);
	bind_bool("data_on_syn_",&data_on_syn_);
	bind_bool("dupseg_fix_", &dupseg_fix_);
	bind_bool("dupack_reset_", &dupack_reset_);
	bind_bool("close_on_empty_", &close_on_empty_);
	bind_bool("delay_growth_", &delay_growth_);
	bind("interval_", &delack_interval_);

	reset();
}

/*
 * reset to starting point, don't set state_ here,
 * because our starting point might be LISTEN rather
 * than CLOSED if we're a passive opener
 */
void
FullTcpAgent::reset()
{
	cancel_timers();	// cancel timers first
	TcpAgent::reset();	// resets most variables
	rq_.clear();
	rtt_init();

	last_ack_sent_ = -1;
	rcv_nxt_ = -1;
	flags_ = 0;
	t_seqno_ = iss_;
	maxseq_ = -1;
	irs_ = -1;
	last_send_time_ = 0.0;
	ts_peer_ = 0.0;

	recent_ = recent_age_ = 0.0;
}

/*
 * headersize:
 *	how big is an IP+TCP header in bytes
 *	(for now, is the basic size, but may changes
 *	 in the future w/options; fix for sack)
 */
int
FullTcpAgent::headersize()
{
	return (TCPIP_BASE_PKTSIZE);
}

/*
 * free up the reassembly queue if there's anything there
 */
FullTcpAgent::~FullTcpAgent()
{
	cancel_timers();	// cancel all pending timers
	rq_.clear();		// free mem in reassembly queue
}

/*
 * the 'advance' interface to the regular tcp is in packet
 * units.  Here we scale this to bytes for full tcp.
 *
 * 'advance' is normally called by an "application" (i.e. data source)
 * to signal that there is something to send
 *
 * 'curseq_' is the last byte number provided by the application
 */
void
FullTcpAgent::advanceby(int np)
{
	// XXX hack:
	//	because np is in packets and a data source
	//	may pass a *huge* number as a way to tell us
	//	to go forever, just look for the huge number
	//	and if it's there, pre-divide it
	if (np >= 0x10000000)
		np /= maxseg_;

	advance_bytes(np * maxseg_);
	return;
}

/*
 * the byte-oriented interface
 */

void    
FullTcpAgent::advance_bytes(int nb)
{

	//
	// state-specific operations:
	//	if CLOSED, reset and try a new active open/connect
	//	if ESTABLISHED, just try to send more
	//	if above ESTABLISHED, we are closing, so don't allow
	//	if anything else (establishing), do nothing here
	//

	if (state_ > TCPS_ESTABLISHED) {
		fprintf(stderr,
		 "%f: FullTcpAgent::advance(%s): cannot advance while in state %d\n",
		 now(), name(), state_);
		return;
	} else if (state_ == TCPS_CLOSED) {
		reset();
		curseq_ = iss_ + nb;
		connect();		// initiate new connection
	} else if (state_ == TCPS_ESTABLISHED) {
		curseq_ += nb;
		send_much(0, REASON_NORMAL, 0);
	}
	return;
}


/*
 * flags that are completely dependent on the tcp state
 * (in real TCP, see tcp_fsm.h, the "tcp_outflags" array)
 */
int FullTcpAgent::outflags()
{
	int flags = 0;
	if ((state_ != TCPS_LISTEN) && (state_ != TCPS_SYN_SENT))
		flags |= TH_ACK;

	if ((state_ == TCPS_SYN_SENT) || (state_ == TCPS_SYN_RECEIVED))
		flags |= TH_SYN;

	if ((state_ == TCPS_FIN_WAIT_1) || (state_ == TCPS_LAST_ACK))
		flags |= TH_FIN;

	return (flags);
}

/*
 * utility function to set rcv_next_ during inital exchange of seq #s
 */

int FullTcpAgent::rcvseqinit(int seq, int dlen)
{
	return (seq + dlen + 1);
}

/*
 * sendpacket: 
 *	allocate a packet, fill in header fields, and send
 *	also keeps stats on # of data pkts, acks, re-xmits, etc
 *
 * fill in packet fields.  Agent::allocpkt() fills
 * in most of the network layer fields for us.
 * So fill in tcp hdr and adjust the packet size.
 */
void FullTcpAgent::sendpacket(int seqno, int ackno, int pflags, int datalen, int reason)
{
        Packet* p = allocpkt();
        hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
        hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
        tcph->seqno() = seqno;
        tcph->ackno() = ackno;
        tcph->flags() = pflags;
        tcph->hlen() = headersize();
        tcph->ts() = now();
	tcph->ts_echo() = ts_peer_;

	/* Open issue:  should tcph->reason map to pkt->flags_ as in ns-1?? */
        tcph->reason() |= reason;
        th->size() = datalen + headersize();
        if (datalen <= 0)
                ++nackpack_;
        else {
                ++ndatapack_;
                ndatabytes_ += datalen;
        }
        if (reason == REASON_TIMEOUT || reason == REASON_DUPACK) {
                ++nrexmitpack_;
                nrexmitbytes_ += datalen;
        }

	last_send_time_ = now();
        send(p, 0);
}

void FullTcpAgent::cancel_timers()
{

	TcpAgent::cancel_timers();      
	delack_timer_.force_cancel();
}


/*
 * see if we should send a segment, and if so, send it
 * (may be ACK or data)
 *
 * simulator var, desc (name in real TCP)
 * --------------------------------------
 * maxseq_, largest seq# we've sent plus one (snd_max)
 * flags_, flags regarding our internal state (t_state)
 * pflags, a local used to build up the tcp header flags (flags)
 * curseq_, is the highest sequence number given to us by "application"
 * highest_ack_, the highest ACK we've seen for our data (snd_una-1)
 * seqno, the next seq# we're going to send (snd_nxt), this will
 *	update t_seqno_ (the last thing we sent)
 */
void FullTcpAgent::output(int seqno, int reason)
{
	int is_retransmit = (seqno < maxseq_);
	int idle = (highest_ack_ == maxseq_);
	int buffered_bytes = (curseq_ + iss_) - highest_ack_;
	int win = window() * maxseg_;
	int off = seqno - highest_ack_;
	int datalen = min(buffered_bytes, win) - off;
	int pflags = outflags();
	int emptying_buffer = FALSE;

	//
	// this is an option that causes us to slow-start if we've
	// been idle for a "long" time, where long means a rto or longer
	// the slow-start is a sort that does not set ssthresh
	//

	if (slow_start_restart_ && idle) {
		if (idle_restart())
			closecwnd(3);
	}

	//
	// in real TCP datalen (len) could be < 0 if there was window
	// shrinkage, or if a FIN has been sent and neither ACKd nor
	// retransmitted.  Only this 2nd case concerns us here...
	//
	if (datalen < 0) {
		datalen = 0;
	} else if (datalen > maxseg_) {
		datalen = maxseg_;
	}

	//
	// see if sending this packet will empty the send buffer
	//
	if ((seqno + datalen) >= (curseq_ + iss_))
		emptying_buffer = TRUE;
		
	if (emptying_buffer) {
		pflags |= TH_PUSH;
		//
		// if close_on_empty set, we are finished
		// with this connection; close it
		//
		if (close_on_empty_) {
			pflags |= TH_FIN;
			newstate(TCPS_FIN_WAIT_1);
		}
	} else  {
		/* not emptying buffer, so can't be FIN */
		pflags &= ~TH_FIN;
	}

	/* sender SWS avoidance (Nagle) */

	if (datalen > 0) {
		// if full-sized segment, ok
		if (datalen == maxseg_)
			goto send;
		// if Nagle disabled and buffer clearing, ok
		if ((idle || nodelay_)  && emptying_buffer)
			goto send;
		// if a retransmission
		if (is_retransmit)
			goto send;
		// if big "enough", ok...
		//	(this is not a likely case, and would
		//	only happen for tiny windows)
		if (datalen >= ((wnd_ * maxseg_) / 2.0))
			goto send;
	}

	if (need_send())
		goto send;

	/*
	 * send now if a SYN or special flag "TF_ACKNOW" is set.
	 * TF_ACKNOW can be set during connection establishment and
	 * to generate acks for out-of-order data
	 */
	// K: K has PUSH here also...
	//	but if nodelay is enabled, the SWS check above should
	//	pushd the packet out now.
	if ((flags_ & TF_ACKNOW) || (pflags & (TH_SYN|TH_FIN)))
		goto send;

	return;		// no reason to send now

send:
	if (pflags & TH_SYN) {
		seqno = iss_;
		if (!data_on_syn_)
			datalen = 0;
	}

	/*
	 * if this is a retransmission of a FIN, pre-adjust the
	 * sequence number so we don't go above maxseq_
	 */

	if (pflags & TH_FIN && flags_ & TF_SENTFIN && seqno == maxseq_)
		--t_seqno_;

	/*
	 * SYNs and FINs each use up one sequence number
	 */

	int ctrl;
	if (ctrl = (pflags & (TH_SYN|TH_FIN))) {
		if (pflags & TH_SYN) {
                        ++t_seqno_;
		}
		if (pflags & TH_FIN) {
                        ++t_seqno_;
                        flags_ |= TF_SENTFIN;
		}
	}

	sendpacket(seqno, rcv_nxt_, pflags, datalen, reason);
	last_ack_sent_ = rcv_nxt_;

        /*      
         * Data sent (as far as we can tell).
         * Any pending ACK has now been sent.
         */      
	flags_ &= ~(TF_ACKNOW|TF_DELACK);

	if (seqno == t_seqno_)
		t_seqno_ += datalen;	// update snd_nxt (t_seqno_)

	// highest: greatest sequence number sent + 1
	//	and adjusted for SYNs and FINs which use up one number
	int highest = seqno + datalen + (ctrl ? 1 : 0);
	if (highest > maxseq_) {
		maxseq_ = highest;
		//
		// if we are using conventional RTT estimation,
		// establish timing on this segment
		//
		if (!ts_option_ && rtt_active_ == FALSE) {
//printf("%f: tcp(%s): starting rtt timer on seq:%d\n",
//now(), name(), seqno);
			rtt_active_ = TRUE;	// set timer
			rtt_seq_ = seqno; 	// timed seq #
			rtt_ts_ = now();	// when set
		}
	}
	/*
	 * Set retransmit timer if not currently set,
	 * and not doing an ack or a keep-alive probe.
	 * Initial value for retransmit timer is smoothed
	 * round-trip time + 2 * round-trip time variance.
	 * Future values are rtt + 4 * rttvar.
	 */
	if ((rtx_timer_.status() != TIMER_PENDING) && (t_seqno_ > highest_ack_)) {
		set_rtx_timer();  // no timer pending, schedule one
	}

	return;
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void FullTcpAgent::send_much(int force, int reason, int maxburst)
{
	/*
	 * highest_ack is essentially "snd_una" in real TCP
	 *
	 * loop while we are in-window (seqno <= (highest_ack + win))
	 * and there is something to send (t_seqno_ < curseq_+iss_)
	 */
	int win = window() * maxseg_;	// window() in pkts
	int npackets = 0;
	int topwin = curseq_ + iss_;
	if (topwin > highest_ack_ + win)
		topwin = highest_ack_ + win;

	if (!force && (delsnd_timer_.status() == TIMER_PENDING))
		return;

	/*
	 * note that if output() doesn't actually send anything, we can
	 * loop forever here
	 */
	while (force || (t_seqno_ < topwin)) {
		if (overhead_ != 0 && (delsnd_timer_.status() != TIMER_PENDING)) {
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		/*
		 * note that if output decides to not actually send
		 * (e.g. because of Nagle), then if we don't break out
		 * of this loop, we can loop forever at the same
		 * simulated time instant
		 */
		int save = t_seqno_;		// scrawl away current t_seqno
		output(t_seqno_, reason);	// updates t_seqno for us
		if (t_seqno_ == save) {		// progress?
			break;
		}

		force = 0;

		if ((outflags() & (TH_SYN|TH_FIN)) ||
		    (maxburst && ++npackets >= maxburst))
			break;
	}
	return;
}

/*
 * Process an ACK
 *	this version of the routine doesn't necessarily
 *	require the ack to be one which advances the ack number
 *
 * if this ACKs a rtt estimate
 *	indicate we are not timing
 *	reset the exponential timer backoff (gamma)
 * update rtt estimate
 * cancel retrans timer if everything is sent and ACK'd, else set it
 * advance the ack number if appropriate
 * update segment to send next if appropriate
 */
void FullTcpAgent::newack(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	register int ackno = tcph->ackno();
	int progress = (ackno > highest_ack_);

	if (ackno == maxseq_) {
		cancel_rtx_timer();	// all data ACKd
	} else if (progress) {
		set_rtx_timer();
	}

	// advance the ack number if this is for new data
	if (progress)
		highest_ack_ = ackno;

	// if we have suffered a retransmit timeout, t_seqno_
	// will have been reset to highest_ ack.  If the
	// receiver has cached some data above t_seqno_, the
	// new-ack value could (should) jump forward.  We must
	// update t_seqno_ here, otherwise we would be doing
	// go-back-n.

	if (t_seqno_ < highest_ack_)
		t_seqno_ = highest_ack_; // seq# to send next

        /*
         * Update RTT only if it's OK to do so from info in the flags header.
         * This is needed for protocols in which intermediate agents
         * in the network intersperse acks (e.g., ack-reconstructors) for
         * various reasons (without violating e2e semantics).
         */
        hdr_flags *fh = (hdr_flags *)pkt->access(off_flags_);
        if (!fh->no_ts_) {
		ts_peer_ = tcph->ts();	// always set (even if ts_option_ == 0)
//printf("%f: tcp(%s): wanna do an update: ts_opt:%d, ack:%d, rtseq:%d\n",
//now(), name(), ts_option_, ackno, rtt_seq_);

                if (ts_option_) {
                        rtt_update(now() - tcph->ts_echo());
		} else if (rtt_active_ && ackno > rtt_seq_) {
			// got an RTT sample, record it
                        t_backoff_ = 1;
                        rtt_active_ = FALSE;
			rtt_update(now() - rtt_ts_);
                }
        }
	return;
}

/*
 * this is the simulated form of the header prediction
 * predicate.  While not really necessary for a simulation, it
 * follows the code base more closely and can sometimes help to reveal
 * odd behavior caused by the implementation structure..
 */
int FullTcpAgent::predict_ok(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
        hdr_flags *fh = (hdr_flags *)pkt->access(off_flags_);

	int p1 = (state_ == TCPS_ESTABLISHED);		// ready
	int p2 = (tcph->flags() == TH_ACK);		// is an ACK
	int p3 = (tcph->seqno() == rcv_nxt_);		// in-order data
	int p4 = (t_seqno_ == maxseq_);			// not re-xmit
	int p5 = (fh->ecn_ == 0);			// no ECN

		// no timestamp, or no ts in this pkt, or ok ts
	int p6 = (!ts_option_ || fh->no_ts_ || (tcph->ts() >= recent_));

	return (p1 && p2 && p3 && p4 && p5 && p6);
}

/*
 * fast_retransmit using the given seqno
 *	perform a fast retransmit
 *	kludge t_seqno_ (snd_nxt) so we do the
 *	retransmit then continue from where we were
 */

void FullTcpAgent::fast_retransmit(int seq)
{
	int onxt = t_seqno_;		// output() changes t_seqno_
	recover_ = maxseq_;		// keep a copy of highest sent
	recover_cause_ = REASON_DUPACK;	// why we started this recovery period
	output(seq, REASON_DUPACK);	// send one pkt
	t_seqno_ = onxt;
}

/*
 * real tcp determines if the remote
 * side should receive a window update/ACK from us, and often
 * results in sending an update every 2 segments, thereby
 * giving the familiar 2-packets-per-ack behavior of TCP.
 * Here, we don't advertise any windows, so we just see if
 * there's at least 'segs_per_ack_' pkts not yet acked
 */

int FullTcpAgent::need_send()
{
	if (flags_ & TF_ACKNOW)
		return TRUE;
	return ((rcv_nxt_ - last_ack_sent_) >= (segs_per_ack_ * maxseg_));
}

/*
 * determine whether enough time has elapsed in order to
 * conclude a "restart" is necessary (e.g. a slow-start)
 *
 * for now, keep track of this similarly to how rtt_update() does
 */

int
FullTcpAgent::idle_restart()
{

	double tao = now() - last_send_time_;
	if (!ts_option_) {
                double tickoff = fmod(last_send_time_ + boot_time_,
			tcp_tick_);
                tao = int((tao + tickoff) / tcp_tick_) * tcp_tick_;
	}

	return (tao > t_rtxcur_);  // verify this
}


/*
 * main reception path - 
 * called from the agent that handles the data path below in its muxing mode
 * advance() is called when connection is established with size sent from
 * user/application agent
 */
void FullTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_cmn *th = (hdr_cmn*)pkt->access(off_cmn_);

	int needoutput = FALSE;
	int ourfinisacked = FALSE;
	int dupseg = FALSE;
	int todrop = 0;

	last_state_ = state_;

	int datalen = th->size() - tcph->hlen(); // # payload bytes
	int ackno = tcph->ackno();	// ack # from packet
	int tiflags = tcph->flags() ; // tcp flags from packet

	if (state_ == TCPS_CLOSED)
		goto drop;

	if (state_ != TCPS_LISTEN)
		dooptions(pkt);

	//
	// if we are using delayed-ACK timers and
	// no delayed-ACK timer is set, set one.
	// They are set to fire every 'interval_' secs, starting
	// at time t0 = (0.0 + k * interval_) for some k such
	// that t0 > now
	//
	if (delack_interval_ > 0.0 &&
	    (delack_timer_.status() != TIMER_PENDING)) {
		int last = int(now() / delack_interval_);
		delack_timer_.resched(delack_interval_ * (last + 1.0) - now());
	}


	// note predict_ok() false if PUSH bit on
	if (predict_ok(pkt)) {
		if (datalen == 0) {
			// check for a received pure ACK and that
			// our own cwnd doesn't limit us
			if (ackno > highest_ack_ && ackno < maxseq_
				&& cwnd_ >= wnd_) {
				newack(pkt);
				opencwnd();
				if ((curseq_ + iss_) > highest_ack_) {
					/* more to send */
					send_much(0, REASON_NORMAL, 0);
				}
				Packet::free(pkt);
				return;
			}
		} else if (ackno == highest_ack_ && rq_.empty()) {
			// check for pure incoming segment
			// the next data segment we're awaiting, and
			// that there's nothing sitting in the reassem-
			// bly queue
			// 	give to "application" here
			//	note: DELACK is inspected only by
			//	tcp_fasttimo() in real tcp.  Every 200 ms
			//	this routine scans all tcpcb's looking for
			//	DELACK segments and when it finds them
			//	changes DELACK to ACKNOW and calls tcp_output()
			rcv_nxt_ += datalen;
			flags_ |= TF_DELACK;
			//
			// special code here to simulate the operation
			// of a receiver who always consumes data,
			// resulting in a call to tcp_output
			Packet::free(pkt);
			if (need_send())
				send_much(1, REASON_NORMAL, 0);
			return;
		}
	}

	switch (state_) {

        /*
         * If the segment contains an ACK then it is bad and do reset.
         * If it does not contain a SYN then it is not interesting; drop it.
         * Otherwise initialize tp->rcv_nxt, and tp->irs, iss is already
	 * selected, and send a segment:
         *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
         * Initialize tp->snd_nxt to tp->iss.
         * Enter SYN_RECEIVED state, and process any other fields of this
         * segment in this state.
         */

	case TCPS_LISTEN:	/* awaiting peer's SYN */

		if (tiflags & TH_ACK) {
			goto dropwithreset;
		}
		if ((tiflags & TH_SYN) == 0) {
			goto drop;
		}

		/* must by a SYN at this point */
		flags_ |= TF_ACKNOW;
		newstate(TCPS_SYN_RECEIVED);
		irs_ = tcph->seqno();
		rcv_nxt_ = rcvseqinit(irs_, datalen);
		t_seqno_ = iss_;
		goto trimthenstep6;

        /*
         * If the state is SYN_SENT:
         *      if seg contains an ACK, but not for our SYN, drop the input.
         *      if seg does not contain SYN, then drop it.
         * Otherwise this is an acceptable SYN segment
         *      initialize tp->rcv_nxt and tp->irs
         *      if seg contains ack then advance tp->snd_una
         *      if SYN has been acked change to ESTABLISHED else SYN_RCVD state
         *      arrange for segment to be acked (eventually)
         *      continue processing rest of data/controls, beginning with URG
         */

	case TCPS_SYN_SENT:	/* we sent SYN, expecting SYN+ACK */
		/* drop if not a SYN */
		if ((tiflags & TH_SYN) == 0) {
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s): no SYN for our SYN(%d)\n",
			        now(), name(), int(maxseq_));
			goto drop;
		}

		/* drop if it's a SYN+ACK and the ack field is bad */
		if ((tiflags & TH_ACK) && ((ackno < (iss_+1)) ||
		    (ackno > maxseq_))) {
			// not an ACK for our SYN, discard
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s): bad ACK (%d) for our SYN(%d)\n",
			        now(), name(), int(ackno), int(maxseq_));
			goto dropwithreset;
		}

		/* looks like an ok SYN or SYN+ACK */
		cancel_rtx_timer();	// cancel timer on our 1st SYN
		irs_ = tcph->seqno();	// get initial recv'd seq #
		rcv_nxt_ = rcvseqinit(irs_, datalen);


		if (tiflags & TH_ACK) {
			// SYN+ACK (our SYN was acked)
			highest_ack_ = ackno;
			newstate(TCPS_ESTABLISHED);

#ifdef notdef
/*
 * if we didn't have to retransmit the SYN,
 * use its rtt as our initial srtt & rtt var.
 */
if (t_rtt_) {
	double tao = now() - tcph->ts();
	rtt_update(tao);
}
#endif

			/*
			 * if there's data, delay ACK; if there's also a FIN
			 * ACKNOW will be turned on later.
			 */
			if (datalen > 0) {
				flags_ |= TF_DELACK;	// data there: wait
			} else {
				flags_ |= TF_ACKNOW;	// ACK peer's SYN
			}
			// special to ns:
			//  generate pure ACK here.
			//  this simulates the ordinary connection establishment
			//  where the ACK of the peer's SYN+ACK contains
			//  no data.  This is typically caused by the way
			//  the connect() socket call works in which the
			//  entire 3-way handshake occurs prior to the app
			//  being able to issue a write() [which actually
			//  causes the segment to be sent].
			sendpacket(t_seqno_, rcv_nxt_, TH_ACK, 0, 0);
		} else {
			// SYN (no ACK) (simultaneous active opens)
			flags_ |= TF_ACKNOW;
			cancel_rtx_timer();
			newstate(TCPS_SYN_RECEIVED);
			/*
			 * decrement t_seqno_: we are sending a
			 * 2nd SYN (this time in the form of a
			 * SYN+ACK, so t_seqno_ will have been
			 * advanced to 2... reduce this
			 */
			t_seqno_--;
		}

trimthenstep6:
		/*
		 * advance the seq# to correspond to first data byte
		 */
		tcph->seqno()++;

		if (tiflags & TH_ACK)
			goto process_ACK;

		goto step6;
	}

	// check for redundant data at head/tail of segment
	//	note that the 4.4bsd [Net/3] code has
	//	a bug here which can cause us to ignore the
	//	perfectly good ACKs on duplicate segments.  The
	//	fix is described in (Stevens, Vol2, p. 959-960).
	//	This code is based on that correction.
	//
	// In addition, it has a modification so that duplicate segments
	// with dup acks don't trigger a fast retransmit when dupseg_fix_
	// is enabled.
	//
	// Yet one more modification: make sure that if the received
	//	segment had datalen=0 and wasn't a SYN or FIN that
	//	we don't turn on the ACKNOW status bit.  If we were to
	//	allow ACKNO to be turned on, normal pure ACKs that happen
	//	to have seq #s below rcv_nxt can trigger an ACK war by
	//	forcing us to ACK the pure ACKs
	//
	todrop = rcv_nxt_ - tcph->seqno();  // how much overlap?

	if (todrop > 0 && ((tiflags & (TH_SYN|TH_FIN)) || datalen > 0)) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			tcph->seqno()++;
			todrop--;
		}
		//
		// see Stevens, vol 2, p. 960 for this check;
		// this check is to see if we are dropping
		// more than this segment (i.e. the whole pkt + a FIN),
		// or just the whole packet (no FIN)
		//
		if (todrop > datalen ||
		    (todrop == datalen && (tiflags & TH_FIN) == 0)) {
                        /*
                         * Any valid FIN must be to the left of the window.
                         * At this point the FIN must be a duplicate or out
                         * of sequence; drop it.
                         */

			tiflags &= ~TH_FIN;

			/*
			 * Send an ACK to resynchronize and drop any data.
			 * But keep on processing for RST or ACK.
			 */

			flags_ |= TF_ACKNOW;
			todrop = datalen;
			dupseg = TRUE;
		}
		tcph->seqno() += todrop;
		datalen -= todrop;
	}

	if (tiflags & TH_SYN) {
		fprintf(stderr,
		    "%f: FullTcpAgent::recv(%s) received unexpected SYN (state:%d)\n",
		        now(), name(), state_);
		goto dropwithreset;
	}

	// K: added TH_SYN, but it is already known here!
	if ((tiflags & TH_ACK) == 0) {
		fprintf(stderr, "%f: FullTcpAgent::recv(%s) got packet lacking ACK (seq %d)\n",
			now(), name(), tcph->seqno());
		goto drop;
	}

	/*
	 * ACK processing
	 */

	switch (state_) {
	case TCPS_SYN_RECEIVED:	/* want ACK for our SYN+ACK */
		if (ackno < highest_ack_ || ackno > maxseq_) {
			// not in useful range
			goto dropwithreset;
		}
		newstate(TCPS_ESTABLISHED);
		/* fall into ... */

        /*
         * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
         * ACKs.  If the ack is in the range
         *      tp->snd_una < ti->ti_ack <= tp->snd_max
         * then advance tp->snd_una to ti->ti_ack and drop
         * data from the retransmission queue.
	 *
	 * note that states CLOSE_WAIT and TIME_WAIT aren't used
	 * in the simulator
         */

        case TCPS_ESTABLISHED:
        case TCPS_FIN_WAIT_1:
        case TCPS_FIN_WAIT_2:
        case TCPS_CLOSING:
        case TCPS_LAST_ACK:

		// look for dup ACKs (dup ack numbers, no data)
		//
		// do fast retransmit/recovery if at/past thresh
		if (ackno <= highest_ack_) {
			// an ACK which doesn't advance highest_ack_
			if (datalen == 0 && (!dupseg_fix_ || !dupseg)) {
                                /*
                                 * If we have outstanding data
                                 * this is a completely
                                 * duplicate ack,
                                 * the ack is the biggest we've
                                 * seen and we've seen exactly our rexmt
                                 * threshhold of them, assume a packet
                                 * has been dropped and retransmit it.
                                 *
                                 * We know we're losing at the current
                                 * window size so do congestion avoidance.
                                 *
                                 * Dup acks mean that packets have left the
                                 * network (they're now cached at the receiver)
                                 * so bump cwnd by the amount in the receiver
                                 * to keep a constant cwnd packets in the
                                 * network.
                                 */

				if ((rtx_timer_.status() != TIMER_PENDING) ||
				    ackno != highest_ack_) {
					// not timed, or re-ordered ACK
					dupacks_ = 0;
				} else if (++dupacks_ == tcprexmtthresh_) {
					// possibly trigger fast retransmit
					if (!bug_fix_ ||
					   (highest_ack_ > recover_) ||
					   (recover_cause_ != REASON_TIMEOUT)) {
						closecwnd(1);
						cancel_rtx_timer();
						rtt_active_ = FALSE;
						fast_retransmit(ackno);
						// we measure cwnd in packets,
						// so don't scale by maxseg_
						// as real TCP does
						cwnd_ = ssthresh_ + dupacks_;
					}
					goto drop;
				} else if (dupacks_ > tcprexmtthresh_) {
					// we just measure cwnd in packets,
					// so don't scale by maxset_ as real
					// tcp does
					cwnd_++;	// fast recovery
					send_much(0, REASON_NORMAL, 0);
					goto drop;
				}
			} else {
				// non-zero length segment
				// (or window changed in real TCP).
				if (dupack_reset_)
					dupacks_ = 0;
			}
			break;	/* take us to "step6" */
		}

		/*
		 * we've finished the fast retransmit/recovery period
		 * (i.e. received an ACK which advances highest_ack_)
		 */

                /*
                 * If the congestion window was inflated to account
                 * for the other side's cached packets, retract it.
                 */

		if (dupacks_ > tcprexmtthresh_ && cwnd_ > ssthresh_) {
			// this is where we can get frozen due
			// to multiple drops in the same window of
			// data
			cwnd_ = ssthresh_;
		}
		dupacks_ = 0;
		if (ackno > maxseq_) {
			// ack more than we sent(!?)
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s) too-big ACK (ack: %d, maxseq:%d)\n",
				now(), name(), int(ackno), int(maxseq_));
			goto dropafterack;
		}

process_ACK:

                /*
                 * If we have a timestamp reply, update smoothed
                 * round trip time.  If no timestamp is present but
                 * transmit timer is running and timed sequence
                 * number was acked, update smoothed round trip time.
                 * Since we now have an rtt measurement, cancel the
                 * timer backoff (cf., Phil Karn's retransmit alg.).
                 * Recompute the initial retransmit timer.
		 *
                 * If all outstanding data is acked, stop retransmit
                 * If there is more data to be acked, restart retransmit
                 * timer, using current (possibly backed-off) value.
                 */
		newack(pkt);
                /*
                 * If no data (only SYN) was ACK'd,
                 *    skip rest of ACK processing.
                 */
		if (ackno == (highest_ack_ + 1))
			goto step6;

		if (ackno > maxseq_)
			needoutput = TRUE;
		// if we are delaying initial cwnd growth (probably due to
		// large initial windows), then only open cwnd if data has
		// been received
		if ((!delay_growth_ || (rcv_nxt_ > 0)) &&
			last_state_ == TCPS_ESTABLISHED) {
			opencwnd();
		}
		// K: added state check in equal but diff way
		if ((state_ >= TCPS_FIN_WAIT_1) && (ackno == maxseq_)) {
			ourfinisacked = TRUE;
		} else {
			ourfinisacked = FALSE;
		}
		// additional processing when we're in special states

		switch (state_) {
                /*
                 * In FIN_WAIT_1 STATE in addition to the processing
                 * for the ESTABLISHED state if our FIN is now acknowledged
                 * then enter FIN_WAIT_2.
                 */
		case TCPS_FIN_WAIT_1:	/* doing active close */
			if (ourfinisacked)
				newstate(TCPS_FIN_WAIT_2);
			break;

                /*
                 * In CLOSING STATE in addition to the processing for
                 * the ESTABLISHED state if the ACK acknowledges our FIN
                 * then enter the TIME-WAIT state, otherwise ignore
                 * the segment.
                 */
		case TCPS_CLOSING:	/* simultaneous active close */;
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				cancel_timers();
			}
			break;
                /*
                 * In LAST_ACK, we may still be waiting for data to drain
                 * and/or to be acked, as well as for the ack of our FIN.
                 * If our FIN is now acknowledged,
                 * enter the closed state and return.
                 */
		case TCPS_LAST_ACK:	/* passive close */
			// K: added state change here
			if (ourfinisacked) {
				newstate(TCPS_CLOSED);
				cancel_timers();
				goto drop;
			} else {
				// should be a FIN we've seen
				hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
                                fprintf(stderr,
                                "%f: %d.%d>%d.%d FullTcpAgent::recv(%s) received non-ACK (state:%d)\n",
                                        now(),
                                        iph->src() >> 8, iph->src() & 0xff,
                                        iph->dst() >> 8, iph->dst() & 0xff,
                                        name(), state_);
                        }
			break;

		/* no case for TIME_WAIT in simulator */
		}  // inner switch
	} // outer switch

step6:
	/* real TCP handles window updates and URG data here */
/* dodata: this label is in the "real" code.. here only for reference */
	/*
	 * DATA processing
	 */

	if ((datalen > 0 || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(state_) == 0) {
		if (tcph->seqno() == rcv_nxt_ && rq_.empty()) {
			// see the "TCP_REASS" macro for this code:
			// got the in-order packet we were looking
			// for, nobody is in the reassembly queue,
			// so this is the common case...
			// note: in "real" TCP we must also be in
			// ESTABLISHED state to come here, because
			// data arriving before ESTABLISHED is
			// queued in the reassembly queue.  Since we
			// don't really have a process anyhow, just
			// accept the data here as-is (i.e. don't
			// require being in ESTABLISHED state)
			flags_ |= TF_DELACK;
			rcv_nxt_ += datalen;
			tiflags = tcph->flags() & TH_FIN;
			// give to "application" here
			needoutput = need_send();
		} else {
			// see the "tcp_reass" function:
			// not the one we want next (or it
			// is but there's stuff on the reass queue);
			// do whatever we need to do for out-of-order
			// segments or hole-fills.  Also,
			// send an ACK to the other side right now.
			// K: some changes here, figure out
			tiflags = rq_.add(pkt);
			if (tiflags & TH_PUSH) {
				// K: APPLICATION recv
				needoutput = need_send();
			} else {
				flags_ |= TF_ACKNOW;
			}
		}
	} else {
		/*
		 * we're closing down or this is a pure ACK that
		 * wasn't handled by the header prediction part above
		 * (e.g. because cwnd < wnd)
		 */
		// K: this is deleted
		tiflags &= ~TH_FIN;
	}

	/*
	 * if FIN is received, ACK the FIN
	 * (let user know if we could do so)
	 */

	if (tiflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(state_) == 0) {
			flags_ |= TF_ACKNOW;
			rcv_nxt_++;
		}
		rq_.clear();	// other side shutting down
		switch (state_) {
                /*
                 * In SYN_RECEIVED and ESTABLISHED STATES
                 * enter the CLOSE_WAIT state.
		 * (in the simulator, go to LAST_ACK)
		 * (passive close)
		 *
		 * special to ns:
		 * Because there is no CLOSE_WAIT state, the
		 * ACK that should be generated for the FIN is
		 * performed directly here..  This code generates
		 * a pure ACK for the FIN, although strictly speaking
		 * it appears to be "TCP-legal" to have data in this packet.
                 */
                case TCPS_SYN_RECEIVED:
                case TCPS_ESTABLISHED:
			sendpacket(t_seqno_, rcv_nxt_, TH_ACK, 0, 0);
			newstate(TCPS_LAST_ACK);
                        break;

                /*
                 * If still in FIN_WAIT_1 STATE FIN has not been acked so
                 * enter the CLOSING state.
		 * (simultaneous close)
                 */
                case TCPS_FIN_WAIT_1:
			newstate(TCPS_CLOSING);
                        break;
                /*
                 * In FIN_WAIT_2 state enter the TIME_WAIT state,
                 * starting the time-wait timer, turning off the other
                 * standard timers.
		 * (in the simulator, just go to CLOSED)
		 * (completion of active close)
                 */
                case TCPS_FIN_WAIT_2:
			sendpacket(t_seqno_, rcv_nxt_, TH_ACK, 0, 0);
                        newstate(TCPS_CLOSED);
			cancel_timers();
                        break;
		}
	}

	if (needoutput || (flags_ & TF_ACKNOW))
		send_much(1, REASON_NORMAL, 0);
	else if ((curseq_ + iss_) > highest_ack_)
		send_much(0, REASON_NORMAL, 0);
	// K: which state to return to when nothing left?
	Packet::free(pkt);
	return;

dropafterack:
	flags_ |= TF_ACKNOW;
	send_much(1, REASON_NORMAL, 0);
	goto drop;

dropwithreset:
	/* we should be sending an RST here, but can't in simulator */
	if (tiflags & TH_ACK)
		sendpacket(ackno, 0, 0x0, 0, REASON_NORMAL);
	else {
		int ack = tcph->seqno() + datalen;
		if (tiflags & TH_SYN)
			ack--;
		sendpacket(0, ack, TH_ACK, 0, REASON_NORMAL);
	}
drop:
   	Packet::free(pkt);
	return;
}

//
// reset_rtx_timer: called during a retransmission timeout
// to perform exponential backoff.  Also, note that because
// we have performed a retransmission, our rtt timer is now
// invalidated (indicate this by setting rtt_active_ false)
//
void FullTcpAgent::reset_rtx_timer(int /* mild */)
{
	// cancel old timer, set a new one
        rtt_backoff();		// double current timeout
        set_rtx_timer();	// set new timer
        rtt_active_ = FALSE;	// no timing during this window
}


/*
 * do an active open
 * (in real TCP, see tcp_usrreq, case PRU_CONNECT)
 */
void FullTcpAgent::connect()
{
	newstate(TCPS_SYN_SENT);	// sending a SYN now

	if (!data_on_syn_) {
		// force no data in this segment
		int cur = curseq_;
		curseq_ = iss_;
		output(iss_, REASON_NORMAL);
		curseq_ = cur + 1;	// K: think I have to add in the syn here
		return;
	}
	output(iss_, REASON_NORMAL);
	return;
}

/*
 * be a passive opener
 * (in real TCP, see tcp_usrreq, case PRU_LISTEN)
 * (for simulation, make this peer's ptype ACKs)
 */
void FullTcpAgent::listen()
{
	newstate(TCPS_LISTEN);
	type_ = PT_ACK;	// instead of PT_TCP
}

/*
 * called when user/application performs 'close'
 */

void FullTcpAgent::usrclosed()
{

	curseq_ = t_seqno_;	// truncate buffer
	switch (state_) {
	case TCPS_CLOSED:
	case TCPS_LISTEN:
	case TCPS_SYN_SENT:
		cancel_timers();
		newstate(TCPS_CLOSED);
		break;
	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		newstate(TCPS_FIN_WAIT_1);
		send_much(1, REASON_NORMAL, 0);
		break;
	}

	return;
}

int FullTcpAgent::command(int argc, const char*const* argv)
{
	// would like to have some "connect" primitive
	// here, but the problem is that we get called before
	// the simulation is running and we want to send a SYN.
	// Because no routing exists yet, this fails.
	// Instead, see code in advance() above.
	//
	// listen can happen any time because it just changes state_
	//
	// close is designed to happen at some point after the
	// simulation is running (using an ns 'at' command)

	if (argc == 2) {
		if (strcmp(argv[1], "listen") == 0) {
			// just a state transition
			listen();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "close") == 0) {
			usrclosed();
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "advance") == 0) {
			advanceby(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "advance-bytes") == 0) {
			advance_bytes(atoi(argv[2]));
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc, argv));
}
/*
 * clear out reassembly queue
 */
void ReassemblyQueue::clear()
{
	seginfo* p;
	for (p = head_; p != NULL; p = p->next_)
		delete p;
	head_ = tail_ = NULL;
	return;
}

/*
 * add a packet to the reassembly queue..
 * will update FullTcpAgent::rcv_nxt_ by way of the
 * ReassemblyQueue::rcv_nxt_ integer reference (an alias)
 */
int ReassemblyQueue::add(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_cmn *th = (hdr_cmn*)pkt->access(off_cmn_);

	int start = tcph->seqno();
	int end = start + th->size() - tcph->hlen();
	int tiflags = tcph->flags();
	seginfo *q, *p, *n;

	if (head_ == NULL) {
		// nobody there, just insert
		tail_ = head_ = new seginfo;
		head_->prev_ = NULL;
		head_->next_ = NULL;
		head_->startseq_ = start;
		head_->endseq_ = end;
		head_->flags_ = tiflags;
	} else {
		p = NULL;
		n = new seginfo;
		n->startseq_ = start;
		n->endseq_ = end;
		n->flags_ = tiflags;
		if (tail_->endseq_ <= start) {
			// common case of end of reass queue
			p = tail_;
			goto endfast;
		}

		q = head_;
		// look for the segment after this one
		while (q != NULL && (end > q->startseq_))
			q = q->next_;
		// set p to the segment before this one
		if (q == NULL)
			p = tail_;
		else
			p = q->prev_;

		if (p == NULL) {
			// insert at head
			n->next_ = head_;
			n->prev_ = NULL;
			head_->prev_ = n;
			head_ = n;
		} else {
endfast:
			// insert in the middle or end
			n->next_ = p->next_;
			p->next_ = n;
			n->prev_ = p;
			if (p == tail_)
				tail_ = n;
		}
	}
	//
	// look for a sequence of in-order segments and
	// set rcv_nxt if we can
	//

	if (head_->startseq_ > rcv_nxt_)
		return 0;	// still awaiting a hole-fill

	tiflags = 0;
	p = head_;
	while (p != NULL) {
		// update rcv_nxt_ to highest in-seq thing
		// and delete the entry from the reass queue
		rcv_nxt_ = p->endseq_;
		tiflags |= p->flags_;
		q = p;
		if (q->prev_)
			q->prev_->next_ = q->next_;
		else
			head_ = q->next_;
		if (q->next_)
			q->next_->prev_ = q->prev_;
		else
			tail_ = q->prev_;
		if (q->next_ && (q->endseq_ < q->next_->startseq_)) {
			delete q;
			break;		// only the in-seq stuff
		}
		p = p->next_;
		delete q;
	}
	return (tiflags);
}

/*
 * deal with timers going off.
 * 2 types for now:
 *	retransmission timer (rtx_timer_)
 *  delayed ack timer (delack_timer_)
 *	delayed send (randomization) timer (delsnd_timer_)
 *
 * real TCP initializes the RTO as 6 sec
 *	(A + 2D, where A=0, D=3), [Stevens p. 305]
 * and thereafter uses
 *	(A + 4D, where A and D are dynamic estimates)
 *
 * note that in the simulator t_srtt_, t_rttvar_ and t_rtt_
 * are all measured in 'tcp_tick_'-second units
 */

void FullTcpAgent::timeout(int tno)
{
//printf("%f: tcp(%s): timeout\n",
//now(), name());

	/*
	 * shouldn't be getting timeouts here
	 */
	if (state_ == TCPS_CLOSED || state_ == TCPS_LISTEN) {
		fprintf(stderr, "%f: (%s) unexpected timeout %d in state %d\n",
			now(), name(), tno, state_);
		return;
	}
 
	if (tno == TCP_TIMER_RTX) {
		/* retransmit timer */
		++nrexmit_;
		recover_ = maxseq_;
		recover_cause_ = REASON_TIMEOUT;
		closecwnd(0);
		reset_rtx_timer(1);
		t_seqno_ = highest_ack_;
		dupacks_ = 0;
		send_much(1, PF_TIMEOUT);
	} else if (tno == TCP_TIMER_DELSND) {
		/*
		 * delayed-send timer, with random overhead
		 * to avoid phase effects
		 */
		send_much(1, PF_TIMEOUT);
	} else if (tno == TCP_TIMER_DELACK) {
		if (flags_ & TF_DELACK) {
			flags_ &= ~TF_DELACK;
			flags_ |= TF_ACKNOW;
			send_much(1, REASON_NORMAL, 0);
		}
		delack_timer_.resched(delack_interval_);
	} else {
		fprintf(stderr, "%f: (%s) UNKNOWN TIMEOUT %d\n",
			now(), name(), tno);
	}
}

void
FullTcpAgent::dooptions(Packet* pkt)
{
	// interesting options: timestamps (here),
	//	CC, CCNEW, CCECHO (future work perhaps)

        hdr_flags *fh = (hdr_flags *)pkt->access(off_flags_);
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	if (ts_option_ && !fh->no_ts_) {
		ts_peer_ = tcph->ts();	// most recent tstamp from peer
		if (tcph->flags() & TH_SYN) {
			flags_ |= TF_RCVD_TSTMP;
			recent_ = tcph->ts();
			recent_age_ = now();
		}
	}
}

void DelAckTimer::expire(Event *) {
        a_->timeout(TCP_TIMER_DELACK);
}
