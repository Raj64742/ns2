/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 * ns-1 implementation:
 *
 * This is an implementation of U. of Arizona's TCP Vegas. I implemented
 * it based on USC's NetBSD-Vegas.
 *					Ted Kuo
 *					North Carolina St. Univ. and
 *					Networking Software Div, IBM
 *					tkuo@eos.ncsu.edu
 **
 * The ns-2 implementation includes contribued fixes described at the
 * end of the file.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-vegas.cc,v 1.24 1999/07/16 17:06:19 heideman Exp $ (NCSU/IBM)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#define MIN(x, y) ((x)<(y) ? (x) : (y))


static class VegasTcpClass : public TclClass {
public:
	VegasTcpClass() : TclClass("Agent/TCP/Vegas") {}
	TclObject* create(int, const char*const*) {
		return (new VegasTcpAgent());
	}
} class_vegas;


VegasTcpAgent::VegasTcpAgent() : TcpAgent()
{
	/* init vegas var */
	bind("v_alpha_", &v_alpha_);
	bind("v_beta_", &v_beta_);
	bind("v_gamma_", &v_gamma_);
	bind("v_rtt_", &v_rtt_);
	// ns_vegas_fix_level_: see comment at the end of this file
	bind("ns_vegas_fix_level_", &ns_vegas_fix_level_);

	reset();
}

void
VegasTcpAgent::reset()
{
	t_cwnd_changed_ = 0.;
	firstrecv_ = -1.0;
	v_slowstart_ = 2;
	v_sa_ = 0;
	v_sd_ = 0;
	v_timeout_ = 1000.;
	v_worried_ = 0;
	v_begseq_ = 0;
	v_begtime_ = 0.;
	v_cntRTT_ = 0; v_sumRTT_ = 0.;
	v_baseRTT_ = 1000000000.;
	v_incr_ = 0;
	v_inc_flag_ = 1;

	TcpAgent::reset();
}

