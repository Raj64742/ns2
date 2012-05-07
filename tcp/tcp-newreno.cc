/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1990, 1997 Regents of the University of California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tcp/tcp-newreno.cc,v 1.59 2012/05/07 02:30:39 tom_henderson Exp $ (LBL)";
#endif

//
// newreno-tcp: a revised reno TCP source, without sack
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"


static class NewRenoTcpClass : public TclClass {
public:
	NewRenoTcpClass() : TclClass("Agent/TCP/Newreno") {}
	TclObject* create(int, const char*const*) {
		return (new NewRenoTcpAgent());
	}
} class_newreno;

NewRenoTcpAgent::NewRenoTcpAgent() : newreno_changes_(0), 
  newreno_changes1_(0), acked_(0), firstpartial_(0), 
  partial_window_deflation_(0), exit_recovery_fix_(0)
{
	bind("newreno_changes_", &newreno_changes_);
	bind("newreno_changes1_", &newreno_changes1_);
	bind("exit_recovery_fix_", &exit_recovery_fix_);
	bind("partial_window_deflation_", &partial_window_deflation_);
}

/* 
 * Process a packet that acks previously unacknowleges data, but
 * does not take us out of Fast Retransmit.
 */
void NewRenoTcpAgent::partialnewack(Packet* pkt)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	if (partial_window_deflation_) {
		// Do partial window deflation before resetting last_ack_
		unsigned int deflate = 0; // Should initialize it?? - haoboy
		if (tcph->seqno() > last_ack_) // assertion
			deflate = tcph->seqno() - last_ack_;
		else 
		  	printf("False call to partialnewack:  deflate %u \
last_ack_ %d\n", deflate, last_ack_);
		if (dupwnd_ > deflate)
			dupwnd_ -= (deflate - 1);
		else {
			cwnd_ -= (deflate - dupwnd_);
			// Leave dupwnd_ > 0 to flag "fast recovery" phase
			dupwnd_ = 1; 
		}
		if (cwnd_ < 1) {cwnd_ = 1;}
	}
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;
	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	if (rtt_active_ && tcph->seqno() >= rtt_seq_) {
		rtt_active_ = 0;
		t_backoff_ = 1;
	}
}

void NewRenoTcpAgent::partialnewack_helper(Packet* pkt)
{
	if (!newreno_changes1_ || firstpartial_ == 0) {
		firstpartial_ = 1;
		/* For newreno_changes1_, 
		 * only reset the retransmit timer for the first
		 * partial ACK, so that, in the worst case, we
		 * don't have to wait for one packet retransmitted
		 * per RTT.
		 */
		newtimer(pkt);
	}
	partialnewack(pkt);
	output(last_ack_ + 1, 0);
}

int
NewRenoTcpAgent::allow_fast_retransmit(int /* last_cwnd_action_*/)
{
	return 0;
}

void
NewRenoTcpAgent::dupack_action()
{
        int recovered = (highest_ack_ > recover_);
	int recovered1 = (highest_ack_ == recover_);
        int allowFastRetransmit = allow_fast_retransmit(last_cwnd_action_);
        if (recovered || (!bug_fix_ && !ecn_) || allowFastRetransmit 
    	      || (bugfix_ss_ && highest_ack_ == 0)) {
                // (highest_ack_ == 0) added to allow Fast Retransmit
                //  when the first data packet is dropped.
                //  Bug report from Mark Allman.
                goto reno_action;
        }
	if (bug_fix_ && less_careful_ && recovered1) {
		/*
		 * For the Less Careful variant, allow a Fast Retransmit
		 *  if highest_ack_ == recover.
		 * RFC 2582 recommends the Careful variant, not the 
		 *  Less Careful one.
		 */
                goto reno_action;
	}

        if (ecn_ && last_cwnd_action_ == CWND_ACTION_ECN) {
                last_cwnd_action_ = CWND_ACTION_DUPACK;
                /*
                 * What if there is a DUPACK action followed closely by ECN
                 * followed closely by a DUPACK action?
                 * The optimal thing to do would be to remember all
                 * congestion actions from the most recent window
                 * of data.  Otherwise "bugfix" might not prevent
                 * all unnecessary Fast Retransmits.
                 */
                reset_rtx_timer(1,0);
                output(last_ack_ + 1, TCP_REASON_DUPACK);
		dupwnd_ = numdupacks_;
                return;
        }

        if (bug_fix_) {
		if (bugfix_ts_ && tss[highest_ack_ % tss_size_] == ts_echo_)
			goto reno_action;
		else if (bugfix_ack_ && cwnd_ > 1 && highest_ack_ - prev_highest_ack_ <= numdupacks_)
			goto reno_action;
		else
                /*
                 * The line below, for "bug_fix_" true, avoids
                 * problems with multiple fast retransmits in one
                 * window of data.
                 */
                	return;
        }

reno_action:
        recover_ = maxseq_;
        reset_rtx_timer(1,0);
        if (!lossQuickStart()) {
                trace_event("NEWRENO_FAST_RETX");
        	last_cwnd_action_ = CWND_ACTION_DUPACK;
        	slowdown(CLOSE_SSTHRESH_HALF|CLOSE_CWND_HALF);
        	output(last_ack_ + 1, TCP_REASON_DUPACK);       // from top
		dupwnd_ = numdupacks_;
	}
        return;
}


void NewRenoTcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = hdr_tcp::access(pkt);
	int valid_ack = 0;

	/* Use first packet to calculate the RTT  --contributed by Allman */

        if (qs_approved_ == 1 && tcph->seqno() > last_ack_)
		endQuickStart();
        if (qs_requested_ == 1)
                processQuickStart(pkt);
	if (++acked_ == 1) 
		basertt_ = Scheduler::instance().clock() - firstsent_;

	/* Estimate ssthresh based on the calculated RTT and the estimated
	   bandwidth (using ACKs 2 and 3).  */

	else if (acked_ == 2)
		ack2_ = Scheduler::instance().clock();
	else if (acked_ == 3) {
		ack3_ = Scheduler::instance().clock();
		new_ssthresh_ = int((basertt_ * (size_ / (ack3_ - ack2_))) / size_);
		if (newreno_changes_ > 0 && new_ssthresh_ < ssthresh_)
			ssthresh_ = new_ssthresh_;
	}

#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		fprintf(stderr,
			"ns: confiuration error: tcp received non-ack\n");
		exit(1);
	}
#endif
        /* W.N.: check if this is from a previous incarnation */
        if (tcph->ts() < lastreset_) {
                // Remove packet and do nothing
                Packet::free(pkt);
                return;
        }
	++nackpack_;
	ts_peer_ = tcph->ts();

	if (hdr_flags::access(pkt)->ecnecho() && ecn_)
		ecn(tcph->seqno());
	recv_helper(pkt);
	recv_frto_helper(pkt);
	if (tcph->seqno() > last_ack_) {
		if (tcph->seqno() >= recover_ 
		    || (last_cwnd_action_ != CWND_ACTION_DUPACK)) {
			if (dupwnd_ > 0) {
			     dupwnd_ = 0;
			     if (last_cwnd_action_ == CWND_ACTION_DUPACK)
				last_cwnd_action_ = CWND_ACTION_EXITED;
			     if (exit_recovery_fix_) {
				int outstanding = maxseq_ - tcph->seqno() + 1;
				if (ssthresh_ < outstanding)
                                        cwnd_ = ssthresh_;
                                else
                                        cwnd_ = outstanding;
			    }
			}
			firstpartial_ = 0;
			recv_newack_helper(pkt);
			if (last_ack_ == 0 && delay_growth_) {
				cwnd_ = initial_window();
			}
		} else {
			/* received new ack for a packet sent during Fast
			 *  Recovery, but sender stays in Fast Recovery */
			if (partial_window_deflation_ == 0)
				dupwnd_ = 0;
			partialnewack_helper(pkt);
		}
	} else if (tcph->seqno() == last_ack_) {
		if (hdr_flags::access(pkt)->eln_ && eln_) {
			tcp_eln(pkt);
			return;
		}
		if (++dupacks_ == numdupacks_) {
			dupack_action();
                        if (!exitFastRetrans_)
                                dupwnd_ = numdupacks_;
		} else if (dupacks_ > numdupacks_ && (!exitFastRetrans_
		      || last_cwnd_action_ == CWND_ACTION_DUPACK)) {
			trace_event("NEWRENO_FAST_RECOVERY");
			++dupwnd_;	// fast recovery

			/* For every two duplicate ACKs we receive (in the
			 * "fast retransmit phase"), send one entirely new
			 * data packet "to keep the flywheel going".  --Allman
			 */
			if (newreno_changes_ > 0 && (dupacks_ % 2) == 1)
				output (t_seqno_++,0);
		} else if (dupacks_ < numdupacks_ && singledup_ ) {
                        send_one();
                }
	}
        if (tcph->seqno() >= last_ack_)
                // Check if ACK is valid.  Suggestion by Mark Allman.
                valid_ack = 1;
	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif

	/*
	 * Try to send more data
	 */

        if (valid_ack || aggressive_maxburst_)
	{
		if (dupacks_ == 0) 
		{
			/*
			 * Maxburst is really only needed for the first
			 *  window of data on exiting Fast Recovery.
			 */
			send_much(0, 0, maxburst_);
		}
		else if (dupacks_ > numdupacks_ - 1 && newreno_changes_ == 0)
		{
			send_much(0, 0, 2);
		}
	}
}

