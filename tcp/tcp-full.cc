/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * Fixes for gensack() and ReassemblyQueue::add() contributed by Richard 
 * Mortier <Richard.Mortier@cl.cam.ac.uk>
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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-full.cc,v 1.89 2001/07/17 18:01:20 kfall Exp $ (LBL)";
#endif

#include "ip.h"
#include "tcp-full.h"
#include "flags.h"
#include "random.h"
#include "template.h"

#define	TRUE 	1
#define	FALSE 	0

#define	MIN(x,y)	(((x)<(y))?(x):(y))
#define	MAX(x,y)	(((x)>(y))?(x):(y))

static class FullTcpClass : public TclClass { 
public:
	FullTcpClass() : TclClass("Agent/TCP/FullTcp") {}
	TclObject* create(int, const char*const*) { 
		return (new FullTcpAgent());
	}
} class_full;

static class TahoeFullTcpClass : public TclClass { 
public:
	TahoeFullTcpClass() : TclClass("Agent/TCP/FullTcp/Tahoe") {}
	TclObject* create(int, const char*const*) { 
		// tcl lib code
		// sets reno_fastrecov_ to false
		return (new TahoeFullTcpAgent());
	}
} class_tahoe_full;

static class NewRenoFullTcpClass : public TclClass { 
public:
	NewRenoFullTcpClass() : TclClass("Agent/TCP/FullTcp/Newreno") {}
	TclObject* create(int, const char*const*) { 
		// tcl lib code
		// sets deflate_on_pack_ to false
		return (new NewRenoFullTcpAgent());
	}
} class_newreno_full;

static class SackFullTcpClass : public TclClass { 
public:
	SackFullTcpClass() : TclClass("Agent/TCP/FullTcp/Sack") {}
	TclObject* create(int, const char*const*) { 
		return (new SackFullTcpAgent());
	}
} class_sack_full;

/*
 * Tcl bound variables:
 *	segsperack: for delayed ACKs, how many to wait before ACKing
 *	segsize: segment size to use when sending
 */
FullTcpAgent::FullTcpAgent() :  
	closed_(0), pipe_(0), fastrecov_(FALSE), 
	last_send_time_(-1.0), infinite_send_(0), irs_(-1), 
	delack_timer_(this), flags_(0), 
	state_(TCPS_CLOSED), ect_(FALSE), recent_ce_(FALSE), 
	last_state_(TCPS_CLOSED), rq_(rcv_nxt_), last_ack_sent_(-1)
{
}

void
FullTcpAgent::delay_bind_init_all()
{
        delay_bind_init_one("segsperack_");
        delay_bind_init_one("segsize_");
        delay_bind_init_one("tcprexmtthresh_");
        delay_bind_init_one("iss_");
        delay_bind_init_one("nodelay_");
        delay_bind_init_one("data_on_syn_");
        delay_bind_init_one("dupseg_fix_");
        delay_bind_init_one("dupack_reset_");
        delay_bind_init_one("close_on_empty_");
        delay_bind_init_one("interval_");
        delay_bind_init_one("ts_option_size_");
        delay_bind_init_one("reno_fastrecov_");
        delay_bind_init_one("pipectrl_");
        delay_bind_init_one("open_cwnd_on_pack_");
        delay_bind_init_one("halfclose_");
        delay_bind_init_one("nopredict_");

	TcpAgent::delay_bind_init_all();
       
      	reset();
}