void
VegasTcpAgent::recv_newack_helper(Packet *pkt)
{
	//hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	newack(pkt);
#if 0
	// like TcpAgent::recv_newack_helper, but without this
	if ( !((hdr_flags*)pkt->access(off_flags_))->ecnecho() || !ecn_ ) {
	        opencwnd();
	}
#endif
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void
VegasTcpAgent::recv(Packet *pkt, Handler *)
{
	double currentTime = vegastime();
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_flags *flagh = (hdr_flags*)pkt->access(off_flags_);

#if 0
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"recieved non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif /* 0 */
	++nackpack_;

	if(firstrecv_<0) { // init vegas rtt vars
		firstrecv_ = currentTime;
		v_baseRTT_ = v_rtt_ = firstrecv_;
		v_sa_  = v_rtt_ * 8.;
		v_sd_  = v_rtt_;
		v_timeout_ = ((v_sa_/4.)+v_sd_)/2.;
	}

	if (flagh->ecnecho())
		ecn(tcph->seqno());
	if (tcph->seqno() > last_ack_) {
		if (last_ack_ == 0 && delay_growth_) {
			cwnd_ = initial_window();
		}
		/* check if cwnd has been inflated */
		if ((dupacks_ > NUMDUPACKS || (ns_vegas_fix_level_ >= 3 && dupacks_ == NUMDUPACKS)) &&
		    cwnd_ > v_newcwnd_) {
			cwnd_ = v_newcwnd_;
			// vegas ssthresh is used only during slow-start
			ssthresh_ = 2;
		}
		int oldack = last_ack_;

		recv_newack_helper(pkt);
		
		/*
		 * begin of once per-rtt actions
		 * 	1. update path fine-grained rtt and baseRTT
		 *      2. decide what to do with cwnd_, inc/dec/unchanged
		 *         based on delta=expect - actual.
		 */
		if(tcph->seqno() >= v_begseq_) {
			double rtt;
			if(v_cntRTT_ > 0)
				rtt = v_sumRTT_ / v_cntRTT_;
			else 
				rtt = currentTime - v_begtime_;

			v_sumRTT_ = 0.0;
			v_cntRTT_ = 0;

			// calc # of packets in transit
			int rttLen = t_seqno_ - v_begseq_;

			/*
			 * decide should we incr/decr cwnd_ by how much
			 */
			if(rtt>0) {
				/* if there's only one pkt in transit, update 
			 	 * baseRTT
			 	 */
				if(rtt<v_baseRTT_ || rttLen<=1)
					v_baseRTT_ = rtt;

				double expect;   // in pkt/sec
				// actual = (# in transit)/(current rtt) 
				v_actual_ = double(rttLen)/rtt;
				// expect = (current window size)/baseRTT
				expect = double(t_seqno_-last_ack_)/v_baseRTT_;

				// calc actual and expect thruput diff, delta
				int delta=int((expect-v_actual_)*v_baseRTT_+0.5);
				if(cwnd_ < ssthresh_) { // slow-start
					// adj cwnd every other rtt
					v_inc_flag_ = !v_inc_flag_;
					if(!v_inc_flag_)
						v_incr_ = 0;
					else {
					    if(delta > v_gamma_) {
						// slow-down a bit to ensure
						// the net is not so congested
						ssthresh_ = 2;
						cwnd_-=(cwnd_/8);
						if(cwnd_<2)
							cwnd_ = 2.;
						v_incr_ = 0;
					    } else 
						v_incr_ = 1;
					}
				} else { // congestion avoidance
					if(delta>v_beta_) {
						/*
						 * slow down a bit, retrack
						 * back to prev. rtt's cwnd
						 * and dont incr in the nxt rtt
						 */
						--cwnd_;
						if(cwnd_<2) cwnd_ = 2;
						v_incr_ = 0;
					} else if(delta<v_alpha_)
						// delta<alpha, faster....
						v_incr_ = 1/cwnd_;
					else // current rate is cool.
						v_incr_ = 0;
				}
			} // end of if(rtt > 0)

			// tag the next packet 
			v_begseq_ = t_seqno_; 
			v_begtime_ = currentTime;
		} // end of once per-rtt section

		/* since we set how much to incr only once per rtt,
		 * need to check if we surpass ssthresh during slow-start
		 * before the rtt is over.
		 */		
		if(v_incr_ == 1 && cwnd_ >= ssthresh_)
			v_incr_ = 0;
		
		/*
		 * incr cwnd unless we havent been able to keep up with it
		 */
		if(v_incr_>0 && (cwnd_-(t_seqno_-last_ack_))<=2)
			cwnd_ = cwnd_+v_incr_;	

		/*
		 * See if we need to update the fine grained timeout value,
		 * v_timeout_
		 */

		// reset v_sendtime for acked pkts and incr v_transmits_
		double sendTime = v_sendtime_[tcph->seqno()%v_maxwnd_];
		int transmits = v_transmits_[tcph->seqno()% v_maxwnd_];
		int range = tcph->seqno() - oldack;
		for(int k=((oldack+1) %v_maxwnd_); \
			k<=(tcph->seqno()%v_maxwnd_) && range >0 ; \
			k=((++k) % v_maxwnd_), range--) {
			v_sendtime_[k] = -1.0;
			v_transmits_[k] = 0;
		}

		if((sendTime !=0.) && (transmits==1)) {
			 // update fine-grained timeout value, v_timeout_.
			double rtt, n;
			rtt = currentTime - sendTime;
			v_sumRTT_ += rtt;
			++v_cntRTT_;
			if(rtt>0) {
				v_rtt_ = rtt;
				if(v_rtt_ < v_baseRTT_)
					v_baseRTT_ = v_rtt_;
				n = v_rtt_ - v_sa_/8;
				v_sa_ += n;
				n = n<0 ? -n : n;
				n -= v_sd_ / 4;
				v_sd_ += n;
				v_timeout_ = ((v_sa_/4)+v_sd_)/2;
				v_timeout_ += (v_timeout_/16);
			}
		}

		/* 
		 * check the 1st or 2nd acks after dup ack received 
		 */
		if(v_worried_>0) {
			/*
			 * check if any pkt has been timeout. if so, 
			 * retx it. no need to change cwnd since we
			 * already did.
			 */
			--v_worried_;
			int expired=vegas_expire(pkt);
			if(expired>=0) {
				dupacks_ = NUMDUPACKS;
				output(expired, TCP_REASON_DUPACK);
			} else
				v_worried_ = 0;
		}
   	} else if (tcph->seqno() == last_ack_)  {
		/* check if a timeout should happen */
		++dupacks_; 
		int expired=vegas_expire(pkt);
		if (expired>=0 || dupacks_ == NUMDUPACKS) {
			double sendTime=v_sendtime_[(last_ack_+1) % v_maxwnd_]; 
			int transmits=v_transmits_[(last_ack_+1) % v_maxwnd_];
			int win = window();
       	                /* The line below, for "bug_fix_" true, avoids
                        * problems with multiple fast retransmits after
			* a retransmit timeout.
                        */
			if ( !bug_fix_ || (highest_ack_ > recover_) || \
			    ( last_cwnd_action_ != CWND_ACTION_TIMEOUT)) {
				last_cwnd_action_ = CWND_ACTION_DUPACK;
				recover_ = maxseq_;
				/* check for timeout after recv a new ack */
				v_worried_ = MIN(2, t_seqno_ - last_ack_ );
		
				/* v_rto expon. backoff */
				if(transmits > 1) 
					v_timeout_ *=2.; 
				else
					v_timeout_ += (v_timeout_/8.);
				/*
				 * if cwnd hasnt changed since the pkt was sent
				 * we need to decr it.
				 */
				if (t_cwnd_changed_ < sendTime) {
					if(win<=3)
						win = 2;
					else if(transmits > 1)
						win >>=1;
					else 
						win -= (win>>2);

					// old behavior (pre fix-2)
					// was to do: vegas_inflate_cwnd(win, currentTime);
					// here.
				} 

				// update coarser grained rto
				reset_rtx_timer(1, 0);
				++nrexmit_;
				if(expired>=0) 
					output(expired, TCP_REASON_DUPACK);
				else
					output(last_ack_ + 1, TCP_REASON_DUPACK);
                        }

			// fix-2 is done always: // if (ns_vegas_fix_level_ >= 2)
			vegas_inflate_cwnd(win, currentTime);
					 
			if(transmits==1) 
				dupacks_ = NUMDUPACKS;
		} else if (dupacks_ > NUMDUPACKS) 
			++cwnd_;
	}
	Packet::free(pkt);
#if 0
	if (trace_)
		plot();
#endif /* 0 */

	/*
	 * Try to send more data
	 */
	if (dupacks_ == 0 || dupacks_ > NUMDUPACKS - 1)
		send_much(0, 0, maxburst_);
}

void
VegasTcpAgent::vegas_inflate_cwnd(int win, double current_time)
{
	// record cwnd_
	v_newcwnd_ = double(win);
	// inflate cwnd_
	cwnd_ = v_newcwnd_ + dupacks_;
	t_cwnd_changed_ = current_time;
}

void
VegasTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		if (highest_ack_ == maxseq_ && !slow_start_restart_) {
			/*
			 * TCP option:
			 * If no outstanding data, then don't do anything.
			 *
			 * Note:  in the USC implementation,
			 * slow_start_restart_ == 0.
			 * I don't know what the U. Arizona implementation
			 * defaults to.
			 */
			return;
		};
		dupacks_ = 0;
		recover_ = maxseq_;
		last_cwnd_action_ = CWND_ACTION_TIMEOUT;
		reset_rtx_timer(0);
		slowdown(CLOSE_CWND_RESTART|CLOSE_SSTHRESH_HALF);
		cwnd_ = double(v_slowstart_);
		v_newcwnd_ = 0;
		if (ns_vegas_fix_level_ >= 1)
			v_worried_ = 0;
		t_cwnd_changed_ = vegastime();
		send_much(0, TCP_REASON_TIMEOUT);
	} else {
		/* delayed-sent timer, with random overhead to avoid
		 * phase effect. */
		send_much(1, TCP_REASON_TIMEOUT);
	};
}

