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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp.h,v 1.46 1998/05/11 18:51:26 kfall Exp $ (LBL)
 */
#ifndef ns_tcp_h
#define ns_tcp_h

#include "agent.h"
#include "packet.h"

struct hdr_tcp {
#define NSA 3
	double ts_;             /* time packet generated (at source) */
	double ts_echo_;        /* the echoed timestamp (originally sent by
	                           the peer */
	int seqno_;             /* sequence number */
	int reason_;            /* reason for a retransmit */
	int sa_start_[NSA+1];   /* selective ack "holes" in packet stream */
	int sa_end_[NSA+1];     /* For each hole i, sa_start[i] contains the
	                         * starting packet sequence no., and sa_end[i]  
	                         * contains the ending packet sequence no. */
	int sa_left_[NSA+1];
	int sa_right_[NSA+1];   /* In Jamshid's implementation, this    *
	                         * pair of variables represents the blocks*
	                         * not the holes.                         */
	int sa_length_;         /* Indicate the number of SACKs in this  *
	                         * packet.  Adds 2+sack_length*8 bytes   */ 
	int ackno_;             /* ACK number for FullTcp */
	int hlen_;              /* header len (bytes) for FullTcp */
	int tcp_flags_;         /* TCP flags for FullTcp */


	/* per-field member functions */
	double& ts() {
		return (ts_);
	}
	double& ts_echo() {
		return (ts_echo_);
	}
	int& seqno() {
		return (seqno_);
	}
	int& reason() {
		return (reason_);
	}
	int* sa_start() {
		return (sa_start_);
	}
	int* sa_end() {
		return (sa_end_);
	}
	int* sa_left() {
		return (sa_left_);
	}
	int* sa_right() {
		return (sa_right_);
	}
	int& sa_length() {
		return (sa_length_);
	}
	int& hlen() {
		return (hlen_);
	}
	int& ackno() {
		return (ackno_);
	}  
	int& flags() {
		return (tcp_flags_);
	}
};

/* 
 * The TCP asym header. The sender includes information that the receiver could
 * use to decide by how much to delay acks.
 * XXXX some of the fields may not be needed
 */ 
struct hdr_tcpasym {
	int ackcount_;          /* the number of segments this ack represents */
	int win_;               /* the amount of window remaining */
	int highest_ack_;       /* the highest ack seen */
	int max_left_to_send_;  /* the max. amount of data that remains to be sent */
	int& ackcount() {
		return (ackcount_);
	}
	int& win() {
		return (win_);
	}
	int& highest_ack() {
		return (highest_ack_);
	}
	int& max_left_to_send() {
		return (max_left_to_send_);
	}
};

/* these are used to mark packets as to why we xmitted them */
#define TCP_REASON_TIMEOUT	0x01
#define	TCP_REASON_DUPACK	0x02
#define	TCP_REASON_RBP		0x03   // used only in tcp-rbp.cc

/* these are reasons we adjusted our congestion window */

#define	CWND_ACTION_DUPACK	1	// dup acks/fast retransmit
#define	CWND_ACTION_TIMEOUT	2	// retransmission timeout
#define	CWND_ACTION_ECN		3	// ECN bit [src quench if supported]

/* these are bits for how to change the cwnd and ssthresh values */

#define	CLOSE_SSTHRESH_HALF	0x00000001
#define	CLOSE_CWND_HALF		0x00000002
#define	CLOSE_CWND_RESTART	0x00000004
#define	CLOSE_CWND_INIT		0x00000008
#define	CLOSE_CWND_ONE		0x00000010

/*
 * tcp_tick_:
 * default 0.1,
 * 0.3 for 4.3 BSD, 
 * 0.01 for new window algorithms,
 */

#define NUMDUPACKS 3		/* normally 3, sometimes 1 */

#define TCP_TIMER_RTX		0
#define TCP_TIMER_DELSND	1
#define TCP_TIMER_BURSTSND	2
#define TCP_TIMER_DELACK	3
#define TCP_TIMER_RESET         4

class TcpAgent;

class RtxTimer : public TimerHandler {
public: 
	RtxTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

class DelSndTimer : public TimerHandler {
public: 
	DelSndTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

class BurstSndTimer : public TimerHandler {
public: 
	BurstSndTimer(TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	TcpAgent *a_;
};

class TcpAgent : public Agent {
public:
	TcpAgent();

	virtual void recv(Packet*, Handler*);
	virtual void timeout(int tno);
	virtual void timeout_nonrtx(int tno);
	int command(int argc, const char*const* argv);

