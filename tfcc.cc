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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tfcc.cc,v 1.7 1998/09/15 02:25:47 kfall Exp $";
#endif

/* tfcc.cc -- TCP-friently congestion control protocol */


#include <stdlib.h>
#include <string.h>

#include "agent.h"
#include "random.h"
#include "rtp.h"
#include "flags.h"

/*
 * TFCC HEADER DEFINITIONS
 */

struct hdr_tfcc {
	double rttest_;	// sender's rtt estimate
	double ts_;	// ts at sender
	double ts_echo_;	// echo'd ts (for rtt estimates)
	double interval_;	// sender's sending interval
	int cong_seq_;		// congestion sequence number
	int nlost_;		// total packets lost

	// per-var methods
	double& rttest() { return (rttest_); }
	double& ts() { return (ts_); }
	double& ts_echo() { return (ts_echo_); }
	int& cseq() { return (cong_seq_); }
	int& nlost() { return (nlost_); }

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

/**************************** class defn ********************************/

class TFCCAgent : public RTPAgent {
public:
	TFCCAgent() : srtt_(-1.0), rttvar_(-1.0), peer_rtt_est_(-1.0),
	last_ts_(-1.0), last_loss_time_(-1.0), last_rtime_(-1.0),
	expected_(0), nlost_(0), plost_(0), cseq_(0), highest_cseq_seen_(-1),
	last_cseq_checked_(-1), needresponse_(0), last_ecn_(0),
	ack_timer_(this), rtt_timer_(this) {
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);
		bind("srtt_", &srtt_);
		bind("minrtt_", &minrtt_);
		bind("maxrtt_", &maxrtt_);
		bind("rttvar_", &rttvar_);
		bind("peer_rtt_est_", &peer_rtt_est_);
		bind("ack_interval_", &ack_interval_);
		bind("silence_thresh_", &silence_thresh_);
	}
protected:
	virtual void makepkt(Packet*);
	virtual void recv(Packet*, Handler*);
	virtual void loss_event(int);	// called when a loss is detected
	virtual void ecn_event();	// called when seeing an ECN
	virtual void peer_rttest_known(double); // called when peer knows rtt
	virtual void rtt_known(double);	// called when we know rtt
	void ack_rate_change();		// changes the ack gen rate
	virtual void slowdown();	// reason to slow down
	virtual void speedup();		// possible opportunity to speed up
	virtual void nopeer();		// lost ack stream from peer

	virtual void rtt_timeout(TimerHandler*);	// rtt heartbeat
	virtual void ack_timeout(TimerHandler*);	// ack sender

        double now() const { return Scheduler::instance().clock(); };
	double rtt_sample(double samp);

	double alpha_;		// weighting factor for rtt mean estimate
	double beta_;		// weighting factor for rtt var estimate

	double srtt_;
	double rttvar_;
	double maxrtt_;		// max rtt seen
	double minrtt_;		// min rtt seen
	double peer_rtt_est_;	// peer's est of the rtt
	double last_ts_;	// ts field carries on last pkt
	double last_loss_time_;	// last time we saw a loss
	double ack_interval_;	// "ack" sending rate

	double last_rtime_;	// last time a pkt was received

	int expected_;		// next expected sequence number
	int nlost_;		// # pkts lost (cumulative)
	int plost_;		// # pkts peer reported lost
	int cseq_;		// congest epoch seq # (as receiver)
	int highest_cseq_seen_;	// peer's last cseq seen (I am sender)
	int last_cseq_checked_;	// last cseq value at rtt heartbeat
	int last_expected_;	// value of expected_ on last rtt beat
	int silence_;		// # of beats we've been idle
	int silence_thresh_;	// call nopeer() if silence_ > silence_thresh_
	int needresponse_;	// send a packet in response to current one
	int last_ecn_;		// last recv had an ecn

	class TFCCAckTimer : public TimerHandler {
	public: 
		TFCCAckTimer(TFCCAgent *a) : TimerHandler() { a_ = a; }
	protected:      
		void expire(Event *) { a_->ack_timeout(this); }
		TFCCAgent *a_;
	} ack_timer_;	// periodic timer for ack generation

	class TFCCRttTimer : public TimerHandler {
	public: 
		TFCCRttTimer(TFCCAgent *a) : TimerHandler() { a_ = a; }
	protected:
		void expire(Event *) { a_->rtt_timeout(this); }
		TFCCAgent *a_;
	} rtt_timer_; // periodic rtt-based heartbeat
};

static class TFCCAgentClass : public TclClass {
public:
	TFCCAgentClass() : TclClass("Agent/RTP/TFCC") {}
	TclObject* create(int, const char*const*) {
		return (new TFCCAgent());
	}
} class_tfcc_agent;

/************************** methods ********************************/

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
	if (m > maxrtt_)
		maxrtt_ = m;
	if (m < minrtt_)
		minrtt_ = m;
        return srtt_;
}       

void
TFCCAgent::makepkt(Packet* p)
{
	hdr_tfcc* th = (hdr_tfcc*)p->access(hdr_tfcc::offset_);
	hdr_flags* fh = (hdr_flags*)p->access(hdr_flags::offset_);

	th->ts_echo() = last_ts_;
	th->ts() = now();
//th->interval() = interval_;
	th->nlost() = nlost_;
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

printf("%f: %s: recv: seq: %d, cseq: %d, expect:%d\n", now(), name(),
	rh->seqno(), th->cseq(), expected_);

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

	last_ts_ = th->ts();

	if (fh->ect()) {
		// turn around ecn's we see
		if (fh->ce()) {
			last_ecn_ = 1;
			ecn_event();
		} else if (fh->cong_action())
			last_ecn_ = 0;
	}

	int loss = 0;
	if (expected_ >= 0 && (loss = rh->seqno() - expected_) > 0) {
		nlost_ += loss;
		loss_event(loss);
		expected_ = rh->seqno() + 1;
	} else
		++expected_;

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
	}

	if (th->nlost() > plost_)
		plost_ = th->nlost();	// cumulative

        last_rtime_ = now();
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
printf("%f %s: loss event: nlost:%d\n", now(), name(), nlost);
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
	if (!running_) {
		// if we are not a data sender, schedule acks
printf("%s: setting ack_interval to %f\n", name(), peerest);
		ack_interval_ = peerest / 2.0;
		ack_rate_change();
	}
}

