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

#ifndef CHOST
#define CHOST
#include "agent.h"
#include "packet.h"
#include "tcp.h"
#include "tcp-int.h"
#include "nilist.h"

#define ROUND_ROBIN 1
#define RANDOM 2

class Segment : public slink {
	friend class CorresHost;
  public:
	Segment() {ts_ = 0;
	seqno_ = later_acks_ = dport_ = sport_ = size_ = rxmitted_ = 
		daddr_ = 0;};
  protected:
	int seqno_;
	int daddr_;
	int dport_;
	int sport_;
	int size_;
	double ts_;		/* timestamp */
	int later_acks_;
	short rxmitted_;
	class IntTcpAgent *sender_;
};

class CorresHost : public slink, public TclObject {
	friend class IntTcpAgent;
  public:
	CorresHost(int addr, int cwndinit = 0, int path_mtu_ = 1500, 
		   int maxcwnd = 999999, int wnd_option = 0 );
	/* add pkt to pipe */
	void add_pkts(int size, int seqno, int daddr, int dport, int sport, 
		      double ts, IntTcpAgent *sender); 
	/* remove pkt from pipe */
	int clean_segs(int size, Packet *pkt, IntTcpAgent *sender,
		       int clean_dup = 1, int uniq_ts = 0);
	int rmv_old_segs(Packet *pkt, IntTcpAgent *sender, int clean_dup = 1,
			 int uniq_ts = 0);

	void opencwnd(int size, IntTcpAgent *sender=0);
	void closecwnd(int how, double ts, IntTcpAgent *sender=0);
	void closecwnd(int how, IntTcpAgent *sender=0);
	int ok_to_snd(int size);
	IntTcpAgent *who_to_snd(int how);
	void add_agent(IntTcpAgent *agent, int size, double winMult,
		       int winInc, int ssthresh);
	void del_agent(IntTcpAgent *agent) {nActive_--;};
	void agent_tout(IntTcpAgent *agent) {nTimeout_++;};
	void agent_ftout(IntTcpAgent *agent) {nTimeout_--;};
	void agent_rcov(IntTcpAgent *agent) {nFastRec_++;};
	void agent_frcov(IntTcpAgent *agent) {nFastRec_--;};
	void quench(int how);

  protected:
	int addr_;	     /* my addr */
	class Islist<IntTcpAgent> conns_; /* active connections */
	Islist_iter<IntTcpAgent> *connIter_;
	u_int nActive_;	     /* number of active tcp conns to this host */
	u_int nTimeout_;     /* number of tcp conns to this host in timeout */
	u_int nFastRec_;     /* number of tcp conns to this host in recovery */
	double closecwTS_; 
	double winMult_;
	int winInc_;
	TracedDouble cwnd_;	     /* total cwnd for host */
	int maxcwnd_;	     /* max # cwnd can ever be */
	TracedDouble ownd_;	     /* outstanding data to host */
	TracedInt ssthresh_;	     /* slow start threshold */
	int count_;	     /* used in window increment algorithms */
	double fcnt_;	     /* used in window increment algorithms */
	int wndOption_;
	int pathmtu_;	     /* should do path mtu discovery here */
			     /* should also do t/tcp cache info here */
	int proxyopt_;	     /* indicates whether the connections are on behalf
			        of distinct users (like those from a proxy) */
	class Islist<Segment> seglist_;	/* list of unack'd segments to peer */
	double lastackTS_;
	/*
	 * State encompassing the round-trip-time estimate.
	 * srtt and rttvar are stored as fixed point;
	 * srtt has 3 bits to the right of the binary point, rttvar has 2.
	 */
	int t_rtt_;	     /* round trip time */
	int t_srtt_;	     /* smoothed round-trip time */
	int t_rttvar_;	     /* variance in round-trip time */
	int t_backoff_;	     /* timer backoff value */
	void rtt_init();
	double rtt_timeout();
	void rtt_update(double tao);
	void rtt_backoff();
	double wndInit_;    /* should = path_mtu_ */

	/* following is for right-edge timer recovery */
	int pending_;
	Event timer_;
	inline void cancel() {	
		(void)Scheduler::instance().cancel(&timer_);
		pending_ = 0;
	}
	int off_tcp_;
};

#endif