	void trace(TracedVar* v);
	virtual void advanceby(int delta);
protected:
	virtual int window();
	print_if_needed(double memb_time);
	void traceAll();
	virtual void traceVar(TracedVar* v);

	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	TracedInt t_seqno_;	/* sequence number */
#define T_RTT_BITS 0
	TracedInt t_rtt_;      	/* round trip time */
#define T_SRTT_BITS 3
	TracedInt t_srtt_;     	/* smoothed round-trip time */
	int srtt_init_;		/* initial value for computing t_srtt_ */
#define T_RTTVAR_BITS 2
	TracedInt t_rttvar_;   	/* variance in round-trip time */
	int rttvar_init_;       /* initial value for computing t_rttvar_ */
	double t_rtxcur_;	/* current retransmit value */
	double rtxcur_init_;       /* initial value for t_rtxcur_ */
	TracedInt t_backoff_;	/* current multiplier, 1 if not backed off */
	virtual void rtt_init();
	virtual double rtt_timeout();	/* provide an RTO based on RTT estimates */
	virtual void rtt_update(double tao);	/* update RTT estimate with sample */
	virtual void rtt_backoff();		/* double multiplier */

	double ts_peer_;        /* the most recent timestamp the peer sent */

	/* connection and packet dynamics */
	virtual void output(int seqno, int reason = 0);
	virtual void send_much(int force, int reason, int maxburst = 0);
	virtual void newtimer(Packet*);
	virtual void dupack_action();		/* do this on dupacks */
	void opencwnd();
	void slowdown(int how);			/* reduce cwnd/ssthresh */
	void ecn();				/* react to quench */
	virtual void set_initial_window();	/* set IW */
	double initial_window();		/* what is IW? */
	void reset();
	void newack(Packet*);
	void tcp_eln(Packet *pkt); /* reaction to ELN (usually wireless) */
	void finish(); /* called when the connection is terminated */

	/* Helper functions. Currently used by tcp-asym */
	virtual void output_helper(Packet*) { return; }
	virtual void send_helper(int) { return; }
	virtual void send_idle_helper() { return; }
	virtual void recv_helper(Packet*) { return; }
	virtual void recv_newack_helper(Packet*);
	virtual void partialnewack_helper(Packet*) {};

	/* Timers */
	RtxTimer rtx_timer_;
	DelSndTimer delsnd_timer_;
	BurstSndTimer burstsnd_timer_;
	virtual void cancel_timers() {
		rtx_timer_.force_cancel();
		burstsnd_timer_.force_cancel();
		delsnd_timer_.force_cancel();
	}
	virtual void cancel_rtx_timer() {
		rtx_timer_.force_cancel();
	}
	virtual void set_rtx_timer();
	void reset_rtx_timer(int mild, int backoff = 1);

	double boot_time_;	/* where between 'ticks' this sytem came up */
	double overhead_;
	double wnd_;
	double wnd_const_;
	double wnd_th_;		/* window "threshold" */
	double wnd_init_;
	double wnd_restart_;
	double tcp_tick_;	/* clock granularity */
	int wnd_option_;
	int wnd_init_option_;   /* 1 for using wnd_init_ */
				/* 2 for using large initial windows */
	int syn_;		/* 1 for modeling SYN/ACK exchange */
	int delay_growth_;  	/* delay opening cwnd until 1st data recv'd */
	int tcpip_base_hdr_size_;  /* size of base TCP/IP header */
	int bug_fix_;		/* 1 for multiple-fast-retransmit fix */
	int ts_option_;		/* use RFC1323-like timestamps? */
	int maxburst_;		/* max # packets can send back-2-back */
	int maxcwnd_;		/* max # cwnd can ever be */
	double maxrto_;		/* max value of an RTO */
	FILE *plotfile_;
	/*
	 * Dynamic state.
	 */
	TracedInt dupacks_;	/* number of duplicate acks */
	TracedInt curseq_;	/* highest seqno "produced by app" */
	int last_ack_;		/* largest consecutive ACK, frozen during
				 *		Fast Recovery */
	TracedInt highest_ack_;	/* not frozen during Fast Recovery */
	int recover_;		/* highest pkt sent before dup acks, */
				/*   timeout, or source quench 	*/
	int last_cwnd_action_;	/* CWND_ACTION_{TIMEOUT,DUPACK,ECN} */
	TracedDouble cwnd_;	/* current window */
	double base_cwnd_;	/* base window (for experimental purposes) */
	double awnd_;		/* averaged window */
	TracedInt ssthresh_;	/* slow start threshold */
	int count_;		/* used in window increment algorithms */
	double fcnt_;		/* used in window increment algorithms */
	int rtt_active_;	/* 1 if a rtt sample is pending */
	int rtt_seq_;		/* seq # of timed seg if rtt_active_ is 1 */
	double rtt_ts_;		/* time at which rtt_seq_ was sent */
	TracedInt maxseq_;	/* used for Karn algorithm */
				/* highest seqno sent so far */
	int ecn_;		/* Explicit Congestion Notification */
        int eln_;               /* Explicit Loss Notification (wireless) */
        int eln_rxmit_thresh_;  /* Threshold for ELN-triggered rxmissions */
        int eln_last_rxmit_;    /* Last packet rxmitted due to ELN info */
	double firstsent_;	/* When first packet was sent  --Allman */
	int off_tcp_;
	int slow_start_restart_; /* boolean: re-init cwnd after connection 
				    goes idle.  On by default. */
	int restart_bugfix_;    /* ssthresh is cut down because of
				   timeouts during a connection's idle period.
				   Setting this boolean fixes this problem.
				   For now, it is off by default. */ 
	int closed_;            /* whether this connection has closed */
        TracedInt ndatapack_;   /* number of data packets sent */
        TracedInt ndatabytes_;  /* number of data bytes sent */
        TracedInt nackpack_;    /* number of ack packets received */
        TracedInt nrexmit_;     /* number of retransmit timeouts 
				   when there was data outstanding */
        TracedInt nrexmitpack_; /* number of retransmited packets */
        TracedInt nrexmitbytes_; /* number of retransmited bytes */
	int trace_all_oneline_;	/* TCP tracing vars all in one line or not? */

