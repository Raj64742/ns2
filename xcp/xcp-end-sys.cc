/*   Author: Dina Katabi 
     Date  : Jan 2002
*/

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcp-end-sys.cc,v 1.1 2004/04/20 16:09:01 haldar Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#include "agent.h"
#include "packet.h"

#include "flags.h"
#include "tcp-sink.h"
#include "xcp-end-sys.h"


#define TRACE 0 // when 0, we don't print any debugging info.

/**********************************************************
 *
 *             cwndShringTimer
 *
 **********************************************************/

void cwndShrinkingTimer::expire(Event*)
{
	
	a_->timeout(TCP_TIMER_DELSND);
}



/*************************************************************
 *
 *                    CC1 TCP
 *
 * - Any packet that it sends has its cwnd and rtt -> output is
 *  modified to do so
 * - If explicit feedback is provided then it uses it 
 * 
 */

int hdr_cctcp::offset_;
static unsigned int next_cctcp = 0;

static class CCTCPHeaderClass : public PacketHeaderClass {
public:
        CCTCPHeaderClass() : PacketHeaderClass("PacketHeader/CC1",
					     sizeof(hdr_cctcp)) {
		bind_offset(&hdr_cctcp::offset_);
	}
} class_cctcphdr;

static class CC1TcpClass : public TclClass {
public:
	CC1TcpClass() : TclClass("Agent/TCP/XCP") {}
	TclObject* create(int, const char*const*) {
		return (new CC1TcpAgent());
	}
} class_cc1;

CC1TcpAgent::CC1TcpAgent(): RenoTcpAgent(), shrink_cwnd_timer_(this)
{
	tcpId_   = next_cctcp;
	next_cctcp++;
	init_rtt_vars();
	price_   = 1;
	profile_ = 0;
	type_ = PT_XCP;
}

//-- output is the standard tcp function except that it fills in the 
// ccenabled_, cwnd_, rtt_, positive_feedback_ variables in the cctcp header
void CC1TcpAgent::output(int seqno, int reason)
{
	int force_set_rtx_timer = 0;
	Packet* p = allocpkt();
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_flags* hf = hdr_flags::access(p);
	tcph->seqno() = seqno;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;
	if (ecn_) {
		hf->ect() = 1;	// ECN-capable transport
	}
	if (cong_action_) {
		hf->cong_action() = TRUE;  // Congestion action.
		cong_action_ = FALSE;
        }

	// Beginning of Dina's Changes
	hdr_cctcp *cctcph                   = hdr_cctcp::access(p);
	cctcph->ccenabled_                  = 1;
	cctcph->cwnd_                       = double(cwnd_/price_);
	cctcph->rtt_                        = srtt_estimate_;
	cctcph->cctcpId_                    = tcpId_;
	cctcph->was_restricted_             = I_was_restricted_;
	cctcph->price_                      = price_;
	cctcph->profile_                    = profile_;
	// Sender bids for a positive feedback only if it will arrive before its transfer ends
	if (int(seqno)+int(cwnd_) < int(curseq_)){
		cctcph->positive_feedback_  = -1; 
		cctcph->restricted_         = 0;
		cctcph->restricted_by_      = -2;
	} else {
		cctcph->positive_feedback_  = 0;
		cctcph->restricted_         = 1;
		cctcph->restricted_by_      = -1;
	} 
	cctcph->shuffling_feedback_         = -1;  // No shuffling
	cctcph->negative_feedback_          = -1;  // negative feedback

	// this is helpful in computing the throughput at the end
	//if ( seqno == 0) {begin_transfer_time_ = Scheduler::instance().clock();}
	//printf(" %d: %g  OUTPUT Function SRTT %g \n",tcpId_, time_now(), srtt_estimate_);
	
	//printf("%d: srtt_estimate  %g \n", tcpId_, srtt_estimate_);
	if(channel_){
		trace_var("throughput", double(cwnd_)/srtt_estimate_);
	}
	// End of Dina's Changes

	/* Check if this is the initial SYN packet. */
	if (seqno == 0) {
		if (syn_) {
			hdr_cmn::access(p)->size() = tcpip_base_hdr_size_;
		}
		if (ecn_) {
			hf->ecnecho() = 1;
//			hf->cong_action() = 1;
			hf->ect() = 0;
		}
	}
        int bytes = hdr_cmn::access(p)->size();

	/* if no outstanding data, be sure to set rtx timer again */
	if (highest_ack_ == maxseq_)
		force_set_rtx_timer = 1;
	/* call helper function to fill in additional fields */
	output_helper(p);

        ++ndatapack_;
        ndatabytes_ += bytes;
	send(p, 0);
	if (seqno == curseq_ && seqno > maxseq_)
		idle();  // Tell application I have sent everything so far
	if (seqno > maxseq_) {
		maxseq_ = seqno;
		if (!rtt_active_) {
			rtt_active_ = 1;
			if (seqno > rtt_seq_) {
				rtt_seq_ = seqno;
				rtt_ts_ = Scheduler::instance().clock();
			}
					
		}
	} else {
        	++nrexmitpack_;
        	nrexmitbytes_ += bytes;
	}
	if (!(rtx_timer_.status() == TIMER_PENDING) || force_set_rtx_timer)
		/* No timer pending.  Schedule one. */
		set_rtx_timer();
}

