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
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
#define RATE_DECREASE   3
#define OUT_OF_SLOW_START 4 

struct hdr_tfrm {

	int seqno;							//data sequence number
	double rate;    				//sender's current rate
	double rtt;     				//RTT estimate of sender
	double tzero;     			//RTO in Umass eqn
	double timestamp;   		//time this message was sent
	int psize;						  //packet size	
	int version;						//protocol version being used
	int UrgentFlag;					//Urgent Flag

	static int offset_;			// offset for this header
	inline static int& offset() 
		{ return offset_; }
	inline static hdr_tfrm* access(Packet* p) {
		return (hdr_tfrm*) p->access(offset_);
	}

};

struct hdr_tfrmc {

  int seqno; 											// not sure yet
  double timestamp; 							//time this nack was sent
  double timestamp_echo; 					//timestamp from the last data packet
  double timestamp_offset; 				//offset since we received that data packet
  double flost;						 				//frequnecy of loss indications
	double rate_since_last_report;  //what it says ...
  int signal;											//increase, decrease, hold ...

  static int offset_; 						// offset for this header
  inline static int& offset() 
		{ return offset_; }
  inline static hdr_tfrmc* access(Packet* p) {
    return (hdr_tfrmc*) p->access(offset_);
  }
};

class TfrmAgent; 

class TfrmSendTimer : public TimerHandler {
public:
    TfrmSendTimer(TfrmAgent *a) : TimerHandler() { a_ = a; }
    virtual void expire(Event *e);
protected:
    TfrmAgent *a_;
};  

class TfrmNoFeedbackTimer : public TimerHandler {
public:
    TfrmNoFeedbackTimer(TfrmAgent *a) : TimerHandler() { a_ = a; }
    virtual void expire(Event *e);
protected:
    TfrmAgent *a_;
}; 

class TfrmAgent : public Agent {
    friend TfrmSendTimer;
    friend TfrmNoFeedbackTimer;
public:
    TfrmAgent();
    void recv(Packet*, Handler*);
    void sendpkt();
    void nextpkt();
    int command(int argc, const char*const* argv);
    void start();
    void stop();
		void update_rtt (double tao, double now) ; 
		void increase_rate (double p, double now) ;
		void decrease_rate (double p, double now);
		void slowstart ();
		void reduce_rate_on_no_feedback() ;

protected:
		
    TfrmSendTimer send_timer_;
    TfrmNoFeedbackTimer NoFeedbacktimer_;
    int seqno_ ; 
		int psize_;

		/*send rate*/
    TracedDouble rate_;										

		/* these two allow rate to be changed gradually */
		TracedDouble oldrate_ ;									
		double delta_ ; 

		/* slow start, cong avoid, decrease ... */
		int rate_change_ ; 

		/* "accurate" estimates for formula */
		double rtt_ ;
		double rttvar_ ;
		double tzero_;

		/* TCP variables for tracking RTT */
    int t_srtt_, t_rtt_, t_rttvar_, rttvar_exp_;
    double t_rtxcur_;
    double tcp_tick_;
		int T_SRTT_BITS, T_RTTVAR_BITS ;
		int srtt_init_, rttvar_init_ ;
		double rtxcur_init_ ;

		/* intial send rate */
		int InitRate_ ;

		/* rate increase control parameter */
    double incrrate_;

		/* these two specify the cong control algo id */
    int version_;
    int slowincr_;

		/* decay factor for accurate RTT estimate */
    double df_;      

		/* controls size of "congestion window" */	
    double SampleSizeMult_;

		/* time last change in rate was made */
    double last_change_;

		/* during slow start, increase rate by this factor every rtt */
		double ssmult_ ;

		/* value of B for the formula */
    int bval_;

		/* random overhead */
		double overhead_ ;

		/* prevents sending at more than 2 times the rcvr receive rate */
		TracedDouble maxrate_ ;

		/* number of packets sent */
    TracedInt ndatapack_;   

		/* urget flag */
		int UrgentFlag ;

		/* have we shiut down? */
		int active_ ;
};