	/* these function are now obsolete, see other above */
	void closecwnd(int how);
	void quench(int how);
};

/* TCP Reno */
class RenoTcpAgent : public virtual TcpAgent {
 public:
	RenoTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
 protected:
	unsigned int dupwnd_;
};

/* TCP New Reno */
class NewRenoTcpAgent : public virtual TcpAgent {
 public:
	NewRenoTcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	virtual void partialnewack_helper(Packet* pkt);
 protected:
	unsigned int dupwnd_;
	int newreno_changes_;	/* 0 for fixing unnecessary fast retransmits */
				/* 1 for additional code from Allman, */
				/* to implement other algorithms from */
				/* Hoe's paper */
	void partialnewack(Packet *pkt);
	int acked_, new_ssthresh_;  /* used if newreno_changes_ == 1 */
	double ack2_, ack3_, basertt_; /* used if newreno_changes_ == 1 */
};

/* TCP vegas (VegasTcpAgent) */
class VegasTcpAgent : public virtual TcpAgent {
 public:
	VegasTcpAgent();
	virtual void recv(Packet *pkt, Handler *);
	virtual void timeout(int tno);
protected:
	double vegastime() {
		return(Scheduler::instance().clock() - firstsent_);
	}
	virtual void output(int seqno, int reason = 0);
	virtual void recv_newack_helper(Packet*);
	int vegas_expire(Packet*); 
	void reset();

	double t_cwnd_changed_; // last time cwnd changed
	double firstrecv_;	// time recv the 1st ack

	int    v_alpha_;    	// vegas thruput thresholds in pkts
	int    v_beta_;  	    	

	int    v_gamma_;    	// threshold to change from slow-start to
				// congestion avoidance, in pkts

	int    v_slowstart_;    // # of pkts to send after slow-start, deflt(2)
	int    v_worried_;      // # of pkts to chk after dup ack (1 or 2)

	double v_timeout_;      // based on fine-grained timer
	double v_rtt_;		
	double v_sa_;		
	double v_sd_;	

	int    v_cntRTT_;       // # of rtt measured within one rtt
	double v_sumRTT_;       // sum of rtt measured within one rtt

	double v_begtime_;	// tagged pkt sent
	int    v_begseq_;	// tagged pkt seqno

	double* v_sendtime_;	// each unacked pkt's sendtime is recorded.
	int*   v_transmits_;	// # of retx for an unacked pkt

	int    v_maxwnd_;	// maxwnd size for v_sendtime_[]
	double v_newcwnd_;	// record un-inflated cwnd

	double v_baseRTT_;	// min of all rtt

	double v_incr_;		// amount cwnd is increased in the next rtt
	int    v_inc_flag_;	// if cwnd is allowed to incr for this rtt

	double v_actual_;	// actual send rate (pkt/s; needed for tcp-rbp)
};

// Local Variables:
// mode:c++
// End:

#endif
