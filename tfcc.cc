/*
 * Copyright (c) 1998 Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tfcc.cc,v 1.1 1998/09/12 00:44:36 kfall Exp $";
#endif

/* tfcc.cc -- TCP-friently congestion control protocol */


#include <stdlib.h>
#include <string.h>

#include "agent.h"
#include "random.h"
#include "rtp.h"

struct hdr_tfcc {
	double rttest_;	// sender's rtt estimate
	double ts_;	// ts at sender
	double ts_echo_;	// echo'd ts (for rtt estimates)
	double interval_;	// sender's sending interval

	// these are based on rtp receiver reports
	int plost_;	// # packets lost
	int maxseq_;	// max seq # recvd
	float lossfrac_;// fraction of packets lost

	// per-var methods
	double& rttest() { return (rttest_); }
	double& ts() { return (ts_); }
	double& ts_echo() { return (ts_echo_); }
	double& interval() { return (interval_); }
	int& plost() { return (plost_); }
	int& maxseq() { return (maxseq_); }
	float& lossfrac() { return (lossfrac_); }

	static int offset_;	// offset of tfcc header
};

int hdr_tfcc::offset_;

static class TFCCHeaderClass : public PacketHeaderClass {
public:
        TFCCHeaderClass() : PacketHeaderClass("PacketHeader/TFCC",
                                            sizeof(hdr_tfcc)) {
                bind_offset(&hdr_tfcc::offset_);
        }
        void export_offsets() {
                field_offset("rttest_", OFFSET(hdr_tfcc, rttest_));
                field_offset("ts_", OFFSET(hdr_tfcc, ts_));
                field_offset("ts_echo_", OFFSET(hdr_tfcc, ts_echo_));
                field_offset("interval_", OFFSET(hdr_tfcc, interval_));
                field_offset("plost_", OFFSET(hdr_tfcc, plost_));
                field_offset("maxseq_", OFFSET(hdr_tfcc, maxseq_));
                field_offset("lossfrac_", OFFSET(hdr_tfcc, lossfrac_));
        }
} class_tfcchdr;

class TFCCAgent : public RTPAgent {
public:
	TFCCAgent() : srtt_(-1.0), rttvar_(-1.0), peer_rtt_est_(-1.0),
	last_rtime_(-1.0), expected_(-1), seqno_(0), ploss_(0),
	lastploss_(0), lastexpected_(0), needresponse_(0) {
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);
		bind("srtt_", &srtt_);
		bind("rttvar_", &rttvar_);
		bind("peer_rtt_est_", &peer_rtt_est_);
	}
protected:
	virtual void makepkt(Packet*);
	virtual void recv(Packet*, Handler*);
	virtual void loss_event();	// called when a loss is detected
        double now() const { return Scheduler::instance().clock(); };
	double rtt_sample(double samp);

	double alpha_;		// weighting factor for rtt mean estimate
	double beta_;		// weighting factor for rtt var estimate

	double srtt_;
	double rttvar_;
	double peer_rtt_est_;	// peer's est of the rtt

	double last_rtime_;	// last time a pkt was received

	int expected_;		// next expected sequence number
	int seqno_;		// first seq# of a series of pkts
	int ploss_;		// total # packets lost
	int lastploss_;		// last ploss_ reported
	int lastexpected_;	// last expected_ reported
	int needresponse_;	// send a packet in response to current one
};

static class TFCCAgentClass : public TclClass {
public:
	TFCCAgentClass() : TclClass("Agent/RTP/TFCC") {}
	TclObject* create(int, const char*const*) {
		return (new TFCCAgent());
	}
} class_tfcc_agent;

double
TFCCAgent::rtt_sample(double m)
{       
	// m is new measurement
        if (srtt_ != 0.0) {
                double delta = m - srtt_;
		srtt_ += alpha_ * delta;
                if (delta < 0.0)
                        delta = -delta;
		rttvar_ += beta_ * (delta - rttvar_);
        } else {
                srtt_ = m;
                rttvar_ = srtt_ / 2.0;
        }
        return srtt_;
}       

void
TFCCAgent::makepkt(Packet* p)
{
	hdr_tfcc* th = (hdr_tfcc*)p->access(hdr_tfcc::offset_);
	th->ts_echo() = th->ts();
	th->ts() = now();
	th->interval() = interval_;
	th->rttest() = srtt_;
	th->plost() = ploss_;
	th->maxseq() = expected_;
	th->lossfrac() = (ploss_ - lastploss_) / (expected_ - lastexpected_);

	lastploss_ = ploss_;
	lastexpected_ = expected_;

	RTPAgent::makepkt(p);
}

/*
 * recv-  packet contains receiver report info,
 * and may also contain data
 */

#define	MAX(a,b)	((a)>(b)?(a):(b))

void
TFCCAgent::recv(Packet* pkt, Handler*)
{
        hdr_rtp* rh = (hdr_rtp*)pkt->access(off_rtp_);
	hdr_tfcc* th = (hdr_tfcc*)pkt->access(hdr_tfcc::offset_);
	Tcl& tcl = Tcl::instance();

	// update our picture of our peer's rtt est
	if (th->rttest() > 0.0) {
		if (peer_rtt_est_ <= 0.0) {
			tcl.evalf("%s peer_rttest_known %f",
				name(), th->rttest());
		}
		peer_rtt_est_ = th->rttest();
	} else {
		// peer has no rtt estimate, so respond right
		// away so it can get on
		needresponse_ = 1;
	}

	// call tcl once we get our first rtt estimate
	if (th->ts_echo() > 0.0) {
		double sample = now() - th->ts_echo();
		if (srtt_ <= 0.0) {
			tcl.evalf("%s rtt_known %f",
				name(), sample);
		}
		rtt_sample(now() - th->ts_echo());
	}
        seqno_ = MAX(rh->seqno(), seqno_);
        /*
         * Check for lost packets
         */
	int loss = 0;
        if (expected_ >= 0 && (loss = seqno_ - expected_) > 0) {
		ploss_ += loss;
		loss_event();
        }
        last_rtime_ = now();
        expected_ = seqno_ + 1;
        Packet::free(pkt);

	if (needresponse_)
		sendpkt();
	needresponse_ = 0;
}

/*
 * this is called when there is a packet loss detected
 */

void
TFCCAgent::loss_event()
{
        // if its been at least an rtt since we've sent something,
        // send it now. lastpkttime_ is the last time we sent something
	// and is updated in rtp.cc
        if ((now() - lastpkttime_) > peer_rtt_est_)
		needresponse_ = 1;
}
