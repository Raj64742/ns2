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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp.h,v 1.4.2.5 1997/04/20 19:30:56 hari Exp $ (LBL)
 */

#ifndef ns_tcp_h
#define ns_tcp_h

#include "agent.h"
#include "packet.h"

struct hdr_tcp {
#define NSA 3
        double ts_;             /* time packet generated (at source) */
	int seqno_;		/* sequence number */
	int reason_;		/* reason for a retransmit */
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

	/* per-field member functions */
	double& ts() {
		return (ts_);
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
};

struct hdr_tcpasym {
	int ackcount_;          /* the number of segments this ack represents */
	double win_;            /* the amount of window remaining */
	int highest_ack_;       /* the highest ack seen */
	int max_left_to_send_;  /* the max. amount of data that remains to be sent */
	int& ackcount() {
		return (ackcount_);
	}
	double& win() {
		return (win_);
	}
	int& highest_ack() {
		return (highest_ack_);
	}
	int& max_left_to_send() {
		return (max_left_to_send_);
	}
};

#define TCP_BETA 2.0
#define TCP_ALPHA 0.125

/* these are used to mark packets as to why we xmitted them */
#define TCP_REASON_TIMEOUT	0x01
#define	TCP_REASON_DUPACK	0x02

/*
 * tcp_tick_:
 * default 0.1,
 * 0.3 for 4.3 BSD, 
 * 0.01 for new window algorithms,
 * 0.0001 or 0.0005 for ATM
 */

#define NUMDUPACKS 3		/* normally 3, sometimes 1 */

#define TCP_TIMER_RTX		0
#define TCP_TIMER_DELSND	1


/* Macro to log the *specified* member whenever its value changes */
#define TCP_TRACE(memb, old_memb, memb_time, name) { \
		       Scheduler& s = Scheduler::instance(); \
		       if (memb != old_memb) { \
			       fprintf(stderr,"time: %-6.3f %s %-6.3f\n", memb_time, name, memb); \
			       old_memb = memb; \
		       } \
		       if (&s) \
			       memb_time = s.clock(); \
		       else \
			       memb_time = 0; \
}

/*
 * Macro to log a set of state variables. It is triggered when the value of one of
 * the variables changes. All the state variables are printed out in a single line.
 */ 
#define TCP_TRACE_ALL(memb, old_memb, memb_time) { \
		       double cur_time; \
		       Scheduler& s = Scheduler::instance(); \
                       char wrk[500]; \
                       int n; \
		       if (&s) \
			       cur_time = s.clock(); \
		       else \
			       cur_time = 0; \
		       if (memb != old_memb && cur_time > memb_time) { \
                               if (memb_time > last_log_time_) { \
                                       sprintf(wrk,"time: %-6.3f saddr: %-2d sport: %-2d daddr: %-2d dport: %-2d maxseq: %-4d hiack: %-4d seqno: %-4d cwnd: %-6.3f ssthresh: %-3d dupacks: %-2d rtt: %-6.3f srtt: %-6.3f rttvar: %-6.3f bkoff: %-d", memb_time, addr_/256, addr_%256, dst_/256, dst_%256, maxseq_, highest_ack_, t_seqno_, cwnd_, ssthresh_, dupacks_, t_rtt_*tcp_tick_, (t_srtt_ >> 3)*tcp_tick_, (t_rttvar_ >> 2)*tcp_tick_, t_backoff_); \
                                       n = strlen(wrk); \
                                       wrk[n] = '\n'; \
                                       wrk[n+1] = 0; \
				       if (channel_) \
					       (void)Tcl_Write(channel_, wrk, n+1); \
                                       wrk[n] = 0; \
			               last_log_time_ = memb_time; \
			       } \
			       old_memb = memb; \
		       } \
                       memb_time = cur_time; \
}

#ifdef NO_TCP_TRACE
#undef TCP_TRACE_ALL
#undef TCP_TRACE
#define TCP_TRACE_ALL
#define TCP_TRACE
#endif

#ifdef 0
class InstVarTrace {
public:
	virtual void update() = 0;
};

class cwndTrace : InstVarTrace {
public:
	cwndTrace() {}
	void update() {
		fprintf(stderr, "cwnd = %g\n", *cwnd_ptr_);
	}
	double*& cwnd_ptr() {
		return(cwnd_ptr_);
	}
private:
	double* cwnd_ptr_;
};
#endif
		       
class TcpAgent : public Agent {
public:
	TcpAgent();
	virtual void recv(Packet*, Handler*);
	int command(int argc, const char*const* argv);

/*	cwndTrace cwnd_trace_;*/

	int& maxseq() {
		TCP_TRACE_ALL(maxseq_, old_maxseq_, maxseq_time_);
		return (maxseq_);
	}

