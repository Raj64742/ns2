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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tfcc.cc,v 1.2 1998/09/12 02:08:47 kfall Exp $";
#endif

/* tfcc.cc -- TCP-friently congestion control protocol */


#include <stdlib.h>
#include <string.h>

#include "agent.h"
#include "random.h"
#include "rtp.h"
#include "flags.h"

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

class TFCCAgent;

class TFCCTimer : public TimerHandler {
public: 
        TFCCTimer(TFCCAgent *a) : TimerHandler() { a_ = a; }
protected:
        void expire(Event *e);
        TFCCAgent *a_;
};

class TFCCAgent : public RTPAgent {

	friend class TFCCTimer;

public:
	TFCCAgent() : ack_timer_(this),
	srtt_(-1.0), rttvar_(-1.0), peer_rtt_est_(-1.0),
	last_rtime_(-1.0), last_ts_(-1.0), expected_(-1), seqno_(0), ploss_(0),
	lastploss_(0), lastexpected_(0), needresponse_(0), last_ecn_(0) {
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);
		bind("srtt_", &srtt_);
		bind("rttvar_", &rttvar_);
		bind("peer_rtt_est_", &peer_rtt_est_);
		bind("ack_interval_", &ack_interval_);
	}
protected:
	virtual void makepkt(Packet*);
	virtual void recv(Packet*, Handler*);
	virtual void loss_event(int);	// called when a loss is detected
	virtual void ecn_event();	// called when seeing an ECN
	virtual void peer_rttest_known(); // called when peer knows rtt
	virtual void rtt_known();	// called when we know rtt
	void ack_rate_change();		// changes the ack gen rate
        double now() const { return Scheduler::instance().clock(); };
	double rtt_sample(double samp);

	double alpha_;		// weighting factor for rtt mean estimate
	double beta_;		// weighting factor for rtt var estimate

	double srtt_;
	double rttvar_;
	double peer_rtt_est_;	// peer's est of the rtt
	double last_ts_;	// ts field carries on last pkt
	double ack_interval_;	// "ack" sending rate

	double last_rtime_;	// last time a pkt was received

	int expected_;		// next expected sequence number
	int seqno_;		// first seq# of a series of pkts
	int ploss_;		// total # packets lost
	int lastploss_;		// last ploss_ reported
	int lastexpected_;	// last expected_ reported
	int needresponse_;	// send a packet in response to current one
	int last_ecn_;		// last recv had an ecn

	RTPTimer ack_timer_;	// periodic timer for ack generation
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
        if (srtt_ > 0.0) {
                double delta = m - srtt_;
		srtt_ += alpha_ * delta;
                if (delta < 0.0)
                        delta = -delta;
		rttvar_ += beta_ * (delta - rttvar_);
        } else {
printf("%s: %f: srtt initialized to %f\n",
name(), now(), m);
                srtt_ = m;
                rttvar_ = srtt_ / 2.0;
        }
        return srtt_;
}       

void
TFCCAgent::makepkt(Packet* p)
{
	hdr_tfcc* th = (hdr_tfcc*)p->access(hdr_tfcc::offset_);
	hdr_flags* fh = (hdr_flags*)p->access(hdr_flags::offset_);

	th->ts_echo() = last_ts_;
	th->ts() = now();
	th->interval() = interval_;
	th->rttest() = srtt_;
	th->plost() = ploss_;
	th->maxseq() = expected_;
	fh->ecnecho() = last_ecn_;
	fh->ect() = 1;
	double denom = expected_ - lastexpected_;
	if (denom)
		th->lossfrac() = (ploss_ - lastploss_) / denom;
	else
		th->lossfrac() = -1.0;

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
	hdr_flags* fh = (hdr_flags*)pkt->access(hdr_flags::offset_);

	Tcl& tcl = Tcl::instance();

	// update our picture of our peer's rtt est
	if (th->rttest() > 0.0) {
		if (peer_rtt_est_ <= 0.0) {
			peer_rttest_known();
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
			rtt_known();
		}
		rtt_sample(now() - th->ts_echo());
	}
	if (rh->seqno() > seqno_) {
        	seqno_ = rh->seqno();
		last_ts_ = th->ts();
	}
        /*
         * Check for lost packets or CE bits
         */

	int loss = 0;
	if (fh->ect()) {
		if (fh->ce())
			last_ecn_ = 1;
		else if (fh->cong_action())
			last_ecn_ = 0;
	}
	if (expected_ >= 0 && (loss = seqno_ - expected_) > 0) {
		ploss_ += loss;
		loss_event(loss);
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
TFCCAgent::loss_event(int nlost)
{
}

void
TFCCAgent::ecn_event()
{
}

void
TFCCAgent::peer_rttest_known()
{
	// first time our peer has indicated it knows the rtt
	// so, adjust our "ack" sending rate to be once per rtt
	ack_interval_ = peer_rtt_est_;
	ack_rate_change();
}

void
TFCCAgent::rtt_known()
{
}


void
TFCCAgent::ack_rate_change()
{   
        ack_timer_.force_cancel();
        double t = lastpkttime_ + ack_interval_;
        if (t > now())
                ack_timer_.resched(t - now());
        else {  
                sendpkt();
                ack_timer_.resched(ack_interval_); 
        }       
}   

void
TFCCTimer::expire(Event*)
{
	a_->sendpkt();
	resched(a_->ack_interval_);
}