void
VegasTcpAgent::output(int seqno, int reason)
{
	Packet* p = allocpkt();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	double now = Scheduler::instance().clock();
	tcph->seqno() = seqno;
	tcph->ts() = now;
	tcph->reason() = reason;

	/* if this is the 1st pkt, setup senttime[] and transmits[]
	 * I alloc mem here, instrad of in the constructor, to cover
	 * cases which windows get set by each different tcp flows */
	if (seqno==0) {
		v_maxwnd_ = int(wnd_);
		v_sendtime_ = new double[v_maxwnd_];
		v_transmits_ = new int[v_maxwnd_];
		for(int i=0;i<v_maxwnd_;i++) {
			v_sendtime_[i] = -1.;
			v_transmits_[i] = 0;
		}
	}

	// record a find grained send time and # of transmits 
	int index = seqno % v_maxwnd_;
	v_sendtime_[index] = vegastime();  
	++v_transmits_[index];

	send(p, 0);
	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far

	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;
				rtt_ts_ = now;
			}
		}
	}
	if (!(rtx_timer_.status() == TIMER_PENDING))
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

/*
 * return -1 if the oldest sent pkt has not been timeout (based on
 * fine grained timer).
 */
int
VegasTcpAgent::vegas_expire(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	double elapse = vegastime() - v_sendtime_[(tcph->seqno()+1)%v_maxwnd_];
	if (elapse >= v_timeout_) {
		return(tcph->seqno()+1);
	}
	return(-1);
}



