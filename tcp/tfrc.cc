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
			 NoFeedbacktimer_(this) 
{
	bind("packetSize_", &size_);
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
	bind("aggr_incr_", &aggr_incr_);
	bind("aggr_dec_", &aggr_dec_);
}


int TfrcAgent::command(int argc, const char*const* argv)
{
	if (argc==2) {
		if (strcmp(argv[1],"start")==0) {
			start();
			return TCL_OK;
		}
		if (strcmp(argv[1],"stop")==0) {
			stop();
			return TCL_OK;
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
	tzero_ = 0;
	last_change_=0;
	maxrate_ = 0; 
	ndatapack_=0;
	active_ = 1; 
	round_id = 0;
	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;
	t_rtxcur_ = rtxcur_init_;
	prevflost = 1 ; prevrtt = 999 ; prevto = 999 ;
	sendpkt();
	send_timer_.resched(size_/rate_);
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

	sendpkt();
	
	// during slow start and congestion avoidance, we increase rate
	// slowly - by amount delta per packet 
	if ((rate_change_ == SLOW_START) && (oldrate_ < rate_)) {
		oldrate_ = oldrate_ + delta_;
		xrate = oldrate_;
	}
	else {
		xrate = rate_;
	}
	if (xrate > SMALLFLOAT) {
		next = size_/xrate; 
		/*
		if (overhead_ > SMALLFLOAT) {
			next = next*(Random::uniform()+0.5);
		}
		*/
		if (next > SMALLFLOAT ) {
			send_timer_.resched(next); 
		}
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

	/* fine grained RTT estimate */

	if (rtt_ > 0) {
		rtt_ = df_*rtt_ + ((1-df_)*(now - tao));
	} else {
		rtt_ = now - tao;
	}

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
	double NumFeedback_ = nck->NumFeedback_;
	double rcvrate; 
	double flost = nck->flost; 

	round_id ++ ;
	UrgentFlag = 0;

	/* compute the max rate as two times rcv rate */ 
	if (rate_since_last_report > 0) 
		maxrate_ = 2*rate_since_last_report*size_;
	else 
		maxrate_ = 0; 
		
	/* update the round trip time */
	update_rtt (ts, now);

	/* if we get no more feedback for some time, cut rate in half */
	if (NumFeedback_ < 1) 
		NoFeedbacktimer_.resched(2*rtt_/NumFeedback_);
	else 
		NoFeedbacktimer_.resched(2*rtt_); 
	
	/* if we are in slow start and we just saw a loss */
	/* then come out of slow start */

	if ((rate_change_ == SLOW_START) && !(flost > 0)) {
		slowstart ();
		Packet::free(pkt);
		return;
	}
	else if ((rate_change_ == SLOW_START) && (flost > 0)) {
		rate_change_ = OUT_OF_SLOW_START;
		rcvrate = p_to_b(flost, rtt_, tzero_, size_, bval_);
		oldrate_ = rate_ = rcvrate;
		prevflost = flost ; 
		prevrtt = rtt_ ; 
		prevto = tzero_ ;
		Packet::free(pkt);
		nextpkt();
		return;
	}

	int x = 0 ;
	if (flost <= prevflost && rtt_ <= prevrtt && tzero_ <= prevto && aggr_incr_) {
		increase_rate(flost);
		x = 1 ;
	}
	else {
		rcvrate = p_to_b(flost, rtt_, tzero_, size_, bval_);
		if (rcvrate>rate_) {
			increase_rate(flost);
			x = 1 ;
		}
		else {
			decrease_rate (flost);		
			x = 2 ;
		}
	}
/*
printf ("%s now=%f %s %s rate=%f rtt=%f p=%f\n", 
x==1?"I":"D", 
now,
rtt_<prevrtt?"RD":"RI",
flost<prevflost?"LD":"LI",
rate_, rtt_, flost);
*/                                                                                            

	prevflost = flost ; 
	prevrtt = rtt_ ; 
	prevto = tzero_ ;
	Packet::free(pkt);
}

void TfrcAgent::slowstart () 
{
	double now = Scheduler::instance().clock(); 

	if (rate_ < size_/rtt_ ) {
		/*if this is the first report, change rate to 1 per rtt*/
		/*compute delta so rate increases slowly to new value */

		oldrate_ = rate_;
		rate_ = size_/rtt_;
		delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
		last_change_ = now;
	} else {

		/*else multiply the rate by ssmult_, and compute delta, so that the*/
		/*rate increases slowly to new value */

		if (maxrate_ > 0) {
			if (ssmult_*rate_ < maxrate_ && now - last_change_ > rtt_) {
				rate_ = ssmult_*rate_; 
				delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
				last_change_=now;
			}
			else {
				if ( (oldrate_ > maxrate_) || (rate_ > maxrate_)) {
					if (oldrate_ > maxrate_) {
						delta_ = 0; 
						rate_ = oldrate_ = 0.5*maxrate_;
						last_change_ = now;
					}
					else {
						rate_ = maxrate_; 
						delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
						last_change_ = now; 
					}
				}
				else {
					if (now - last_change_ > rtt_) {
						rate_ = maxrate_;
						delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
						last_change_=now;
					}
				}
			}
		}
		else {
			rate_ = ssmult_*rate_; 
			delta_ = (rate_ - oldrate_)/(rate_*rtt_/size_);
			last_change_=now;
		}
	}
	nextpkt();
}

void TfrcAgent::increase_rate (double p) 
{
	double rcvrate ;
	double now = Scheduler::instance().clock(); 
	rcvrate = p_to_b(p, rtt_, tzero_, size_, bval_);
	rate_change_ = CONG_AVOID;
	if (rate_ < size_/rtt_) {
		// The sending rate is less than one pkt/RTT.
		rate_ = (rcvrate + rate_)/2;
		if (rate_ > maxrate_)
			rate_ = maxrate_;
  	}
	else {
		// Increase the sending rate by at most one pkt/RTT.
		double mult = (now-last_change_)/rtt_ ;
		if (mult > 2) 
			mult = 2 ;
		rate_ = rate_ + (size_/rtt_)*mult ;
		if (rate_ > maxrate_)
			rate_ = maxrate_;
	}       
	oldrate_ = rate_;
	last_change_ = now;
}

void TfrcAgent::decrease_rate (double p) 
{
	double now = Scheduler::instance().clock(); 
	if (p > prevflost || aggr_dec_) {
		double rcvrate = p_to_b(p, rtt_, tzero_, size_, bval_);
		rate_ = rcvrate;
	}
	else {
		rate_ = rate_*prevrtt/rtt_ ; 
	}
	oldrate_ = rate_;
	rate_change_ = RATE_DECREASE;
	last_change_ = now;
}

void TfrcAgent::sendpkt()
{
	if (active_) {
		Packet* p = allocpkt();
		hdr_tfrc *tfrch = hdr_tfrc::access(p);
		tfrch->seqno=seqno_++;
		tfrch->timestamp=Scheduler::instance().clock();
		tfrch->rtt=rtt_;
		tfrch->tzero=tzero_;
		tfrch->rate=oldrate_;
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
	delta_ = 0;
	UrgentFlag = 1;
	NoFeedbacktimer_.resched(2*rtt_);
	round_id ++ ;
	nextpkt();
}

void TfrcSendTimer::expire(Event *) {
  	a_->nextpkt();
}

void TfrcNoFeedbackTimer::expire(Event *) {
	a_->reduce_rate_on_no_feedback ();
}
