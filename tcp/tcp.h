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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp.h,v 1.3 1997/03/28 20:25:55 mccanne Exp $ (LBL)
 */

#ifndef ns_tcp_h
#define ns_tcp_h

#include "agent.h"
#include "packet.h"

struct bd_tcp {
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

/*XXX*/
class TCPHeader;
extern TCPHeader tcphdr;

class TCPHeader : public PacketHeader {
public:
	inline int hdrsize() { return (sizeof(bd_tcp)); }
        static inline bd_tcp* access(u_char *p) {
                return ((bd_tcp*)(p + tcphdr.offset_));
        }
};

typedef bd_tcp hdr_tcp;

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

class TcpAgent : public Agent {
public:
	TcpAgent();
	virtual void recv(Packet*, Handler*);
	int command(int argc, const char*const* argv);
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
};

#endif
