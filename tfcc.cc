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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tfcc.cc,v 1.3 1998/09/12 02:56:36 kfall Exp $";
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
	int cong_seq_;		// congestion sequence number

	// per-var methods
	double& rttest() { return (rttest_); }
	double& ts() { return (ts_); }
	double& ts_echo() { return (ts_echo_); }
	double& interval() { return (interval_); }
	int& cseq() { return (cong_seq_); }

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
                field_offset("cong_seq_", OFFSET(hdr_tfcc, cong_seq_));
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
	last_rtime_(-1.0), last_ts_(-1.0), last_loss_time_(-1.0),
	expected_(-1), seqno_(0), cseq_(0), highest_cseq_seen_(-1),
	needresponse_(0), last_ecn_(0) {
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
	virtual void peer_rttest_known(double); // called when peer knows rtt
	virtual void rtt_known(double);	// called when we know rtt
	void ack_rate_change();		// changes the ack gen rate
	void slowdown();	// reason to slow down
	void speedup();		// possible opportunity to speed up
        double now() const { return Scheduler::instance().clock(); };
	double rtt_sample(double samp);

	double alpha_;		// weighting factor for rtt mean estimate
	double beta_;		// weighting factor for rtt var estimate

	double srtt_;
	double rttvar_;
	double peer_rtt_est_;	// peer's est of the rtt
	double last_ts_;	// ts field carries on last pkt
	double last_loss_time_;	// last time we saw a loss
	double ack_interval_;	// "ack" sending rate

	double last_rtime_;	// last time a pkt was received

	int expected_;		// next expected sequence number
	int seqno_;		// first seq# of a series of pkts
	int cseq_;		// congest epoch seq # (as receiver)
	int highest_cseq_seen_;	// peer's last cseq seen (I am sender)
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
	th->cseq() = cseq_;

	fh->ecnecho() = last_ecn_;
	fh->ect() = 1;

	RTPAgent::makepkt(p);
}

/*
 * recv-  packet contains receiver report info,
 * and may also contain data
 */

void
TFCCAgent::recv(Packet* pkt, Handler*)
{
        hdr_rtp* rh = (hdr_rtp*)pkt->access(off_rtp_);
	hdr_tfcc* th = (hdr_tfcc*)pkt->access(hdr_tfcc::offset_);
	hdr_flags* fh = (hdr_flags*)pkt->access(hdr_flags::offset_);

	/*
	 * do the duties of a receiver
	 */

	if (th->rttest() > 0.0) {
		// update our picture of our peer's rtt est
		if (peer_rtt_est_ <= 0.0) {
			peer_rttest_known(th->rttest());
		}
		peer_rtt_est_ = th->rttest();
	} else {
		// peer has no rtt estimate, so respond right
		// away so it can get one
		needresponse_ = 1;
	}

	if (rh->seqno() > seqno_) {
		// last seqno and ts we've seen
        	seqno_ = rh->seqno();
		last_ts_ = th->ts();
	}

	if (fh->ect()) {
		// turn around ecn's we see
		if (fh->ce()) {
			last_ecn_ = 1;
			ecn_event();
		} else if (fh->cong_action())
			last_ecn_ = 0;
	}

	int loss = 0;
	if (expected_ >= 0 && (loss = seqno_ - expected_) > 0)
		loss_event(loss);

	/*
	 * do the duties of a sender
	 */

	if (th->ts_echo() > 0.0) {
		// update our rtt estimate
		double sample = now() - th->ts_echo();
		if (srtt_ <= 0.0) {
			rtt_known(sample);
		}
		rtt_sample(sample);
	}

	if (th->cseq() > highest_cseq_seen_) {
		// look at receiver's congestion report
		highest_cseq_seen_ = th->cseq();
		slowdown();
	} else if (th->cseq() == highest_cseq_seen_) {
		speedup();
	} else {
		fprintf(stderr, "%s: oops! decreasing cseqs!\n",
			name());
		abort();
	}

        last_rtime_ = now();
        expected_ = seqno_ + 1;
        Packet::free(pkt);

	if (needresponse_)
		sendpkt();
	needresponse_ = 0;
}

/*
 * as a receiver, this
 * is called when there is a packet loss detected
 */

void
TFCCAgent::loss_event(int nlost)
{
	// if its been awhile (more than an rtt estimate) since the last loss,
	// this is a new indication of congestion
	if (peer_rtt_est_ < 0.0 || (now() - last_loss_time_) > peer_rtt_est_) {
		++cseq_;
		needresponse_ = 1;
	}
	last_loss_time_ = now();
}

/*
 * as a receiver, this is called when a packet arrives with a CE
 * bit set
 */
void
TFCCAgent::ecn_event()
{
	loss_event(0);	// for now
}

/*
 * as a receiver, this is called once when the peer first known
 * the rtt
 */
void
TFCCAgent::peer_rttest_known(double peerest)
{
	// first time our peer has indicated it knows the rtt
	// so, adjust our "ack" sending rate to be once per rtt
printf("%s: setting ack_interval to %f\n", name(), peerest);
	ack_interval_ = peerest;
	ack_rate_change();
}

/*
 * as a sender, this is called when we first know the rtt
 */
void
TFCCAgent::rtt_known(double rtt)
{
}

/*
 * as a sender, this is called when we are receiving acks with no
 * new congestion indications
 */

void
TFCCAgent::speedup()
{
printf("%s SPEEDUP\n", name());
}

/*
 * as a sender, this is called when we are receiving acks with
 * new congestion indications
 */

void
TFCCAgent::slowdown()
{
printf("%s SLOWDOWN\n", name());
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