/*
 * Post-Arizona Vegas fixes:
 *

Date: Thu, 11 Mar 1999 15:01:22 -0500 (EST)
From: Kuang-Yeh Wang <kwang@cs.umd.edu>
To: ns-users@mash.cs.berkeley.edu
Subject: bug report: VegasTcpAgent
Message-ID: <Pine.OSF.3.95.990311133538.2890B-100000@congo.cs.umd.edu>
MIME-Version: 1.0
Content-Type: TEXT/PLAIN; charset=US-ASCII

Problem:

Under certain circumstances, TCP Vegas may set its cwnd_ (congestion
window) to 0 and never transmit any packets again, even though there are
packets waiting to be sent and the network is idle.

Problem source and potential solutions:

tcp-vegas.cc, line 137 "cwnd_ = v_newcwnd_".  It's meant to restore cwnd_
to its value before it was inflated due to duplicate ACKs.  Obviously
VegasTcpAgent should check if cwnd_ has indeed been inflated before doing
this.  Problem is, the check (line 136) "if(dupacks_ > NUMDUPACKS &&
cwnd_ > v_newcwnd_)" is *not enough*.

Here's a case where things go wrong:
1. Duplicate ACKs are received, and "v_worried_" is set to 2 (line 305).
2. Before any new ACKs arrives, a timeout occurs, and therefore
   "v_newcwnd_" is set to 0 (line 379).
3. A new ACK arrives, and it's determined that an outstanding packet has
   "expired" (line 281; note that v_worried_ > 0) and therefore dupacks_ =
   NUMDUPACKS (= 3).
4. A duplicate ACK arrives, so ++dupcaks_ (line 290).  "v_newcwnd_ =
   double(win)" is not executed since we are in "CWND_ACTION_TIMEOUT" 
   state and hence v_newcwnd_ stays 0.
5. A new ACK arrives and cwnd_ is set to v_newcwnd_ (line 137), which is
   0.  The TCP sender drops dead.

I got this case using an error model on a link.

Potential solutions:

1. When timeout occurs, set v_worried_ to 0.  It seems to make no sense to
   "keep worried" in common cases, since usually all outstanding packets
   will be retransmitted anyway after a timeout.

However, the problem may still occur if there are more than 3 duplicate
ACKs right after a timeout and therefore v_newcwnd_ stays 0 and dupacks_
becomes > 3.

2. Inflate cwnd_ even though the test on lines 299, 300 fails.  This
   closes a loophole in trying to guarantee that if dupacks_ > 3, cwnd_
   must have been inflated.  I am not sure if there are other loopholes.
   In fact, RenoTcpAgent does this in similar cases (tcp-reno.cc, line 88,
   whether dupack_action() decides to "slow down" or not).  I've tried to
   fix it this way but haven't come up with a "clean" code that I'm happy
   with.

3. Another curious problem is the test "if(dupacks_ > NUMDUPACKS &&  cwnd_
   > v_newcwnd_)" itself (line 136).  I think it should be "if(dupacks_ >=
   NUMDUPACKS &&  cwnd_ > v_newcwnd_)".  In "normal" cases (i.e., the
   problems I mentioned above *not included*), dupacks_ == 3 means cwnd_
   has been inflated, and I don't see why it shouldn't be restored to its
   pre-inflation value in this particular case.

A quick look at Brakmo's TCP Vegas implementation (on x-kernel;
ftp://ftp.cs.arizona.edu/xkernel/new-protocols/Vegas.tar.Z) shows that it
has the same problems of lingering "v_worried_" (1. above) and not
"deflating" cwnd_ in some cases (3. above), so I'm not sure if they are
bugs or features of TCP Vegas.  The x-kernel implementation may not have
the "drop dead" syndrome because it doesn't set v_newcwnd_ to 0 when a
timeout occurs.  However, it may still set a "wrong" cwnd_ value due to
the "v_worried_" problem.

In any case, I think we can't accept the "drop dead" phenomenon as a
feature of VegasTcpAgent.  If anyone knows of a newer TCP Vegas
implementation (than the x-kernel version mentioned above) that has
addressed these three problems, please let us (me and other interested
people) know.  Thanks a lot!

========================================
Kuang-Yeh Wang		kwang@cs.umd.edu
University of Maryland at College Park
Department of Computer Science
========================================

----------------------------------------------------------------------

Date: Thu, 8 Apr 1999 13:39:11 -0400 (EDT)
From: Kuang-Yeh Wang <kwang@cs.umd.edu>
To: John Heidemann <johnh@ISI.EDU>
Subject: Re: bug report: VegasTcpAgent 
In-Reply-To: <199904080325.UAA22327@dash.usc.edu>
Message-ID: <Pine.OSF.3.95.990408122429.1221i-400000@congo.cs.umd.edu>
MIME-Version: 1.0
Content-Type: MULTIPART/MIXED; BOUNDARY="0-1486779282-923593151=:1221"

...

... I found another potential bug in the current
VegasTcpAgent:
4. When it resets the rtx timer during a fast retransmission (due to
   duplicate ACKs and/or fine-grained timeout), it causes RTO backoff
   (line 332) and hurts performance.  It shouldn't behave this way (RTO
   backoff should happen only when a timeout happens), Reno doesn't, and
   I'm pretty sure Brakmo's Vegas doesn't either.  It could have been put
   in place inadvertently as the backoff flag in reset_rtx_timer() is on
   by default.

The attached "fixes" address all four problems.  I think nos. 2 and 4 are
not present in Brakmo's x-kernel implementation, so their fixes would be
activated *without* the flag "ns_vegas_fixes_" on.

Let me repeat my caveat: these "fixes" are based on my observations in
this and my previous e-mail messages, and both the fixes and the
observations may need closer scrutiny and debate.

Best regards,
Kuang-Yeh

*/
