/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999  International Computer Science Institute
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
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
 
#include "tfrc.h"
#include "formula.h"
#include "flags.h"

int hdr_tfrc::offset_;
int hdr_tfrc_ack::offset_;

static class TFRCHeaderClass : public PacketHeaderClass {
public:
	TFRCHeaderClass() : PacketHeaderClass("PacketHeader/TFRC",
					      sizeof(hdr_tfrc)) {
		bind_offset(&hdr_tfrc::offset_);
	}
} class_tfrchdr;

static class TFRC_ACKHeaderClass : public PacketHeaderClass {
public:
	TFRC_ACKHeaderClass() : PacketHeaderClass("PacketHeader/TFRC_ACK",
						  sizeof(hdr_tfrc_ack)) {
		bind_offset(&hdr_tfrc_ack::offset_);
	}
} class_tfrc_ackhdr;

static class TfrcClass : public TclClass {
public:
  	TfrcClass() : TclClass("Agent/TFRC") {}
  	TclObject* create(int, const char*const*) {
    		return (new TfrcAgent());
  	}
} class_tfrc;


TfrcAgent::TfrcAgent() : Agent(PT_TFRC), send_timer_(this), 
	 NoFeedbacktimer_(this), rate_(0), oldrate_(0), maxrate_(0)
{
	bind("packetSize_", &size_);
	bind("rate_", &rate_);
	bind("df_", &df_);
	bind("tcp_tick_", &tcp_tick_);
	bind("ndatapack_", &ndatapack_);
	bind("srtt_init_", &srtt_init_);
	bind("rttvar_init_", &rttvar_init_);
	bind("rtxcur_init_", &rtxcur_init_);
	bind("rttvar_exp_", &rttvar_exp_);
	bind("T_SRTT_BITS", &T_SRTT_BITS);
	bind("T_RTTVAR_BITS", &T_RTTVAR_BITS);
	bind("InitRate_", &InitRate_);
	bind("overhead_", &overhead_);
	bind("ssmult_", &ssmult_);
	bind("bval_", &bval_);
	bind("ca_", &ca_);
	bind_bool("printStatus_", &printStatus_);
	bind_bool("conservative_", &conservative_);
	bind_bool("ecn_", &ecn_);
	bind("maxHeavyRounds_", &maxHeavyRounds_);
	bind("SndrType_", &SndrType_); 
	bind("scmult_", &scmult_);
	seqno_ = -1;
	maxseq_ = 0;
}

void TfrcAgent::advanceby(int delta)
{
  maxseq_ += delta;
	
	// if no packets hve been sent so far, 
	// we call start. Otherwise, we wiat for
	// next packet send timer to expire, which
	// can be aftera long time. Is this the desired
	// behavior?
	
	if (seqno_ == -1) {
  	start(); 
	}
} 

int TfrcAgent::command(int argc, const char*const* argv)
{
	if (argc==2) {
		// are we an infinite sender?
		if ( (strcmp(argv[1],"start")==0) && (SndrType_ == 0)) {
			start();
			return TCL_OK;
		}
		if (strcmp(argv[1],"stop")==0) {
			stop();
			return TCL_OK;
		}
	}
  if ((argc == 3) && (SndrType_ == 1)) {
		// or do we need an FTP type app? 
    if (strcmp(argv[1], "advance") == 0) {
      advanceby(atoi(argv[2]));
      return (TCL_OK);
    }
    if (strcmp(argv[1], "advanceby") == 0) {
      advanceby(atoi(argv[2]));
      return (TCL_OK);
    }
	}
	return (Agent::command(argc, argv));
}

void TfrcAgent::start()
{
	seqno_=0;				
	rate_ = InitRate_;
	delta_ = 0;
	oldrate_ = rate_;  
	rate_change_ = SLOW_START;
	UrgentFlag = 1;
	rtt_=0;	 
	sqrtrtt_=1;
	rttcur_=1;
	tzero_ = 0;
	last_change_=0;
	maxrate_ = 0; 
	ss_maxrate_ = 0;
	ndatapack_=0;
	active_ = 1; 
	round_id = 0;
	heavyrounds_ = 0;
	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	rcvrate = 0 ;

	first_pkt_rcvd = 0 ;
	// send the first packet
	sendpkt();
	// ... at initial rate
	send_timer_.resched(size_/rate_);
	// ... and start timer so we can cut rate 
	// in half if we do not get feedback
	NoFeedbacktimer_.resched(2*size_/rate_); 
}