/*----- opencwnd 
 *
 * Option  2 lets TCP open its window 
 * by the amount indicated by the router
 * Which option to use depends on the header of 
 * received ack and is figured out in recv_newack_helper
 * 
 */
void CC1TcpAgent::opencwnd()
{
	double next_cwnd;
	//	if (cwnd_ < ssthresh_) {
	//	/* slow-start (exponential) */
	//	cwnd_ += 1;
	//} else {
		/* linear */
	//printf(" opencwnd option is %d \n", wnd_option_);
		switch (wnd_option_) {
		case 0:
			if (++count_ >= cwnd_) {
				count_ = 0;
				++cwnd_;
			}
			printf("Error: in TCP Why do I have wnd_option_ = 0\n");
			abort();
			break;

		case 1:
			/* This is the standard algorithm. */
			cwnd_ += 1 / cwnd_;
			break;

		case 2:
			//----- PCP option 
			if(TRACE){
				printf("%d: %g: Explicit Increase option: feedback %g, cwnd %g \n",tcpId_, 
				       time_now(), current_positive_feedback_, double(((TracedDouble) cwnd_)));
			}
			next_cwnd = cwnd_+ current_positive_feedback_;
			if (next_cwnd < 1) {next_cwnd = 1;}
			cwnd_ = (next_cwnd < wnd_ ? next_cwnd : wnd_);
                        break;

		default:
#ifdef notdef
			/*XXX*/
			error("illegal window option %d", wnd_option_);
#endif
			abort();
		}
		//}
	// if maxcwnd_ is set (nonzero), make it the cwnd limit
	if (maxcwnd_ && (int(cwnd_) > maxcwnd_))
		cwnd_ = maxcwnd_;

	return;
}

