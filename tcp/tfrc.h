/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "timer-handler.h"
#include "random.h"

#define SMALLFLOAT 0.0000001

/* receiver response */ 

#define DECREASE 1
#define NORMAL 2
#define INCREASE 3

/* modes of rate change */
#define SLOW_START 1
#define CONG_AVOID 2
#define RATE_DECREASE	 3
#define OUT_OF_SLOW_START 4 

struct hdr_tfrc {

	int seqno;		//data sequence number
	double rate;		//sender's current rate
	double rtt;	 	//RTT estimate of sender
	double tzero;	 	//RTO in Umass eqn
	double timestamp; 	//time this message was sent
	int psize;		//packet size	
	int UrgentFlag;		//Urgent Flag
	int round_id ; 		//round id.

	static int offset_;	// offset for this header
	inline static int& offset() { 
		return offset_; 
	}
	inline static hdr_tfrc* access(const Packet* p) {
		return (hdr_tfrc*) p->access(offset_);
	}

};

struct hdr_tfrc_ack {

	int seqno;	 	// not sure yet
	double timestamp;		//time this nack was sent
	double timestamp_offset;	//offset since we received data packet
	double timestamp_echo;		//timestamp from the last data packet
	double flost;		//frequnecy of loss indications
	double rate_since_last_report;	//what it says ...
	double NumFeedback_;	//number of times/RTT feedback is to be sent 
	int losses;		// number of losses in last RTT

	static int offset_;		 // offset for this header
	inline static int& offset() { 
		return offset_; 
	}
	inline static hdr_tfrc_ack* access(const Packet* p) {
		return (hdr_tfrc_ack*) p->access(offset_);
	}
};

class TfrcAgent; 

class TfrcSendTimer : public TimerHandler {
public:
		TfrcSendTimer(TfrcAgent *a) : TimerHandler() { a_ = a; }
		virtual void expire(Event *e);
protected:
		TfrcAgent *a_;
};	

class TfrcNoFeedbackTimer : public TimerHandler {
public:
		TfrcNoFeedbackTimer(TfrcAgent *a) : TimerHandler() { a_ = a; }
		virtual void expire(Event *e);
protected:
		TfrcAgent *a_;
}; 

class TfrcAgent : public Agent {
		friend TfrcSendTimer;
	friend TfrcNoFeedbackTimer;
public:
	TfrcAgent();
	void recv(Packet*, Handler*);
	void sendpkt();
	void nextpkt();
	int command(int argc, const char*const* argv);
	void start();
	void stop();
	void update_rtt(double tao, double now); 
	void increase_rate(double p);
	void decrease_rate();
	void slowstart();
	void reduce_rate_on_no_feedback();
	void advanceby(int delta); 

protected:
	TfrcSendTimer send_timer_;
	TfrcNoFeedbackTimer NoFeedbacktimer_;
	int SndrType_;          // 0 -> infinite sender, 1 -> need FTP
	int maxseq_;            // max seq produced by the app so far
	int seqno_;             // next seq to be sent 
	int psize_;
	double rate_;		// send rate
	double oldrate_;	// allows rate to be changed gradually
	double delta_;		// allows rate to be changed gradually
	int rate_change_; 	// slow start, cong avoid, decrease ...
	double rcvrate  ; 	// TCP friendly rate based on current RTT 
				//  and recever-provded loss estimate
	double maxrate_;	// prevents sending at more than 2 times the 
				//  rate at which the receiver is _receving_ 
	double ss_maxrate_;	// max rate for during slowstart
 	int printStatus_;	// to print status reports

	/* "accurate" estimates for formula */
	double rtt_; /*EWMA version*/
	double rttcur_; /*Instantaneous version*/
	double rttvar_;
	double tzero_;
	double sqrtrtt_; /*The mean of the sqrt of the RTT*/

	int ca_; //Enable Sqrt(RTT) based congestion avoidance mode

	/* TCP variables for tracking RTT */
	int t_srtt_; 
	int t_rtt_;
	int t_rttvar_;
	int rttvar_exp_;
	double t_rtxcur_;
	double tcp_tick_;
	int T_SRTT_BITS; 
	int T_RTTVAR_BITS;
	int srtt_init_; 
	int rttvar_init_;
	double rtxcur_init_;

	int InitRate_;		// initial send rate
	double df_;		// decay factor for accurate RTT estimate
	double last_change_;	// time last change in rate was made
	double ssmult_;		// during slow start, increase rate by this 
				//  factor every rtt
	int bval_;		// value of B for the formula
	double overhead_;	// if > 0, dither outgoing packets 
	TracedInt ndatapack_;	// number of packets sent
	int UrgentFlag;		// urgent flag
	int active_;		// have we shut down? 
	int round_id ;		// round id
	int first_pkt_rcvd ;	// first ack received yet?
	int ecn_ ;		// Set to 1 for an ECN-capable connection.

	/* Responses to heavy congestion. */
	int conservative_;	// set to 1 for an experimental, conservative 
				//   response to heavy congestion
	double scmult_;         // self clocking parameter for conservative_
	int heavyrounds_;	// the number of RTTs so far when the
				//  sending rate > 2 * receiving rate
	int maxHeavyRounds_;	// the number of allowed rounds for
				//  sending rate > 2 * receiving rate


};