void TfrcAgent::stop()
{
	active_ = 0;
	send_timer_.force_cancel();
}

void TfrcAgent::nextpkt()
{
	double next = -1;
	double xrate = -1; 

	if (SndrType_ == 0) {
		sendpkt();
	}
	else {
		if (maxseq_ > seqno_) {
			sendpkt();
		}
	}
	
	// during slow start and congestion avoidance, we increase rate
	// slowly - by amount delta per packet 
	if ((rate_change_ == SLOW_START) && (oldrate_+SMALLFLOAT< rate_)) {
		oldrate_ = oldrate_ + delta_;
		xrate = oldrate_;
	}
	else {
		if (ca_) 
			xrate = rate_ * sqrtrtt_/sqrt(rttcur_);
		else
			xrate = rate_;
	}
	if (xrate > SMALLFLOAT) {
		next = size_/xrate;
		//
		// randomize between next*(1 +/- woverhead_) 
		//
		next = next*(2*overhead_*Random::uniform()-overhead_+1);
		if (next > SMALLFLOAT)
			send_timer_.resched(next);
	}
}

void TfrcAgent::update_rtt (double tao, double now) 
{
	/* the TCP update */
	t_rtt_ = int((now-tao) /tcp_tick_ + 0.5);
	if (t_rtt_==0) t_rtt_=1;
	if (t_srtt_ != 0) {
		register short delta;
		delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);    
		if ((t_srtt_ += delta) <= 0)    
			t_srtt_ = 1;
		if (delta < 0)
			delta = -delta;
	  	delta -= (t_rttvar_ >> T_RTTVAR_BITS);
	  	if ((t_rttvar_ += delta) <= 0)  
			t_rttvar_ = 1;
	} else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;		
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);	
	}
	t_rtxcur_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS))) + t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	tzero_=t_rtxcur_;

	/* fine grained RTT estimate for use in the equation */
	if (rtt_ > 0) {
		rtt_ = df_*rtt_ + ((1-df_)*(now - tao));
		sqrtrtt_ = df_*sqrtrtt_ + ((1-df_)*sqrt(now - tao));
	} else {
		rtt_ = now - tao;
		sqrtrtt_ = sqrt(now - tao);
	}
	rttcur_ = now - tao;
}

/*
 * Receive a status report from the receiver.
 */
void TfrcAgent::recv(Packet *pkt, Handler *)
{
	double now = Scheduler::instance().clock();
	hdr_tfrc_ack *nck = hdr_tfrc_ack::access(pkt);
	double ts = nck->timestamp_echo;
	double rate_since_last_report = nck->rate_since_last_report;
	// double NumFeedback_ = nck->NumFeedback_;
	double flost = nck->flost; 
	int losses = nck->losses;

	round_id ++ ;
	UrgentFlag = 0;

	if (rate_since_last_report > 0) {
		/* compute the max rate for slow-start as two times rcv rate */ 
		ss_maxrate_ = 2*rate_since_last_report*size_;
		if (conservative_) { 
			if (losses >= 1) {
				/* there was a loss in the most recent RTT */
				if (debug_) printf("time: %5.2f losses: %d rate %5.2f\n", 
				  now, losses, rate_since_last_report);
				maxrate_ = rate_since_last_report*size_;
			} else { 
				/* there was no loss in the most recent RTT */
				maxrate_ = scmult_*rate_since_last_report*size_;
			}
		} else 
			maxrate_ = 2*rate_since_last_report*size_;
	} else {
		ss_maxrate_ = 0;
		maxrate_ = 0; 
	}

		
	/* update the round trip time */
	update_rtt (ts, now);

	/* .. and estimate of fair rate */
	rcvrate = p_to_b(flost, rtt_, tzero_, size_, bval_);

	/* if we get no more feedback for some time, cut rate in half */
	double t = 2*rtt_ ; 
	if (t < 2*size_/rate_) 
		t = 2*size_/rate_ ; 
	NoFeedbacktimer_.resched(t);
	
	/* if we are in slow start and we just saw a loss */
	/* then come out of slow start */

	if (first_pkt_rcvd == 0) {
		first_pkt_rcvd = 1 ; 
		slowstart ();
		nextpkt();
	}
	else {
		if (rate_change_ == SLOW_START) {
			if (flost > 0) {
				rate_change_ = OUT_OF_SLOW_START;
				oldrate_ = rate_ = rcvrate;
			}
			else {
				slowstart ();
				nextpkt();
			}
		}
		else {
			if (rcvrate>rate_) 
				increase_rate(flost);
			else 
				decrease_rate ();		
		}
	}
	if (printStatus_) {
		printf("time: %5.2f rate: %5.2f\n", now, rate_);
		double packetrate = rate_ * rtt_ / size_;
		printf("time: %5.2f packetrate: %5.2f\n", now, packetrate);
	}
	Packet::free(pkt);
}