void CC1TcpAgent::recv_newack_helper(Packet *pkt) {
	//hdr_tcp *tcph = hdr_tcp::access(pkt);
	newack(pkt);
	// Dina's changes
	hdr_cctcp *cctcph = hdr_cctcp::access(pkt);
	if(cctcph->ccenabled_ != 10){
		printf("Dina-tcp-new.cc:213, Error received a non-cc-ack \n");
		abort();
	} 
	if(channel_){
		trace_var("positive_feedback_", cctcph-> positive_feedback_);
		trace_var("negative_feedback_", cctcph-> negative_feedback_);
		trace_var("controlling_hop_", cctcph-> controlling_hop_);
		trace_var("restricted_by_", cctcph->restricted_by_);
	}
	if (cctcph->positive_feedback_ != -1 ){
		// If a router sent some info
		wnd_option_ = 2; // the CC1 option
		current_positive_feedback_ = cctcph-> positive_feedback_;
	} else {
		wnd_option_ = 1;
		printf("CC1TcpAgent::recv_newack_helper, and feedback is -1\n");
	}
	if (cctcph->negative_feedback_ != -1 ){
		double old_cwnd = cwnd_;
		cwnd_ = cwnd_ - cctcph->negative_feedback_;
		if (cwnd_ < 1) cwnd_ = 1;
			printf("%d:  DECREASE old_cwnd %g, cwnd %g\n", 
			       tcpId_, old_cwnd, double(cwnd_));
	}
	if (cctcph->restricted_ == 0 || cctcph->restricted_ == 1){
		I_was_restricted_ = cctcph->restricted_;
	} else { 
		printf("TCP%d  %g  Error in Dina-tcp.cc bad restriction %d \n", fid_, time_now(),cctcph->restricted_);
		abort();
	}
	// End of Dina's changes


	// code below is old TCP
	//if (!ect_ || !hdr_flags::access(pkt)->ecnecho() ||
	//(old_ecn_ && ecn_burst_)) 
	/* If "old_ecn", this is not the first ACK carrying ECN-Echo
	 * after a period of ACKs without ECN-Echo.
	 * Therefore, open the congestion window. */
	//opencwnd();
	//if (ect_) {
	//if (!hdr_flags::access(pkt)->ecnecho())
	//	ecn_backoff_ = 0;
	//if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
	//	ecn_burst_ = TRUE;
	//else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
	//	ecn_burst_ = FALSE;
	//}
	//if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
	//!hdr_flags::access(pkt)->cong_action())
	//ect_ = 1;
	/* if the connection is done, call finish() */
	//if ((highest_ack_ >= curseq_-1) && !closed_) {
	//	closed_ = 1;
	//finish();
	//}

	// Code below is from the ns2.8 tcp.cc

	if (!ect_ || !hdr_flags::access(pkt)->ecnecho() ||
	    (old_ecn_ && ecn_burst_)) {
		/* If "old_ecn", this is not the first ACK carrying ECN-Echo
		 * after a period of ACKs without ECN-Echo.
		 * Therefore, open the congestion window. */
		/* if control option is set, and the sender is not
		   window limited, then do not increase the window size */
		
		if (!control_increase_ || 
		    (control_increase_ && (network_limited() == 1))) 
	      		opencwnd();
	}
	if (ect_) {
		if (!hdr_flags::access(pkt)->ecnecho())
			ecn_backoff_ = 0;
		if (!ecn_burst_ && hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = TRUE;
		else if (ecn_burst_ && ! hdr_flags::access(pkt)->ecnecho())
			ecn_burst_ = FALSE;
	}
	if (!ect_ && hdr_flags::access(pkt)->ecnecho() &&
		!hdr_flags::access(pkt)->cong_action())
		ect_ = 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
	if (QOption_ && curseq_ == highest_ack_ +1) {
		cancel_rtx_timer();
	}
}

void CC1TcpAgent::rtt_update(double tao)
{
	double now = Scheduler::instance().clock();
	// Dina's changes
	// rtt estimate in tcp is taylored for computation of rto
	// which is not what we want. Thus we keep our own estimate of rtt
	//printf("rtt_update %g\n\n", tao);
	rtt_estimate_ = tao;
	if (flag_first_ack_received_){
		srtt_estimate_ = 0.6 * srtt_estimate_ + 0.4 * rtt_estimate_;
	} else {
		srtt_estimate_ = rtt_estimate_;
		flag_first_ack_received_ = 1;
	}
	if (TRACE) {printf("%d:  %g  SRTT %g, RTT %g \n", tcpId_, now, srtt_estimate_, rtt_estimate_);}
	// End of Dina's Changes
	if (ts_option_)
		t_rtt_ = int(tao /tcp_tick_ + 0.5);
	else {
		double sendtime = now - tao;
		sendtime += boot_time_;
		double tickoff = fmod(sendtime, tcp_tick_);
		t_rtt_ = int((tao + tickoff) / tcp_tick_);
	}
	if (t_rtt_ < 1)
		t_rtt_ = 1;

	//
	// srtt has 3 bits to the right of the binary point
	// rttvar has 2
	//
        if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);	// d = (m - a0)
		if ((t_srtt_ += delta) <= 0)	// a1 = 7/8 a0 + 1/8 m
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (t_rttvar_ >> T_RTTVAR_BITS);
		if ((t_rttvar_ += delta) <= 0)	// var1 = 3/4 var0 + 1/4 |d|
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		// srtt = rtt
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	// rttvar = rtt / 2
	}
	//
	// Current retransmit value is 
	//    (unscaled) smoothed round trip estimate
	//    plus 2^rttvar_exp_ times (unscaled) rttvar. 
	//
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) +
		t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;

	return;
}

