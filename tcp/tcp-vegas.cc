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
 */

#ifndef lint
static char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-vegas.cc,v 1.3 1997/06/19 19:20:05 heideman Exp $ (NCSU/IBM)";
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
	TclObject* create(int argc, const char*const* argv) {
		return (new VegasTcpAgent());
	}
} class_vegas;


VegasTcpAgent::VegasTcpAgent() : TcpAgent()
{
	/* init vegas var */
	bind("v_alpha_", &v_alpha_);
	bind("v_beta_", &v_beta_);
	bind("v_gamma_", &v_gamma_);
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
}

/*
 * xxx: johnh: it's not clear that this is the right def
 * but it's what was done in ns-1
 *
 * Also, this function should just appear by inheritance,
 * but if it's not defined gcc-2.7.2.1 gives me errors at link time:
 * /home/johnh/WORKING/VINT/ns-2/tcp-vegas.cc:54: undefined reference to `VegasTcpAgent::Handler virtual table'
 * /home/johnh/WORKING/VINT/ns-2/tcp-vegas.cc:54: undefined reference to `VegasTcpAgent::TcpAgent virtual table'
 * ...
 */
int
VegasTcpAgent::window()
{
	return TcpAgent::window();
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

	if(firstrecv_<0) { // init vegas rtt vars
		firstrecv_ = currentTime;
		v_baseRTT_ = v_rtt_ = firstrecv_ - firstsent_;
		v_sa_  = v_rtt_ * 8.;
		v_sd_  = v_rtt_;
		v_timeout_ = ((v_sa_/4.)+v_sd_)/2.;
	}

	if (flagh->ecn_)
		quench(1);
	if (tcph->seqno() > last_ack_) {
		/* check if cwnd has been inflated */
		if(dupacks() > NUMDUPACKS &&  cwnd() > v_newcwnd_) {
			cwnd() = v_newcwnd_;
			// vegas ssthresh is used only during slow-start
			ssthresh() = 2;
		}
		int oldack = last_ack_;
		newack(pkt);
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
			int rttLen = t_seqno() - v_begseq_;

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
				expect = double(t_seqno()-last_ack_)/v_baseRTT_;

				// calc actual and expect thruput diff, delta
				int delta=int((expect-v_actual_)*v_baseRTT_+0.5);
				if(cwnd() < ssthresh()) { // slow-start
					// adj cwnd every other rtt
					v_inc_flag_ = !v_inc_flag_;
					if(!v_inc_flag_)
						v_incr_ = 0;
					else {
					    if(delta > v_gamma_) {
						// slow-down a bit to ensure
						// the net is not so congested
						ssthresh() = 2;
						cwnd()-=(cwnd()/8);
						if(cwnd()<2)
							cwnd() = 2.;
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
						--cwnd();
						if(cwnd()<2) cwnd() = 2;
						v_incr_ = 0;
					} else if(delta<v_alpha_)
						// delta<alpha, faster....
						v_incr_ = 1/cwnd_;
					else // current rate is cool.
						v_incr_ = 0;
				}
			} // end of if(rtt > 0)

			// tag the next packet 
			v_begseq_ = t_seqno(); 
			v_begtime_ = currentTime;
		} // end of once per-rtt section

		/* since we set how much to incr only once per rtt,
		 * need to check if we surpass ssthresh during slow-start
		 * before the rtt is over.
		 */		
		if(v_incr_ == 1 && cwnd() >= ssthresh())
			v_incr_ = 0;
		
		/*
		 * incr cwnd unless we havent been able to keep up with it
		 */
		if(v_incr_>0 && (cwnd()-(t_seqno()-last_ack_))<=2)
			cwnd() = cwnd()+v_incr_;	

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
				dupacks() = NUMDUPACKS;
				output(expired, TCP_REASON_DUPACK);
			} else
				v_worried_ = 0;
		}
   	} else if (tcph->seqno() == last_ack_)  {
		/* check if a timeout should happen */
		++dupacks(); 
		int expired=vegas_expire(pkt);
		if (expired>=0 || dupacks() == NUMDUPACKS) {
			double sendTime=v_sendtime_[(last_ack_+1) % v_maxwnd_]; 
			int transmits=v_transmits_[(last_ack_+1) % v_maxwnd_];
       	                /* The line below, for "bug_fix_" true, avoids
                        * problems with multiple fast retransmits after
			* a retransmit timeout.
                        */
			if ( !bug_fix_ || (highest_ack() > recover_) || \
			    ( recover_cause_ != 2)) {
				int win = window();
				recover_cause_ = 1;
				recover_ = maxseq();
				/* check for timeout after recv a new ack */
				v_worried_ = MIN(2, t_seqno() - last_ack_ );
		
				/* v_rto expon. backoff */
				if(transmits > 1) 
					v_timeout_ *=2.; 
				else
					v_timeout_ += (v_timeout_/8.);
				/*
				 * if cwnd hasnt changed since the pkt was sent
				 * we need to decr it.
				 */
				if(t_cwnd_changed_ < sendTime ) {
					if(win<=3)
						win=2;
					else if(transmits > 1)
						win >>=1;
					else 
						win -= (win>>2);

					// record cwnd_
					v_newcwnd_ = double(win);
					// inflate cwnd_
					cwnd() = v_newcwnd_ + dupacks();
					t_cwnd_changed_ = currentTime;
				} 

				// update coarser grained rto
				reset_rtx_timer(1);
				if(expired>=0) 
					output(expired, TCP_REASON_DUPACK);
				else
					output(last_ack_ + 1, TCP_REASON_DUPACK);
					 
				if(transmits==1) 
					dupacks() = NUMDUPACKS;
                        }
		} else if (dupacks() > NUMDUPACKS) 
			++cwnd();
	}
	Packet::free(pkt);
#if 0
	if (trace_)
		plot();
#endif /* 0 */

	/*
	 * Try to send more data
	 */
	if (dupacks() == 0 || dupacks() > NUMDUPACKS - 1)
		send_much(0, 0, maxburst_);
}

void
VegasTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupacks_ = 0;
		recover_ = maxseq_;
		recover_cause_ = 2;
		reset_rtx_timer(0);
		closecwnd(0); 
		cwnd_ = double(v_slowstart_);
		v_newcwnd_ = 0;
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

	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_)
				rtt_seq_ = seqno;
		}
	}
	if (!pending_[TCP_TIMER_RTX])
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