	int& highest_ack() {
		TCP_TRACE_ALL(highest_ack_, old_highest_ack_, highest_ack_time_);
		return (highest_ack_);
	}

	int& t_seqno() {
		TCP_TRACE_ALL(t_seqno_, old_t_seqno_, t_seqno_time_);
		return (t_seqno_);
	}

	double& cwnd() {
		TCP_TRACE_ALL(cwnd_, old_cwnd_, cwnd_time_);
		return (cwnd_);
	}

	int& ssthresh() {
		TCP_TRACE_ALL(ssthresh_, old_ssthresh_, ssthresh_time_);
		return (ssthresh_);
	}

	int& dupacks() {
		TCP_TRACE_ALL(dupacks_, old_dupacks_, dupacks_time_);
		return (dupacks_);
	}

	int& t_rtt() {
		TCP_TRACE_ALL(t_rtt_, old_t_rtt_, t_rtt_time_);
		return (t_rtt_);
	}

	int& t_srtt() {
		TCP_TRACE_ALL(t_srtt_, old_t_srtt_, t_srtt_time_);
		return (t_srtt_);
	}

	int& t_rttvar() {
		TCP_TRACE_ALL(t_rttvar_, old_t_rttvar_, t_rttvar_time_);
		return (t_rttvar_);
	}

	int& t_backoff() {
		TCP_TRACE_ALL(t_backoff_, old_t_backoff_, t_backoff_time_);
		return (t_backoff_);
	}


protected:
	virtual int window();
	virtual void plot();

	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	int t_seqno_;		/* sequence number */
	int t_rtt_;      	/* round trip time */
	int t_srtt_;     	/* smoothed round-trip time */
	int t_rttvar_;   	/* variance in round-trip time */
	int t_backoff_;
	void rtt_init();
	double rtt_timeout();
	void rtt_update(double tao);
	void rtt_backoff();

	/*XXX start/stop */
	void output(int seqno, int reason = 0);
	virtual void send(int force, int reason, int maxburst = 0);
	void set_rtx_timer();
	void reset_rtx_timer(int mild);
	void newtimer(Packet* pkt);
	void opencwnd();
	void closecwnd(int how);
	virtual void timeout(int tno);
	void reset();
	void newack(Packet* pkt);
	void quench(int how);

	double overhead_;
	double wnd_;
	double wnd_const_;
	double wnd_th_;		/* window "threshold" */
	double wnd_init_;
	double tcp_tick_;	/* clock granularity */
	int wnd_option_;
	int bug_fix_;		/* 1 for multiple-fast-retransmit fix */
	int maxburst_;		/* max # packets can send back-2-back */
	int maxcwnd_;		/* max # cwnd can ever be */
	FILE *plotfile_;
	/*
	 * Dynamic state.
	 */
	int dupacks_;		/* number of duplicate acks */
	int curseq_;		/* highest seqno "produced by app" */
	int last_ack_;		/* largest consecutive ACK, frozen during
				 *		Fast Recovery */
	int highest_ack_;	/* not frozen during Fast Recovery */
	int recover_;		/* highest pkt sent before dup acks, */
				/*   timeout, or source quench 	*/
	int recover_cause_;	/* 1 for dup acks */
				/* 2 for timeout */
				/* 3 for source quench */
	double cwnd_;		/* current window */
	double awnd_;		/* averaged window */
	int ssthresh_;		/* slow start threshold */
	int count_;		/* used in window increment algorithms */
	double fcnt_;		/* used in window increment algorithms */
	int rtt_active_;	/* 1 for a first-time transmission of a pkt */
	int rtt_seq_;		/* first-time seqno sent after retransmits */
	int maxseq_;		/* used for Karn algorithm */
				/* highest seqno sent so far */
	int ecn_;		/* 1 to avoid multiple Fast Retransmits */

	int off_ip_;
	int off_tcp_;

private:
	double last_log_time_; /* the time at which the state variables were logged
				  last */
	/* 
	 * The following members are used to keep track of the previous value
	 * of various state variables and the time of the last change.
	 */
	int old_maxseq_;
	double maxseq_time_;
	int old_highest_ack_;
	double highest_ack_time_;
	int old_t_seqno_;
	double t_seqno_time_;
	double old_cwnd_;
	double cwnd_time_;
	int old_ssthresh_;
	double ssthresh_time_;
	int old_dupacks_;
	double dupacks_time_;
	int old_t_rtt_;
	double t_rtt_time_;
	int old_t_srtt_;
	double t_srtt_time_;
	int old_t_rttvar_;
	double t_rttvar_time_;
	int old_t_backoff_;
	double t_backoff_time_;
};

#endif
