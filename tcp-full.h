/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 *  This product includes software developed by the Network Research
 *  Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-full.h,v 1.14 1997/12/18 03:10:03 kfall Exp $ (LBL)
 */

#ifndef ns_tcp_full_h
#define ns_tcp_full_h

#include "tcp.h"

/*
 * these defines are directly from tcp_var.h in "real" TCP
 * they are used in the 'tcp_flags_' member variable
 */

#define TF_ACKNOW       0x0001          /* ack peer immediately */
#define TF_DELACK       0x0002          /* ack, but try to delay it */
#define TF_NODELAY      0x0004          /* don't delay packets to coalesce */
#define TF_NOOPT        0x0008          /* don't use tcp options */
#define TF_SENTFIN      0x0010          /* have sent FIN */

#define TCPS_CLOSED             0       /* closed */
#define TCPS_LISTEN             1       /* listening for connection */
#define TCPS_SYN_SENT           2       /* active, have sent syn */
#define TCPS_SYN_RECEIVED       3       /* have send and received syn */
#define TCPS_ESTABLISHED        4       /* established */
#define TCPS_FIN_WAIT_1         6       /* have closed, sent fin */
#define TCPS_CLOSING            7       /* closed xchd FIN; await FIN ACK */
#define TCPS_LAST_ACK           8       /* had fin and close; await FIN ACK */
#define TCPS_FIN_WAIT_2         9       /* have closed, fin is acked */

#define TCPS_HAVERCVDFIN(s)     ((s) == TCPS_CLOSING || (s) == TCPS_CLOSED)

#define TCPIP_BASE_PKTSIZE      40      /* base TCP/IP header in real life */
/* these are used to mark packets as to why we xmitted them */
#define REASON_NORMAL   0  
#define REASON_TIMEOUT  1
#define REASON_DUPACK   2

/* bits for the tcp_flags field below */
/* from tcp.h in the "real" implementation */
/* RST and URG are not used in the simulator */
 
#define TH_FIN  0x01        /* FIN: closing a connection */
#define TH_SYN  0x02        /* SYN: starting a connection */
#define TH_PUSH 0x08        /* PUSH: used here to "deliver" data */
#define TH_ACK  0x10        /* ACK: ack number is valid */

#define PF_TIMEOUT 0x04		/* protocol defined */

class FullTcpAgent;

class DelAckTimer : public TimerHandler {
public:
	DelAckTimer(FullTcpAgent *a) : TimerHandler(), a_(a) { }
protected:
	virtual void expire(Event *);
	FullTcpAgent *a_;
};

class ReassemblyQueue : public TcpAgent {
	struct seginfo {
		seginfo* next_;
		seginfo* prev_;
		int startseq_;
		int endseq_;
		int flags_;
	};

public:
	ReassemblyQueue(int& nxt) : head_(NULL), tail_(NULL),
		rcv_nxt_(nxt) { }
	int empty() { return (head_ == NULL); }
	int add(Packet*);
	void clear();
protected:
	seginfo* head_;
	seginfo* tail_;
	int& rcv_nxt_;
};

class FullTcpAgent : public TcpAgent {
 public:
	FullTcpAgent();
	~FullTcpAgent();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno); 	// tcp_timers() in real code
	void advanceby(int);	// over-rides tcp base version
	void advance_bytes(int);	// unique to full-tcp
	int command(int argc, const char*const* argv);

 protected:
	int segs_per_ack_;  // for window updates
	int nodelay_;       // disable sender-side Nagle?
	int data_on_syn_;   // send data on initial SYN?
	double last_send_time_;	// time of last send
	int close_on_empty_;	// close conn when buffer empty
	int tcprexmtthresh_;    // fast retransmit threshold
	int iss_;       // initial send seq number
	int irs_;	// initial recv'd # (peer's iss)
	int dupseg_fix_;    // fix bug with dup segs and dup acks?
	int dupack_reset_;  // zero dupacks on dataful dup acks?
	int delay_growth_;  // delay opening cwnd until 1st data recv'd
	double delack_interval_;

	int headersize();   // a tcp header
	int outflags();     // state-specific tcp header flags
	int rcvseqinit(int, int); // how to set rcv_nxt_
	int predict_ok(Packet*); // predicate for recv-side header prediction
	int idle_restart();	// should I restart after idle?
	void fast_retransmit(int);  // do a fast-retransmit on specified seg
	inline double now() { return Scheduler::instance().clock(); }
	void newstate(int ns) { state_ = ns; } // future hook for traces

	void reset_rtx_timer(int);  // adjust the rtx timer
	void reset();       // reset to a known point
	void connect();     // do active open
	void listen();      // do passive open
	void usrclosed();   // user requested a close
	int need_send();    // need to send ACK/win-update now?
	void output(int seqno, int reason = 0); // output 1 packet
	void send_much(int force, int reason, int maxburst = 0);
	void sendpacket(int seq, int ack, int flags, int dlen, int why);
	void newack(Packet* pkt);   // process an ACK
	DelAckTimer delack_timer_;	// other timers in tcp.h
	void cancel_timers() {
		TcpAgent::cancel_timers();
		delack_timer_.force_cancel();
	}

	/*
	* the following are part of a tcpcb in "real" RFC793 TCP
	*/
	int maxseg_;        /* MSS */
	int flags_;     /* controls next output() call */
	int state_;     /* enumerated type: FSM state */
	int last_state_; /* FSM state at last pkt recv */
	int rcv_nxt_;       /* next sequence number expected */
	ReassemblyQueue rq_;    /* TCP reassembly queue */
	/*
	* the following are part of a tcpcb in "real" RFC1323 TCP
	*/
	int last_ack_sent_; /* ackno field from last segment we sent */
};

#endif