/*
 * as a sender, this is called when we first know the rtt
 */
void
TFCCAgent::rtt_known(double rtt)
{
printf("%s: RTT KNOWN (%f), starting timer\n", name(), rtt);
	rtt_timer_.resched(rtt);
}

/*
 * as a sender, this is called when we are receiving acks with no
 * new congestion indications
 */

void
TFCCAgent::speedup()
{
	if (running_) {
		interval_ = (srtt_ * interval_) / (srtt_ + interval_);
printf("%s SPEEDUP [srtt:%f], new interval:%f, ppw:%d\n",
	name(), srtt_, interval_, int(srtt_ / interval_));
		rate_change();
	}
}

/*
 * as a sender, this is called when we are receiving acks with
 * new congestion indications
 */

void
TFCCAgent::slowdown()
{
	if (running_) {
		interval_ *= 2.0;
printf("%s SLOWDOWN [srtt: %f], new interval:%f, ppw:%d\n",
	name(), srtt_, interval_, int(srtt_ / interval_));
		rate_change();
	}
}

/*
 * nopeer: called if we haven't heard any acks from the
 * receiver in 'silence_thresh_' number of rtt ticks
 */

void
TFCCAgent::nopeer()
{
	// for now, just 1/2 sending rate
	slowdown();
	silence_ = 0;
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

/*
 * called on RTT periods to determine action to take
 */
void
TFCCAgent::rtt_timeout(TimerHandler* timer)
{
printf(">>>>> %f: %s: RTT beat: last_checked:%d, highest_seen:%d\n",
    now(), name(), last_cseq_checked_, highest_cseq_seen_);

	if (last_cseq_checked_ < 0) {
		// initialize
		last_cseq_checked_ = highest_cseq_seen_;
		last_expected_ = expected_;
		timer->resched(srtt_);
	} else {
		//
		// check peer's congestion status
		//
		if (last_cseq_checked_ == highest_cseq_seen_) {
			speedup();
		} else {
			slowdown();
			last_cseq_checked_ = highest_cseq_seen_;
		}

		//
		// check if we've heard anything from peer in awhile
		//

		if (expected_ == last_expected_) {
			// nothing since last beat
			if (++silence_ >= silence_thresh_)
				nopeer();
		} else {
			// yep, heard something
			silence_ = 0;
			timer->resched(srtt_);
		}
	}
	return;
}

/*
 * called periodically to send acks
 */
void
TFCCAgent::ack_timeout(TimerHandler* timer)
{
	sendpkt();
	timer->resched(ack_interval_);
}

/******************** EXTENSIONS ****************************/
// VTFCC -- "VegasLike" TFCC (not finished)
class VTFCCAgent : public TFCCAgent {
public:
	VTFCCAgent() : lastseq_(0), lastlost_(0) { }
protected:
	void slowdown();
	void linear_slowdown();
	void speedup();
	void rtt_known(double rtt);

	int lastseq_;
	int lastlost_;

	double expectedrate_;
	double actualrate_;
	double lowerthresh_;
	double upperthresh_;
};

static class VTFCCAgentClass : public TclClass {
public:
	VTFCCAgentClass() : TclClass("Agent/RTP/TFCC/VegasLike") {}
	TclObject* create(int, const char*const*) {
		return (new VTFCCAgent());
	}
} class_vtfcc_agent;

void
VTFCCAgent::rtt_known(double rtt)
{
	expectedrate_ = 1.0 / interval_;
	TFCCAgent::rtt_known(rtt);
}

void
VTFCCAgent::slowdown()
{
	if (running_) {
		interval_ *= 2.0;
printf("%s V-SLOWDOWN [srtt: %f], new interval:%f, ppw:%d\n",
	name(), srtt_, interval_, int(srtt_ / interval_));
		rate_change();
		expectedrate_ = srtt_ / (interval_ * minrtt_);
	}
}

void
VTFCCAgent::linear_slowdown()
{
	if (running_) {
		interval_ = (srtt_ * interval_) / (srtt_ + interval_);
printf("%s V-MILDSLOWDOWN [srtt: %f], new interval:%f, ppw:%d\n",
	name(), srtt_, interval_, int(srtt_ / interval_));
		rate_change();
		expectedrate_ = srtt_ / (interval_ * minrtt_);
	}
}

void
VTFCCAgent::speedup()
{
	if (running_) {
		//
		//  we might thing we want to speed up
		// because we got no drops, but in Vegas, we might
		// want to slow down still
		//
		double actualrate = ((seqno_ - lastseq_) -
			(plost_ - lastlost_)) / srtt_;
		lastlost_ = plost_;	// not quite right XXX
		lastseq_ = seqno_;
		if (actualrate < expectedrate_) {
			linear_slowdown();
			return;
		}

		// otherwise, increase by 1
		interval_ = (srtt_ * interval_) / (srtt_ + interval_);
printf("%s V-SPEEDUP [srtt:%f], new interval:%f, ppw:%d\n",
	name(), srtt_, interval_, int(srtt_ / interval_));
		rate_change();
		expectedrate_ = srtt_ / (interval_ * minrtt_);
	}
}