void TfrcAgent::slowstart () 
{
	double now = Scheduler::instance().clock(); 

	if (rate_+SMALLFLOAT< size_/rtt_ ) {
		/* if this is the first report, change rate to 1 per rtt */
		/* compute delta so rate increases slowly to new value   */
		oldrate_ = rate_;
		rate_ = size_/rtt_;
		delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
		last_change_ = now;
	} else {
		/* else multiply the rate by ssmult_, and compute delta, */
		/*  so that the rate increases slowly to new value       */
		if (ss_maxrate_ > 0) {
			if (ssmult_*rate_ < ss_maxrate_ && now - last_change_ > rtt_) {
				rate_ = ssmult_*rate_; 
				delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
				last_change_=now;
			} else {
				if ( (oldrate_ > ss_maxrate_) || (rate_ > ss_maxrate_)) {
					if (oldrate_ > ss_maxrate_) {
						delta_ = 0; 
						rate_ = oldrate_ = 0.5*ss_maxrate_;
						last_change_ = now;
					} else {
						rate_ = ss_maxrate_; 
						delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
						last_change_ = now; 
					}
				} else {
					if (now - last_change_ > rtt_) {
						rate_ = ss_maxrate_;
						delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
						last_change_=now;
					}
				}
			}
		} else {
			rate_ = ssmult_*rate_; 
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
		}
	}
}

void TfrcAgent::increase_rate (double p)
{               
        double now = Scheduler::instance().clock();

	double mult = (now-last_change_)/rtt_ ;
	if (mult > 2) mult = 2 ;

	rate_ = rate_ + (size_/rtt_)*mult ;
	double maximumrate = (maxrate_>size_/rtt_)?maxrate_:size_/rtt_ ;
	maximumrate = (maximumrate>rcvrate)?rcvrate:maximumrate;
	rate_ = (rate_ > maximumrate)?maximumrate:rate_ ;
	
        rate_change_ = CONG_AVOID;  
        last_change_ = now;
	heavyrounds_ = 0;
}       

void TfrcAgent::decrease_rate () 
{
	double now = Scheduler::instance().clock(); 
	rate_ = rcvrate;
	double maximumrate = (maxrate_>size_/rtt_)?maxrate_:size_/rtt_ ;

	// Allow sending rate to be greater than maximumrate
	//   (which is by default twice the receiving rate)
	//   for at most maxHeavyRounds_ rounds.
	if (rate_ > maximumrate)
		heavyrounds_++;
	else
		heavyrounds_ = 0;
	if (heavyrounds_ > maxHeavyRounds_) {
		rate_ = (rate_ > maximumrate)?maximumrate:rate_ ;
	}

	rate_change_ = RATE_DECREASE;
	last_change_ = now;
}

void TfrcAgent::sendpkt()
{
	if (active_) {
		Packet* p = allocpkt();
		hdr_tfrc *tfrch = hdr_tfrc::access(p);
		hdr_flags* hf = hdr_flags::access(p);
		if (ecn_) {
			hf->ect() = 1;  // ECN-capable transport
		}
		tfrch->seqno=seqno_++;
		tfrch->timestamp=Scheduler::instance().clock();
		tfrch->rtt=rtt_;
		tfrch->tzero=tzero_;
		tfrch->rate=rate_;
		tfrch->psize=size_;
		tfrch->UrgentFlag=UrgentFlag;
		tfrch->round_id=round_id;
		ndatapack_++;
		send(p, 0);
	}
}

void TfrcAgent::reduce_rate_on_no_feedback()
{
	rate_change_ = RATE_DECREASE; 
	rate_*=0.5;
	UrgentFlag = 1;
	round_id ++ ;
	double t = 2*rtt_ ; 
	if (t < 2*size_/rate_) 
		t = 2*size_/rate_ ; 
	NoFeedbacktimer_.resched(t);
	nextpkt();
}

void TfrcSendTimer::expire(Event *) {
  	a_->nextpkt();
}

void TfrcNoFeedbackTimer::expire(Event *) {
	a_->reduce_rate_on_no_feedback ();
}