int
FullTcpAgent::delay_bind_dispatch(const char *varName, const char *localName, TclObject *tracer)
{
        if (delay_bind(varName, localName, "segsperack_", &segs_per_ack_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "segsize_", &maxseg_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "tcprexmtthresh_", &tcprexmtthresh_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "iss_", &iss_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nodelay_", &nodelay_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "data_on_syn_", &data_on_syn_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "dupseg_fix_", &dupseg_fix_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "dupack_reset_", &dupack_reset_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "close_on_empty_", &close_on_empty_, tracer)) return TCL_OK;
        if (delay_bind_time(varName, localName, "interval_", &delack_interval_, tracer)) return TCL_OK;
        if (delay_bind(varName, localName, "ts_option_size_", &ts_option_size_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "reno_fastrecov_", &reno_fastrecov_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "pipectrl_", &pipectrl_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "open_cwnd_on_pack_", &open_cwnd_on_pack_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "halfclose_", &halfclose_, tracer)) return TCL_OK;
        if (delay_bind_bool(varName, localName, "nopredict_", &nopredict_, tracer)) return TCL_OK;

        return TcpAgent::delay_bind_dispatch(varName, localName, tracer);
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
	rq_.clear();		// clear reassembly queue
	rtt_init();		// zero rtt, srtt, backoff

	last_ack_sent_ = -1;
	rcv_nxt_ = -1;
	pipe_ = 0;
	flags_ = 0;
	t_seqno_ = iss_;
	maxseq_ = -1;
	irs_ = -1;
	last_send_time_ = -1.0;
	if (ts_option_)
		recent_ = recent_age_ = 0.0;
	else
		recent_ = recent_age_ = -1.0;

	fastrecov_ = FALSE;
}

/*
 * pack() -- is the ACK a partial ACK? (not past recover_)
 */

int
FullTcpAgent::pack(Packet *pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	return (tcph->ackno() >= highest_ack_ &&
		tcph->ackno() < recover_);
}

/*
 * baseline reno TCP exists fast recovery on a partial ACK
 */

void
FullTcpAgent::pack_action(Packet*)
{
	if (reno_fastrecov_ && fastrecov_ && cwnd_ > ssthresh_) {
		cwnd_ = ssthresh_; // retract window if inflated
	}
	fastrecov_ = FALSE;
	dupacks_ = 0;
}

/*
 * ack_action -- same as partial ACK action for base Reno TCP
 */

void
FullTcpAgent::ack_action(Packet* p)
{
	FullTcpAgent::pack_action(p);
}

/*
 * headersize:
 *	how big is an IP+TCP header in bytes; include options such as ts
 * this function should be virtual so others (e.g. SACK) can override
 */
int
FullTcpAgent::headersize()
{
	int total = tcpip_base_hdr_size_;
	if (total < 1) {
		fprintf(stderr,
		    "TCP-FULL(%s): warning: tcpip hdr size is only %d bytes\n",
			name(), tcpip_base_hdr_size_);
	}

	if (ts_option_)
		total += ts_option_size_;

	return (total);
}

/*
 * free resources: any pending timers + reassembly queue
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
 * 'curseq_' is the sequence number of the last byte provided
 * by the application.  In the case where no data has been supplied
 * by the application, curseq_ is the iss_.
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
 * the byte-oriented interface: advance_bytes(int nbytes)
 */

void
FullTcpAgent::advance_bytes(int nb)
{

	//
	// state-specific operations:
	//	if CLOSED, reset and try a new active open/connect
	//	if ESTABLISHED, just try to send more
	//	if SYN_SENT, just queue
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
	} else if (state_ == TCPS_ESTABLISHED || state_ == TCPS_SYN_SENT) {
		// allow queued data to accumulate even
		// in SYN_SENT
		closed_ = 0;
		if (curseq_ < iss_)
			curseq_ = iss_;
		curseq_ += nb;
		if (state_ == TCPS_ESTABLISHED)
			send_much(0, REASON_NORMAL, maxburst_);
	}
	return;
}

/*
 * If MSG_EOF is set, by setting close_on_empty_ to TRUE, we ensure that
 * a FIN will be sent when the send buffer emptys.
 * 
 * When (in the future?) FullTcpAgent implements T/TCP, avoidance of 3-way 
 * handshake can be handled in this function.
 */
void
FullTcpAgent::sendmsg(int nbytes, const char *flags)
{
	if (flags && strcmp(flags, "MSG_EOF") == 0) 
		close_on_empty_ = TRUE;	
	if (nbytes == -1) {
		infinite_send_ = TRUE;
		advance_bytes(0);
	} else
		advance_bytes(nbytes);
}

/*
 * flags that are completely dependent on the tcp state
 * (in real TCP, see tcp_fsm.h, the "tcp_outflags" array)
 */
int
FullTcpAgent::outflags()
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
 * reaass() -- extract the appropriate fields from the packet
 *	and pass this info the ReassemblyQueue add routine
 *
 * returns the TCP header flags representing the "or" of
 *	the flags contained in the adjacent sequence # blocks
 */

int
FullTcpAgent::reass(Packet* pkt)
{  
        hdr_tcp *tcph =  hdr_tcp::access(pkt);
        hdr_cmn *th = hdr_cmn::access(pkt);
   
        int start = tcph->seqno();
        int end = start + th->size() - tcph->hlen();
        int tiflags = tcph->flags();
	int fillshole = (start == rcv_nxt_);
	int rval;
   
	// end contains the seq of the last byte of
	// in the packet plus one

	if (start == end && (tiflags & TH_FIN) == 0) {
		fprintf(stderr, "%f: FullTcpAgent::reass() -- bad condition - adding non-FIN zero-len seg\n",
			now());
		abort();
	}

	rval = rq_.add(start, end, tiflags, 0);

present:
	 //
	 // If we've never received a SYN (unlikely)
	 // or this is an out of order addition, no reason to coalesce
	 //

	 if (TCPS_HAVERCVDSYN(state_) == 0 || !fillshole) {
	 	return (0);
	}
	//
	// If we get some data, no need to present to user yet
	//
	if (state_ == TCPS_SYN_RECEIVED && (end > start))
		return (0);

	// clear out data that has been passed up to rcv_nxt_,
	// collects flags

	rval = rq_.cleartonxt();

        return (rval);
}

/*
 * utility function to set rcv_next_ during inital exchange of seq #s
 */

int
FullTcpAgent::rcvseqinit(int seq, int dlen)
{
	return (seq + dlen + 1);
}

/*
 * build a header with the timestamp option if asked
 */
int
FullTcpAgent::build_options(hdr_tcp* tcph)
{
	int total = 0;
	if (ts_option_) {
		tcph->ts() = now();
		tcph->ts_echo() = recent_;
		total += ts_option_size_;
	} else {
		tcph->ts() = tcph->ts_echo() = -1.0;
	}
	return (total);
}

/*
 * sendpacket: 
 *	allocate a packet, fill in header fields, and send
 *	also keeps stats on # of data pkts, acks, re-xmits, etc
 *
 * fill in packet fields.  Agent::allocpkt() fills
 * in most of the network layer fields for us.
 * So fill in tcp hdr and adjust the packet size.
 *
 * Also, return the size of the tcp header.
 */
void
FullTcpAgent::sendpacket(int seqno, int ackno, int pflags, int datalen, int reason)
{
        Packet* p = allocpkt();
        hdr_tcp *tcph = hdr_tcp::access(p);

	/* build basic header w/options */

        tcph->seqno() = seqno;
        tcph->ackno() = ackno;
        tcph->flags() = pflags;
        tcph->reason() |= reason; // make tcph->reason look like ns1 pkt->flags?
	tcph->sa_length() = 0;    // may be increased by build_options()
        tcph->hlen() = tcpip_base_hdr_size_;
	tcph->hlen() += build_options(tcph);

	/* ECT, ECN, and congestion action */

        hdr_flags *fh = hdr_flags::access(p);
	fh->ect() = ect_;	// on after mutual agreement on ECT
	if (ecn_ && !ect_)	// initializing; ect = 0, ecnecho = 1
		fh->ecnecho() = 1;
	else {
		fh->ecnecho() = recent_ce_;
	}
	fh->cong_action() =  cong_action_;


	/* actual size is data length plus header length */

        hdr_cmn *ch = hdr_cmn::access(p);
        ch->size() = datalen + tcph->hlen();

        if (datalen <= 0)
                ++nackpack_;
        else {
                ++ndatapack_;
                ndatabytes_ += datalen;
		last_send_time_ = now();	// time of last data
        }
        if (reason == REASON_TIMEOUT || reason == REASON_DUPACK) {
                ++nrexmitpack_;
                nrexmitbytes_ += datalen;
        }

	last_ack_sent_ = ackno;
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
	if (maxseg_ == 0) 
	   	maxseg_ = size_ - headersize();
	else
		size_ =  maxseg_ + headersize();
	int is_retransmit = (seqno < maxseq_);
	int quiet = (highest_ack_ == maxseq_);
	int pflags = outflags();
	int syn = (seqno == iss_);
	int emptying_buffer = FALSE;
	int buffered_bytes = (infinite_send_) ? TCP_MAXSEQ :
				curseq_ - highest_ack_ + 1;

	int win = window() * maxseg_;	// window (in bytes)
	int off = seqno - highest_ack_;	// offset of seg in window
	int datalen;

	// be careful if we have not received any ACK yet
	if (highest_ack_ < 0) {
		if (!infinite_send_)
			buffered_bytes = curseq_ - iss_;;
		off = seqno - iss_;
	}

	if (syn && !data_on_syn_)
		datalen = 0;
	else if (pipectrl_)
		datalen = buffered_bytes - off;
	else
		datalen = min(buffered_bytes, win) - off;

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
	// this is an option that causes us to slow-start if we've
	// been idle for a "long" time, where long means a rto or longer
	// the slow-start is a sort that does not set ssthresh
	//

	if (slow_start_restart_ && quiet && datalen > 0) {
		if (idle_restart()) {
			slowdown(CLOSE_CWND_INIT);
		}
	}

	//
	// see if sending this packet will empty the send buffer
	// a dataless SYN packet counts also
	//

	if (!infinite_send_ && ((seqno + datalen) > curseq_ || 
	    (syn && datalen == 0))) {
		emptying_buffer = TRUE;
		//
		// if not a retransmission, notify application that 
		// everything has been sent out at least once.
		//
		if (!syn) {
			idle();
			if (close_on_empty_ && quiet) {
				flags_ |= TF_NEEDCLOSE;
			}
		}
		pflags |= TH_PUSH;
		//
		// if close_on_empty set, we are finished
		// with this connection; close it
		//
	} else  {
		/* not emptying buffer, so can't be FIN */
		pflags &= ~TH_FIN;
	}
	if (infinite_send_ && (syn && datalen == 0))
		pflags |= TH_PUSH;  // set PUSH for dataless SYN

	/* sender SWS avoidance (Nagle) */

	if (datalen > 0) {
		// if full-sized segment, ok
		if (datalen == maxseg_)
			goto send;
		// if Nagle disabled and buffer clearing, ok
		if ((quiet || nodelay_)  && emptying_buffer)
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
	 * send now if a control packet or we owe peer an ACK
	 * TF_ACKNOW can be set during connection establishment and
	 * to generate acks for out-of-order data
	 */

	if ((flags_ & (TF_ACKNOW|TF_NEEDCLOSE)) ||
	    (pflags & (TH_SYN|TH_FIN))) {
		goto send;
	}

        /*      
         * No reason to send a segment, just return.
         */      
	return;

send:

	// is a syn or fin?

	syn = (pflags & TH_SYN) ? 1 : 0;
	int fin = (pflags & TH_FIN) ? 1 : 0;

	sendpacket(seqno, rcv_nxt_, pflags, datalen, reason);

        /*      
         * Data sent (as far as we can tell).
         * Any pending ACK has now been sent.
         */      
	flags_ &= ~(TF_ACKNOW|TF_DELACK);

	/*
	 * if we have reacted to congestion recently, the
	 * slowdown() procedure will have set cong_action_ and
	 * sendpacket will have copied that to the outgoing pkt
	 * CACT field. If that packet contains data, then
	 * it will be reliably delivered, so we are free to turn off the
	 * cong_action_ state now  If only a pure ACK, we keep the state
	 * around until we actually send a segment
	 */

	int reliable = datalen + syn + fin; // seq #'s reliably sent
	if (cong_action_ && reliable > 0)
		cong_action_ = FALSE;

	/*
	 * SYNs and FINs each use up one sequence number, but
	 * there's no reason to advance t_seqno_ by one for a FIN
	 */

	if (!fin && seqno == t_seqno_) {
		t_seqno_ += reliable;
	}


	// highest: greatest sequence number sent + 1
	//	and adjusted for SYNs and FINs which use up one number

	int highest = seqno + reliable;
	if (highest > maxseq_) {
		maxseq_ = highest;
		//
		// if we are using conventional RTT estimation,
		// establish timing on this segment
		//
		if (!ts_option_ && rtt_active_ == FALSE) {
			rtt_active_ = TRUE;	// set timer
			rtt_seq_ = seqno; 	// timed seq #
			rtt_ts_ = now();	// when set
		}
	}

//if (fin)
//printf("%f %s: sent FIN, seq: %d, maxseq now: %d\n",
//now(), name(), seqno, int(maxseq_));

	/*
	 * Set retransmit timer if not currently set,
	 * and not doing an ack or a keep-alive probe.
	 * Initial value for retransmit timer is smoothed
	 * round-trip time + 2 * round-trip time variance.
	 * Future values are rtt + 4 * rttvar.
	 */
	if (rtx_timer_.status() != TIMER_PENDING && reliable) {
		set_rtx_timer();  // no timer pending, schedule one
	}

	if (flags_ & TF_NEEDCLOSE) {
		flags_ &= ~TF_NEEDCLOSE;
		if (state_ <= TCPS_ESTABLISHED && state_ != TCPS_CLOSED)
			usrclosed();
	}

	return;
}

/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void
FullTcpAgent::send_much(int force, int reason, int maxburst)
{
	/*
	 * highest_ack is essentially "snd_una" in real TCP
	 *
	 * loop while we are in-window (seqno <= (highest_ack + win))
	 * and there is something to send (t_seqno_ < curseq_+iss_)
	 */
	int win = window() * maxseg_;	// window() in pkts
	int npackets = 0;
	int topwin = curseq_; // 1 seq number past the last byte we can send
	if ((topwin > highest_ack_ + win) || infinite_send_)
		topwin = highest_ack_ + win;

	if (!force && (delsnd_timer_.status() == TIMER_PENDING))
		return;

	/*
	 * note that if output() doesn't actually send anything, we can
	 * loop forever here
	 */

	while (force ||
	      (pipectrl_ ? (pipe_ < window()) : (t_seqno_ < topwin))) {

		if (!force && overhead_ != 0 &&
		    (delsnd_timer_.status() != TIMER_PENDING)) {
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
		++pipe_;
		force = 0;

		if ((outflags() & (TH_SYN|TH_FIN)) ||
		    (maxburst && ++npackets >= maxburst))
			break;
	}
	return;
}

/*
 * This function is invoked when the connection is done. It in turn
 * invokes the Tcl finish procedure that was registered with TCP.
 * This function mimics tcp_close()
 */
void FullTcpAgent::finish()
{
	if (closed_)
		return;
	closed_ = 1;
	rq_.clear();
	cancel_timers();
	Tcl::instance().evalf("%s done", this->name());
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
void
FullTcpAgent::newack(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	register int ackno = tcph->ackno();
	int progress = (ackno > highest_ack_);

//if (state_ > TCPS_ESTABLISHED)
//printf("%f %s: newack: %d, state %d, maxseq:%d\n", now(), name(),
//ackno, int(state_), int(maxseq_));


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
        hdr_flags *fh = hdr_flags::access(pkt);
        if (!fh->no_ts_) {
                if (ts_option_) {
			recent_age_ = now();
			recent_ = tcph->ts();
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
 *
 * Here's the comment from the real TCP:
 *
 * Header prediction: check for the two common cases
 * of a uni-directional data xfer.  If the packet has
 * no control flags, is in-sequence, the window didn't
 * change and we're not retransmitting, it's a
 * candidate.  If the length is zero and the ack moved
 * forward, we're the sender side of the xfer.  Just
 * free the data acked & wake any higher level process
 * that was blocked waiting for space.  If the length
 * is non-zero and the ack didn't move, we're the
 * receiver side.  If we're getting packets in-order
 * (the reassembly queue is empty), add the data to
 * the socket buffer and note that we need a delayed ack.
 * Make sure that the hidden state-flags are also off.
 * Since we check for TCPS_ESTABLISHED above, it can only
 * be TF_NEEDSYN.
 */

int
FullTcpAgent::predict_ok(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
        hdr_flags *fh = hdr_flags::access(pkt);

	/* not the fastest way to do this, but perhaps clearest */

	int p1 = (state_ == TCPS_ESTABLISHED);		// ready
	int p2 = ((tcph->flags() & (TH_SYN|TH_FIN|TH_ACK)) == TH_ACK); // ACK
	int p3 = ((flags_ & TF_NEEDFIN) == 0);		// don't need fin
	int p4 = (!ts_option_ || fh->no_ts_ || (tcph->ts() >= recent_)); // tsok
	int p5 = (tcph->seqno() == rcv_nxt_);		// in-order data
	int p6 = (t_seqno_ == maxseq_);			// not re-xmit
	int p7 = (!ecn_ || fh->ecnecho() == 0);		// no ECN
	int p8 = (tcph->sa_length() == 0);		// no SACK info

	return (p1 && p2 && p3 && p4 && p5 && p6 && p7 && p8);
}

/*
 * fast_retransmit using the given seqno
 *	perform a fast retransmit
 *	kludge t_seqno_ (snd_nxt) so we do the
 *	retransmit then continue from where we were
 *	Also, stash our current location in recover_
 */

void FullTcpAgent::fast_retransmit(int seq)
{
	// we are now going to fast-retransmit and willtrace that event
	trace_event("FAST_RETX");
	
	int onxt = t_seqno_;		// output() changes t_seqno_
	recover_ = maxseq_;		// keep a copy of highest sent
	last_cwnd_action_ = CWND_ACTION_DUPACK;
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
	if (last_send_time_ < 0.0) {
		// last_send_time_ isn't set up yet, we shouldn't
		// do the idle_restart
		return (0);
	}

	double tao = now() - last_send_time_;
	if (!ts_option_) {
                double tickoff = fmod(last_send_time_ + boot_time_,
			tcp_tick_);
                tao = int((tao + tickoff) / tcp_tick_) * tcp_tick_;
	}

	return (tao > t_rtxcur_);  // verify this CHECKME
}

/*
 * tcp-full's version of set_initial_window()... over-rides
 * the one in tcp.cc
 */
void
FullTcpAgent::set_initial_window() {
	if (delay_growth_)
		cwnd_ = wnd_init_;
	else    
		cwnd_ = initial_window();
}       

/*
 * main reception path - 
 * called from the agent that handles the data path below in its muxing mode
 * advance() is called when connection is established with size sent from
 * user/application agent
 */
void FullTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	hdr_cmn *th = hdr_cmn::access(pkt);
	hdr_flags *fh = hdr_flags::access(pkt);

	int needoutput = FALSE;
	int ourfinisacked = FALSE;
	int dupseg = FALSE;
	int todrop = 0;

	last_state_ = state_;

	int datalen = th->size() - tcph->hlen(); // # payload bytes
	int ackno = tcph->ackno();		 // ack # from packet
	int tiflags = tcph->flags() ; 		 // tcp flags from packet
	if (state_ == TCPS_CLOSED)
		goto drop;

        /*
         * Process options if not in LISTEN state,
         * else do it below
         */
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

	//
	// sanity check for ECN: shouldn't be seeing a CE bit if
	// ECT wasn't set on the packet first.  If we see this, we
	// probably have a misbehaving router...
	//

	if (fh->ce() && !fh->ect()) {
	    fprintf(stderr,
	    "%f: FullTcpAgent::recv(%s): warning: CE bit on, but ECT false!\n",
		now(), name());
	}

	if (!nopredict_ && predict_ok(pkt)) {
                /*
                 * If last ACK falls within this segment's sequence numbers,
                 * record the timestamp.
		 * See RFC1323 (now RFC1323 bis)
                 */
                if (ts_option_ && !fh->no_ts_ &&
		    tcph->seqno() <= last_ack_sent_) {
			/*
			 * this is the case where the ts value is newer than
			 * the last one we've seen, and the seq # is the one
			 * we expect [seqno == last_ack_sent_] or older
			 */
			recent_age_ = now();
			recent_ = tcph->ts();
                }

		//
		// generate a stream of ecnecho bits until we see a true
		// cong_action bit
		//
		if (ecn_) {
			if (fh->ce() && fh->ect())
				recent_ce_ = TRUE;
			else if (fh->cong_action())
				recent_ce_ = FALSE;
		}

		// Header predication basically looks to see
		// if the incoming packet is an expected pure ACK
		// or an expected data segment

		if (datalen == 0) {
			// check for a received pure ACK in the correct range..
			// also checks to see if we are wnd_ limited
			// (we don't change cwnd at all below), plus
			// not being in fast recovery and not a partial ack.
			// If we are in fast
			// recovery, go below so we can remember to deflate
			// the window if we need to
			if (ackno > highest_ack_ && ackno < maxseq_ &&
			    cwnd_ >= wnd_ && !fastrecov_) {
				newack(pkt);	// update timers,  highest_ack_
				/* no adjustment of cwnd here */
				if (curseq_ >= highest_ack_ || infinite_send_)
					send_much(0, REASON_NORMAL, maxburst_);
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
			recvBytes(datalen); // notify application of "delivery"
			//
			// special code here to simulate the operation
			// of a receiver who always consumes data,
			// resulting in a call to tcp_output
			Packet::free(pkt);
			if (need_send())
				send_much(1, REASON_NORMAL, maxburst_);
			return;
		}
	}


	// header prediction failed
	// (e.g. pure ACK out of valid range, SACK present, etc)...
	// do slow path processing

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

		/*
		 * must by a SYN (no ACK) at this point...
		 * in real tcp we would bump the iss counter here also
		 */
		dooptions(pkt);
		irs_ = tcph->seqno();
		t_seqno_ = iss_; /* tcp_sendseqinit() macro in real tcp */
		rcv_nxt_ = rcvseqinit(irs_, datalen);
		flags_ |= TF_ACKNOW;
		if (ecn_ && fh->ecnecho()) {
			ect_ = TRUE;
		}
		newstate(TCPS_SYN_RECEIVED);
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

	case TCPS_SYN_SENT:	/* we sent SYN, expecting SYN+ACK (or SYN) */

		/* drop if it's a SYN+ACK and the ack field is bad */
		if ((tiflags & TH_ACK) &&
			((ackno <= iss_) || (ackno > maxseq_))) {
			// not an ACK for our SYN, discard
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s): bad ACK (%d) for our SYN(%d)\n",
			        now(), name(), int(ackno), int(maxseq_));
			goto dropwithreset;
		}

		if ((tiflags & TH_SYN) == 0) {
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s): no SYN for our SYN(%d)\n",
			        now(), name(), int(maxseq_));
			goto drop;
		}

		/* looks like an ok SYN or SYN+ACK */
#ifdef notdef
cancel_rtx_timer();	// cancel timer on our 1st SYN [does this belong!?]
#endif
		irs_ = tcph->seqno();	// get initial recv'd seq #
		rcv_nxt_ = rcvseqinit(irs_, datalen);

		/*
		 * we are seeing either a SYN or SYN+ACK.  For pure SYN,
		 * ecnecho tells us our peer is ecn-capable.  For SYN+ACK,
		 * it's acking our SYN, so it already knows we're ecn capable,
		 * so it can just turn on ect
		 */
		if (ecn_ && (fh->ecnecho() || fh->ect()))
			ect_ = TRUE;

		if (tiflags & TH_ACK) {
			// SYN+ACK (our SYN was acked)
			// CHECKME
			highest_ack_ = ackno;
			cwnd_ = initial_window();

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

                        /*
                         * Received <SYN,ACK> in SYN_SENT[*] state.
                         * Transitions:
                         *      SYN_SENT  --> ESTABLISHED
                         *      SYN_SENT* --> FIN_WAIT_1
                         */

			if (flags_ & TF_NEEDFIN) {
				newstate(TCPS_FIN_WAIT_1);
				flags_ &= ~TF_NEEDFIN;
				tiflags &= ~TH_SYN;
			} else {
				newstate(TCPS_ESTABLISHED);
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
			t_seqno_--;	// CHECKME
		}

trimthenstep6:
		/*
		 * advance the seq# to correspond to first data byte
		 */
		tcph->seqno()++;

		if (tiflags & TH_ACK)
			goto process_ACK;

		goto step6;

	case TCPS_LAST_ACK:
	case TCPS_CLOSING:
		break;
	} /* end switch(state_) */

        /*
         * States other than LISTEN or SYN_SENT.
         * First check timestamp, if present.
         * Then check that at least some bytes of segment are within
         * receive window.  If segment begins before rcv_nxt,
         * drop leading data (and SYN); if nothing left, just ack.
         *
         * RFC 1323 PAWS: If we have a timestamp reply on this segment
         * and it's less than ts_recent, drop it.
         */

	if (ts_option_ && !fh->no_ts_ && recent_ && tcph->ts() < recent_) {
		if ((now() - recent_age_) > TCP_PAWS_IDLE) {
			/*
			 * this is basically impossible in the simulator,
			 * but here it is...
			 */
                        /*
                         * Invalidate ts_recent.  If this segment updates
                         * ts_recent, the age will be reset later and ts_recent
                         * will get a valid value.  If it does not, setting
                         * ts_recent to zero will at least satisfy the
                         * requirement that zero be placed in the timestamp
                         * echo reply when ts_recent isn't valid.  The
                         * age isn't reset until we get a valid ts_recent
                         * because we don't want out-of-order segments to be
                         * dropped when ts_recent is old.
                         */
			recent_ = 0.0;
		} else {
			goto dropafterack;
		}
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
	//	allow ACKNOW to be turned on, normal pure ACKs that happen
	//	to have seq #s below rcv_nxt can trigger an ACK war by
	//	forcing us to ACK the pure ACKs
	//
	todrop = rcv_nxt_ - tcph->seqno();  // how much overlap?

	if (todrop > 0 && ((tiflags & (TH_SYN|TH_FIN)) || datalen > 0)) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			tcph->seqno()++;
			th->size()--;	// XXX Must decrease packet size too!!
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
			dupseg = TRUE;	// *completely* duplicate

		}

		/*
		 * Trim duplicate data from the front of the packet
		 */

		tcph->seqno() += todrop;
		th->size() -= todrop;	// XXX Must decrease size too!!
					// why? [kf]..prob when put in RQ
		datalen -= todrop;

	}

	/*
	 * If we are doing timstamps and this packet has one, and
	 * If last ACK falls within this segment's sequence numbers,
	 * record the timestamp.
	 * See RFC1323 (now RFC1323 bis)
	 */
	if (ts_option_ && !fh->no_ts_ && tcph->seqno() <= last_ack_sent_) {
		/*
		 * this is the case where the ts value is newer than
		 * the last one we've seen, and the seq # is the one we expect
		 * [seqno == last_ack_sent_] or older
		 */
		recent_age_ = now();
		recent_ = tcph->ts();
	}

	if (tiflags & TH_SYN) {
		fprintf(stderr,
		    "%f: FullTcpAgent::recv(%s) received unexpected SYN (state:%d)\n",
		        now(), name(), state_);
		goto dropwithreset;
	}

	if ((tiflags & TH_ACK) == 0) {
		fprintf(stderr, "%f: FullTcpAgent::recv(%s) got packet lacking ACK (seq %d)\n",
			now(), name(), tcph->seqno());
		goto drop;
	}

	/*
	 * Ack processing.
	 */

	switch (state_) {
	case TCPS_SYN_RECEIVED:	/* want ACK for our SYN+ACK */
		if (ackno < highest_ack_ || ackno > maxseq_) {
			// not in useful range
			goto dropwithreset;
		}
                /*
                 * Make transitions:
                 *      SYN-RECEIVED  -> ESTABLISHED
                 *      SYN-RECEIVED* -> FIN-WAIT-1
                 */
                if (flags_ & TF_NEEDFIN) {
			newstate(TCPS_FIN_WAIT_1);
                        flags_ &= ~TF_NEEDFIN;
                } else {
                        newstate(TCPS_ESTABLISHED);
                }
		cwnd_ = initial_window();
		/* fall into ... */

        /*
         * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
         * ACKs.  If the ack is in the range
         *      tp->snd_una < ti->ti_ack <= tp->snd_max
         * then advance tp->snd_una to ti->ti_ack and drop
         * data from the retransmission queue.
	 *
	 * note that state TIME_WAIT isn't used
	 * in the simulator
         */

        case TCPS_ESTABLISHED:
        case TCPS_FIN_WAIT_1:
        case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
        case TCPS_CLOSING:
        case TCPS_LAST_ACK:

		//
		// look for ECNs in ACKs, react as necessary
		//

		if (fh->ecnecho() && (!ecn_ || !ect_)) {
			fprintf(stderr,
			    "%f: FullTcp(%s): warning, recvd ecnecho but I am not ECN capable!\n",
				now(), name());
		}

                //
                // generate a stream of ecnecho bits until we see a true
                // cong_action bit
                // 
                if (ecn_) {
                        if (fh->ce() && fh->ect())
                                recent_ce_ = TRUE;
                        else if (fh->cong_action())
                                recent_ce_ = FALSE;
                }

		// look for dup ACKs (dup ack numbers, no data)
		//
		// do fast retransmit/recovery if at/past thresh
		if (ackno <= highest_ack_) {
			// an ACK which doesn't advance highest_ack_
			if (datalen == 0 && (!dupseg_fix_ || !dupseg)) {
				--pipe_; // ACK indicates pkt cached @ receiver

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
					//trace_event("FAST_RECOVERY");
					fastrecov_ = TRUE;
					
					/* re-sync the pipe_ estimate */
					pipe_ = maxseq_ - highest_ack_;
					pipe_ /= maxseg_;
					pipe_ -= (dupacks_ + 1);

pipe_ = int(cwnd_) - dupacks_ - 1;

					dupack_action(); // maybe fast rexmt
					goto drop;

				} else if (dupacks_ > tcprexmtthresh_) {
					//trace_event("FAST_RECOVERY");
					if (reno_fastrecov_) {
						// we just measure cwnd in
						// packets, so don't scale by
						// maxseg_ as real
						// tcp does
						
						cwnd_++;
					}
					send_much(0, REASON_NORMAL, maxburst_);
					goto drop;
				}
			} else {
				// non zero-length [dataful] segment
				// with a dup ack (normal for dataful segs)
				// (or window changed in real TCP).
				if (dupack_reset_) {
					dupacks_ = 0;
					fastrecov_ = FALSE;
				}
			}
			break;	/* take us to "step6" */
		} /* end of dup acks */

		/*
		 * we've finished the fast retransmit/recovery period
		 * (i.e. received an ACK which advances highest_ack_)
		 * The ACK may be "good" or "partial"
		 */

process_ACK:

		if (ackno > maxseq_) {
			// ack more than we sent(!?)
			fprintf(stderr,
			    "%f: FullTcpAgent::recv(%s) too-big ACK (ack: %d, maxseq:%d)\n",
				now(), name(), int(ackno), int(maxseq_));
			goto dropafterack;
		}

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
		newack(pkt);	// handle timers, update highest_ack_
		--pipe_;

		/*
		 * if this is a partial ACK, invoke whatever we should
		 */

		int partial = pack(pkt);

		if (partial)
			pack_action(pkt);
		else
			ack_action(pkt);

		/*
		 * if this is an ACK with an ECN indication, handle this
		 */

		if (fh->ecnecho())
			ecn(highest_ack_);  // updated by newack(), above

		// CHECKME: handling of rtx timer
		if (ackno == maxseq_) {
			needoutput = TRUE;
		}

		/*
		 * If no data (only SYN) was ACK'd,
		 *    skip rest of ACK processing.
		 */
		if (ackno == (highest_ack_ + 1))
			goto step6;

		// if we are delaying initial cwnd growth (probably due to
		// large initial windows), then only open cwnd if data has
		// been received
                /*
                 * When new data is acked, open the congestion window.
                 * If the window gives us less than ssthresh packets
                 * in flight, open exponentially (maxseg per packet).
                 * Otherwise open about linearly: maxseg per window
                 * (maxseg^2 / cwnd per packet).
                 */
		if ((!delay_growth_ || (rcv_nxt_ > 0)) &&
		    last_state_ == TCPS_ESTABLISHED) {
			if (!partial || open_cwnd_on_pack_)
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
			if (ourfinisacked) {
				// got the ACK, now await incoming FIN
				newstate(TCPS_FIN_WAIT_2);
				cancel_timers();
				needoutput = FALSE;
			}
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
#ifdef notdef
cancel_timers();	// DOES THIS BELONG HERE?, probably (see tcp_cose
#endif
				finish();
				goto drop;
			} else {
				// should be a FIN we've seen
				hdr_ip* iph = hdr_ip::access(pkt);
                                fprintf(stderr,
                                "%f: %d.%d>%d.%d FullTcpAgent::recv(%s) received non-ACK (state:%d)\n",
                                        now(),
                                        iph->saddr(), iph->sport(),
                                        iph->daddr(), iph->dport(),
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
	 * (only good/new data... dup data has already been trimmed above)
	 */

	if ((datalen > 0 || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(state_) == 0) {

		//
		// the following 'if' implements the "real" TCP
		// TCP_REASS macro
		//

		if (tcph->seqno() == rcv_nxt_ && rq_.empty()) {
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
			// in "real" TCP, this is sbappend() + sorwakeup()
			if (datalen)
				recvBytes(datalen); // notify app. of "delivery"
			needoutput = need_send();
		} else {
			// see the "tcp_reass" function:
			// not the one we want next (or it
			// is but there's stuff on the reass queue);
			// do whatever we need to do for out-of-order
			// segments or hole-fills.  Also,
			// send an ACK (or DSACK) to the other side right now.
			// Note that we may have just a FIN here (datalen = 0)
			int rcv_nxt_old_ = rcv_nxt_; // notify app. if changes
			tiflags = reass(pkt);
			if (rcv_nxt_ > rcv_nxt_old_) {
				// if rcv_nxt_ has advanced, must have
				// been a hole fill.  In this case, there
				// is something to give to application
				recvBytes(rcv_nxt_ - rcv_nxt_old_);
			}
			flags_ |= TF_ACKNOW;

			if (tiflags & TH_PUSH) {
				//
				// ???: does this belong here
				// K: APPLICATION recv
				needoutput = need_send();
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
		switch (state_) {
                /*
                 * In SYN_RECEIVED and ESTABLISHED STATES
                 * enter the CLOSE_WAIT state.
		 * (passive close)
                 */
                case TCPS_SYN_RECEIVED:
                case TCPS_ESTABLISHED:
			newstate(TCPS_CLOSE_WAIT);
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
                        newstate(TCPS_CLOSED);
			cancel_timers();
                        break;
		}
	} /* end of if FIN bit on */

	if (needoutput || (flags_ & TF_ACKNOW))
		send_much(1, REASON_NORMAL, maxburst_);
	else if (curseq_ >= highest_ack_ || infinite_send_)
		send_much(0, REASON_NORMAL, maxburst_);
	// K: which state to return to when nothing left?

	if (!halfclose_ && state_ == TCPS_CLOSE_WAIT && highest_ack_ == maxseq_)
		usrclosed();

	Packet::free(pkt);

	// haoboy: Is here the place for done{} of active close? 
	// It cannot be put in the switch above because we might need to do
	// send_much() (an ACK)
	if (state_ == TCPS_CLOSED) 
		Tcl::instance().evalf("%s done", this->name());

	return;

dropafterack:
	flags_ |= TF_ACKNOW;
	send_much(1, REASON_NORMAL, maxburst_);
	goto drop;

dropwithreset:
	/* we should be sending an RST here, but can't in simulator */
	if (tiflags & TH_ACK) {
		sendpacket(ackno, 0, 0x0, 0, REASON_NORMAL);
	} else {
		int ack = tcph->seqno() + datalen;
		if (tiflags & TH_SYN)
			ack--;
		sendpacket(0, ack, TH_ACK, 0, REASON_NORMAL);
	}
drop:
   	Packet::free(pkt);
	return;
}
/*  
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 *  
 *      bugfix  ecn     last-cwnd == ecn        action  
 *  
 *      0       0       0                       full_reno_action
 *      0       0       1                       full_reno_action [impossible]
 *      0       1       0                       full_reno_action
 *      0       1       1                       1/2 window, return 
 *      1       0       0                       nothing 
 *      1       0       1                       nothing         [impossible]
 *      1       1       0                       nothing 
 *      1       1       1                       1/2 window, return
 */ 
    
void
FullTcpAgent::dupack_action()
{   
        int recovered = (highest_ack_ > recover_);
	//int recovered = !fastrecov_;
        if (recovered || (!bug_fix_ && !ecn_) ||
	    last_cwnd_action_ == CWND_ACTION_DUPACK) {
                goto full_reno_action;
        }       
    
        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
                slowdown(CLOSE_CWND_HALF);
		cancel_rtx_timer();
		rtt_active_ = FALSE;
		fast_retransmit(highest_ack_);
                return; 
        }      
    
        if (bug_fix_) {
                /*
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      
                return;  
        }
    
full_reno_action:    
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
	cancel_rtx_timer();
	rtt_active_ = FALSE;
	fast_retransmit(highest_ack_);
	// we measure cwnd in packets,
	// so don't scale by maxseg_
	// as real TCP does
	cwnd_ = ssthresh_ + dupacks_;
        return;
}

void
FullTcpAgent::timeout_action()
{
	recover_ = maxseq_;
	if (last_cwnd_action_ == CWND_ACTION_ECN) {
		slowdown(CLOSE_CWND_ONE);
	} else {
		slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_RESTART);
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
	}
	reset_rtx_timer(1);
	t_seqno_ = (highest_ack_ < 0) ? iss_ : int(highest_ack_);
	fastrecov_ = FALSE;
	dupacks_ = 0;
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
//curseq_ = t_seqno_ - 1;	// truncate buffer
	curseq_ = maxseq_ - 1;
	infinite_send_ = 0;
	switch (state_) {
	case TCPS_CLOSED:
	case TCPS_LISTEN:
		cancel_timers();
		newstate(TCPS_CLOSED);
		break;
	case TCPS_SYN_SENT:
		newstate(TCPS_CLOSED);
		/* fall through */
	case TCPS_LAST_ACK:
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		newstate(TCPS_FIN_WAIT_1);
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_CLOSE_WAIT:
		newstate(TCPS_LAST_ACK);
		flags_ |= TF_NEEDFIN;
		send_much(1, REASON_NORMAL, maxburst_);
		break;
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSING:
		/* usr asked for a close more than once [?] */
		fprintf(stderr,
		  "%f FullTcpAgent(%s): app close in bad state %d\n",
		  now(), name(), state_);
		break;
	default:
		fprintf(stderr,
		  "%f FullTcpAgent(%s): app close in unknown state %d\n",
		  now(), name(), state_);
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
		if (strcmp(argv[1], "advanceby") == 0) {
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
		timeout_action();
		send_much(1, PF_TIMEOUT, maxburst_);
	} else if (tno == TCP_TIMER_DELSND) {
		/*
		 * delayed-send timer, with random overhead
		 * to avoid phase effects
		 */
		send_much(1, PF_TIMEOUT, maxburst_);
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

        hdr_flags *fh = hdr_flags::access(pkt);
	hdr_tcp *tcph = hdr_tcp::access(pkt);

	if (ts_option_ && !fh->no_ts_) {
		if (tcph->ts() < 0.0) {
			fprintf(stderr,
			    "%f: FullTcpAgent(%s) warning: ts_option enabled in this TCP, but appears to be disabled in peer\n",
				now(), name());
		} else if (tcph->flags() & TH_SYN) {
			flags_ |= TF_RCVD_TSTMP;
			recent_ = tcph->ts();
			recent_age_ = now();
		}
	}
}

void FullTcpAgent::newstate(int ns)
{
//printf("%f %s: newstate (%d)->(%d)\n",
//now(), name(), state_, ns);
	state_ = ns;
}

void DelAckTimer::expire(Event *) {
        a_->timeout(TCP_TIMER_DELACK);
}

/*
 * for TCP Tahoe, we force a slow-start as the dup ack
 * action.  Also, no window inflation due to multiple dup
 * acks.  The latter is arranged by setting reno_fastrecov_
 * false [which is performed by the Tcl constructor for Tahoe in
 * ns-agent.tcl].
 */

/* 
 * Tahoe
 * Dupack-action: what to do on a DUP ACK.  After the initial check
 * of 'recover' below, this function implements the following truth
 * table:
 * 
 *      bugfix  ecn     last-cwnd == ecn        action  
 * 
 *      0       0       0                       full_tahoe_action
 *      0       0       1                       full_tahoe_action [impossible]
 *      0       1       0                       full_tahoe_action
 *      0       1       1                       1/2 window, return
 *      1       0       0                       nothing 
 *      1       0       1                       nothing         [impossible]
 *      1       1       0                       nothing 
 *      1       1       1                       1/2 window, return
 */

void
TahoeFullTcpAgent::dupack_action()
{  
        int recovered = (highest_ack_ > recover_);
        if (recovered || (!bug_fix_ && !ecn_) ||
            last_cwnd_action_ == CWND_ACTION_DUPACK) {
                goto full_tahoe_action;
        }
   
        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		last_cwnd_action_ = CWND_ACTION_DUPACK;
                slowdown(CLOSE_CWND_ONE);
		set_rtx_timer();
                rtt_active_ = FALSE;
		t_seqno_ = highest_ack_;	// slow-start
//send_much(0, REASON_NORMAL, 0);
                return; 
        }
   
        if (bug_fix_) {
                /*
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      
                return;  
        }
   
full_tahoe_action:
	trace_event("FAST_RETX");
	recover_ = maxseq_;
	last_cwnd_action_ = CWND_ACTION_DUPACK;
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_ONE);	// cwnd->1
	set_rtx_timer();
        rtt_active_ = FALSE;
	t_seqno_ = highest_ack_;		// slow-start
	send_much(0, REASON_NORMAL, 0);
        return; 
}  


/*
 * for NewReno, a partial ACK does not exit fast recovery,
 * and does not reset the dup ACK counter (which might trigger fast
 * retransmits we don't want).  In addition, the number of packets
 * sent in response to an ACK is limited to recov_maxburst_ during
 * recovery periods.
 */

NewRenoFullTcpAgent::NewRenoFullTcpAgent() : save_maxburst_(-1)
{
	bind("recov_maxburst_", &recov_maxburst_);
}

void
NewRenoFullTcpAgent::pack_action(Packet*)
{
	fast_retransmit(highest_ack_);
	cwnd_ = ssthresh_;
	if (save_maxburst_ < 0) {
		save_maxburst_ = maxburst_;
		maxburst_ = recov_maxburst_;
	}
}

void
NewRenoFullTcpAgent::ack_action(Packet* p)
{
	if (save_maxburst_ >= 0) {
		maxburst_ = save_maxburst_;
		save_maxburst_ = -1;
	}
	FullTcpAgent::ack_action(p);
}

/*
 *
 * ****** SACK ******
 *
 * for Sack, do the following
 */

SackFullTcpAgent::SackFullTcpAgent() : sack_nxt_(-1), sq_(sack_min_)
{
	bind("sack_option_size_", &sack_option_size_);
	bind("sack_block_size_", &sack_block_size_);
	bind("max_sack_blocks_", &max_sack_blocks_);
}

SackFullTcpAgent::~SackFullTcpAgent()
{
	sq_.clear();
}

void
SackFullTcpAgent::reset()
{
	sq_.clear();
	sack_max_ = sack_min_ = sack_nxt_ = -1;
	reno_fastrecov_ = FALSE;	// always F for sack
	pipectrl_ = FALSE;		// start in window mode
	FullTcpAgent::reset();
}

/*
 * override FullTcpAgent::recv() method to parse sack info
 * when it arrives.  Then call standard recv() method.
 */

void
SackFullTcpAgent::recv(Packet* pkt, Handler* h)
{
	hdr_tcp* tcph = hdr_tcp::access(pkt);
	int ackno = tcph->ackno();

	if (state_ == TCPS_ESTABLISHED &&
	    (ackno > iss_ && ackno <= maxseq_)) {

		int slen = tcph->sa_length();
		int i;  
	   
		for (i = 0; i < slen; ++i) {
			if (sack_max_ < tcph->sa_right(i))
				sack_max_ = tcph->sa_right(i);
			sq_.add(tcph->sa_left(i), tcph->sa_right(i), 0);  
		}
	}
	FullTcpAgent::recv(pkt, h);
}


int
SackFullTcpAgent::hdrsize(int nsackblocks)
{
	int total = FullTcpAgent::headersize();
        if (nsackblocks > 0) {
                total += ((nsackblocks * sack_block_size_)
                        + sack_option_size_);
	}
	return (total);
}

void
SackFullTcpAgent::ack_action(Packet* p)
{
	FullTcpAgent::ack_action(p);

	if (!sq_.empty() && sack_max_ <= highest_ack_) {
		sq_.clear();
	}

	if (sack_nxt_ < highest_ack_)
		sack_nxt_ = highest_ack_;
	pipectrl_ = FALSE;
}

void
SackFullTcpAgent::dupack_action()
{

        int recovered = (highest_ack_ > recover_);

        if (recovered || (!bug_fix_ && !ecn_) ||
            last_cwnd_action_ == CWND_ACTION_DUPACK) {
                goto full_sack_action;
        }           

        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
		pipectrl_ = TRUE;
                slowdown(CLOSE_CWND_HALF);
                cancel_rtx_timer();
                rtt_active_ = FALSE;
		recover_ = maxseq_;
		send_much(1, REASON_DUPACK, maxburst_);
                return; 
        }               
   
        if (bug_fix_) {
                /*                              
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */      
                return;  
        }
   
full_sack_action:                               
	trace_event("FAST_RECOVERY");
        slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
        cancel_rtx_timer();
        rtt_active_ = FALSE;
	pipectrl_ = TRUE;
	recover_ = maxseq_;
	send_much(1, REASON_DUPACK, maxburst_);
        return;
}

void
SackFullTcpAgent::pack_action(Packet*)
{
	--pipe_;
	if (sack_nxt_ < highest_ack_)
		sack_nxt_ = highest_ack_;
}

void
SackFullTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (fastrecov_)
		send_holes(force, maxburst);
	FullTcpAgent::send_much(force, reason, maxburst);
}

int
SackFullTcpAgent::build_options(hdr_tcp* tcph)
{
	int total = FullTcpAgent::build_options(tcph);

        if (!rq_.empty()) {
                int nblk = rq_.gensack(&tcph->sa_left(0), max_sack_blocks_);
                tcph->sa_length() = nblk;
		total += (nblk * sack_block_size_) + sack_option_size_;
        } else {
                tcph->sa_length() = 0;
        }
	return (total);
}

void
SackFullTcpAgent::send_holes(int force, int maxburst)
{

#ifdef notdef	// for now, don't do anything for SACK
		// as it is broken... will fix this next

	int npack = 0;
	int save_tseq = t_seqno_;
	t_seqno_ = sack_nxt_;

	if (sq_.empty()) {
		// no holes, just return
		return;
	}

        int nxtblk[2]; // left, right of 1 sack block
	sq_.sync();    // reset to beginning of sack list

	// skip over old blocks
	while (sq_.nextblk(nxtblk)) {
		if (t_seqno_ <= nxtblk[1])
			break;
	}

	while (force || (pipe_ < window())) {
		force = 0;
		// don't sent off the top
		if (t_seqno_ > recover_ || t_seqno_ >= sack_max_)
			break;

		// skip this one if the receiver has it already
		if (t_seqno_ >= nxtblk[0] && t_seqno_ <= nxtblk[1]) {
			t_seqno_ = nxtblk[1];
			if (sq_.nextblk(nxtblk))
				continue;
			break;	// no more blocks, finish up
		}

		// try to send something, check to see if we did
		int save = t_seqno_;
		output(t_seqno_, REASON_SACK);
		if (t_seqno_ == save) {
			break;
		}
		++pipe_;		// sent something

		if ((maxburst && ++npack >= maxburst) ||
		    (outflags() & (TH_SYN|TH_FIN)))
			break;
	}

	sack_nxt_ = t_seqno_;	// update next hole fill
	t_seqno_ = save_tseq;	// restore t_seqno_

#endif

	return;
}

void
SackFullTcpAgent::timeout_action()
{
	FullTcpAgent::timeout_action();
	sq_.clear();
	sack_min_ = highest_ack_;
	sack_nxt_ = sack_max_ = -1;
}