void CC1TcpAgent::rtt_init()
{
	t_rtt_ = 0;
	t_srtt_ = int(srtt_init_ / tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_ / tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	t_backoff_ = 1;

	// Dina's Change 
	init_rtt_vars();
	rtt_active_ = 0;
	rtt_seq_ = -1;
	// End of Dina's Changes
}

void CC1TcpAgent::trace_var(char * var_name, double var)
{
  char wrk[500];
  if (channel_) {
    int n;
    sprintf(wrk, "%g x x x x %s %g",time_now(),var_name, var);
    n = strlen(wrk);
    wrk[n] = '\n'; 
    wrk[n+1] = 0;
    (void)Tcl_Write(channel_, wrk, n+1);
  }
  return; 
}

int CC1TcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {		
	  if (strcmp(argv[1], "set-price") == 0) {
	    price_ = double(atof(argv[2]));
	    printf("%d:  price_ = %g \n", tcpId_, price_);
	    if (price_ < 0.0) {
	      printf("%d:  Error price_ = %g < 0 \n", tcpId_, price_);
	      abort();
	    }
	    return (TCL_OK);
	  }
	  else if (strcmp(argv[1], "set-profile") == 0) {
	    profile_ = double(atof(argv[2]));
	    printf("%d:  profile_ = %g \n", tcpId_, profile_);
	    if (profile_ < 0.0) {
	      printf("%d:  Error profile_ = %g < 0 \n", tcpId_, profile_);
	      abort();
	    }
	    return (TCL_OK);
	  }
	}
	return RenoTcpAgent::command(argc, argv);
}
/*************************************************************
 *
 *                    XCPSink
 *
 * The code is very similar to TcpSink. However, I defined a new 
 * class so that i don't change the tcp-sink.cc file.  
 */


class CCTcpSink : public Agent {
public:
	CCTcpSink(Acker*);
	void recv(Packet* pkt, Handler*);
	void reset();
	int command(int argc, const char*const* argv);
	TracedInt& maxsackblocks() { return max_sack_blocks_; }
protected:
	void ack(Packet*);
	virtual void add_to_ack(Packet* pkt);
	Acker* acker_;
	int ts_echo_bugfix_;

	friend void Sacker::configure(TcpSink*);
	TracedInt max_sack_blocks_;	/* used only by sack sinks */
	Packet* save_;		/* place to stash saved packet while delaying */
				/* used by DelAckSink */
};
static class CCTcpSinkClass : public TclClass {
public:
	CCTcpSinkClass() : TclClass("Agent/XCPSink") {}
	TclObject* create(int, const char*const*) {
		return (new CCTcpSink(new Acker));
	}
} class_cctcpsink;
 

CCTcpSink::CCTcpSink(Acker* acker) : Agent(PT_ACK), acker_(acker)
{
  //bind("packetSize_", &size_);
  //bind("maxSackBlocks_", &max_sack_blocks_); // used only by sack
  //bind_bool("ts_echo_bugfix_", &ts_echo_bugfix_);
}

int CCTcpSink::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void CCTcpSink::reset() 
{
	acker_->reset();	
	save_ = NULL;
}

void CCTcpSink::ack(Packet* opkt)
{
	Packet* npkt = allocpkt();
	double now = Scheduler::instance().clock();
	hdr_flags *sf;

	hdr_tcp *otcp = hdr_tcp::access(opkt);
	hdr_tcp *ntcp = hdr_tcp::access(npkt);
	ntcp->seqno() = acker_->Seqno();
	ntcp->ts() = now;

	if (ts_echo_bugfix_)  /* TCP/IP Illustrated, Vol. 2, pg. 870 */
		ntcp->ts_echo() = acker_->ts_to_echo();
	else
		ntcp->ts_echo() = otcp->ts();

	hdr_ip* oip = HDR_IP(opkt);
	hdr_ip* nip = HDR_IP(npkt);
	nip->flowid() = oip->flowid();

	hdr_flags* of = hdr_flags::access(opkt);
	hdr_flags* nf = hdr_flags::access(npkt);
	if (save_ != NULL) 
		sf = hdr_flags::access(save_);
		// Look at delayed packet being acked. 
	if ( (save_ != NULL && sf->cong_action()) || of->cong_action() ) 
		// Sender has responsed to congestion. 
		acker_->update_ecn_unacked(0);
	if ( (save_ != NULL && sf->ect() && sf->ce())  || 
			(of->ect() && of->ce()) )
		// New report of congestion.  
		acker_->update_ecn_unacked(1);
	if ( (save_ != NULL && sf->ect()) || of->ect() )
		// Set EcnEcho bit.  
		nf->ecnecho() = acker_->ecn_unacked();
	if (!of->ect() && of->ecnecho() ||
		(save_ != NULL && !sf->ect() && sf->ecnecho()) ) 
		 // This is the negotiation for ECN-capability.
		 // We are not checking for of->cong_action() also. 
		 // In this respect, this does not conform to the 
		 // specifications in the internet draft 
		nf->ecnecho() = 1;

	/* Dina's Changes
	 * the below code relay the new congestion feedback 
	 * to the sender
	 */
	int off_cctcp_ = hdr_cctcp::offset();
	hdr_cctcp* occtcp = (hdr_cctcp*)opkt->access(off_cctcp_);
	hdr_cctcp* ncctcp = (hdr_cctcp*)npkt->access(off_cctcp_);
	ncctcp->ccenabled_ = 10; // a special value to indicate that it is a cc ack
       	ncctcp->positive_feedback_  = occtcp->positive_feedback_;
	ncctcp->negative_feedback_  = occtcp->negative_feedback_;
	ncctcp->shuffling_feedback_ = occtcp->shuffling_feedback_;
	ncctcp->controlling_hop_    = occtcp->controlling_hop_;
	ncctcp->cwnd_               = occtcp->cwnd_;
	ncctcp->rtt_                = occtcp->rtt_;
	ncctcp->cctcpId_            = occtcp->cctcpId_;
	ncctcp->restricted_         = occtcp->restricted_;
	ncctcp->restricted_by_      = occtcp->restricted_by_;
	ncctcp->was_restricted_     = occtcp->was_restricted_;
	ncctcp->price_              = occtcp->price_;
	ncctcp->profile_            = occtcp->profile_;
	// End of Dina's Changes

	acker_->append_ack(HDR_CMN(npkt), ntcp, otcp->seqno());
	add_to_ack(npkt);
	send(npkt, 0);
}

void CCTcpSink::add_to_ack(Packet*)
{
	return;
}

void CCTcpSink::recv(Packet* pkt, Handler*)
{
	int numToDeliver;
	int numBytes = HDR_CMN(pkt)->size();
	hdr_tcp *th = hdr_tcp::access(pkt);
	acker_->update_ts(th->seqno(),th->ts());
      	numToDeliver = acker_->update(th->seqno(), numBytes);
	if (numToDeliver)
		recvBytes(numToDeliver);
      	ack(pkt);
	Packet::free(pkt);
}
