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

struct hdr_tfrm {

	int seqno;    //data sequence number
	double rate;    //sender's current rate
	double rtt;     //RTT estimate of sender
	double tzero;     //RTO in Umass eqn
	double timestamp;   //time this message was sent
	int psize;
	int version;

	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_tfrm* access(Packet* p) {
		return (hdr_tfrm*) p->access(offset_);
	}

};

struct hdr_tfrmc {

  int seqno; // not sure yet
  double timestamp; //time this nack was sent
  double timestamp_echo; //timestamp from a data packet
  double timestamp_offset; //offset since we received that data packet
  double flost;
  int signal;

  static int offset_; // offset for this header
  inline static int& offset() { return offset_; }
  inline static hdr_tfrmc* access(Packet* p) {
       return (hdr_tfrmc*) p->access(offset_);
  }

};

#define DECREASE 1
#define NORMAL 2
#define INCREASE 3

class TfrmAgent; 

class TfrmSendTimer : public TimerHandler {
public:
       TfrmSendTimer(TfrmAgent *a) : TimerHandler() { a_ = a; }
       virtual void expire(Event *e);
protected:
       TfrmAgent *a_;
};  

class TfrmAgent : public Agent {
       friend TfrmSendTimer;
public:
       TfrmAgent();
       void recv(Packet*, Handler*);
       void sendpkt();
       void nextpkt();
       int command(int argc, const char*const* argv);
       void start();
       void stop();
protected:
       double rate_, rtt_, rttvar_, tzero_;

       //for TCP tahoe RTO alg.
       int t_srtt_, t_rtt_, t_rttvar_, rttvar_exp_;
       double t_rtxcur_;
       double tcp_tick_;
		int T_SRTT_BITS, T_RTTVAR_BITS ;
		int srtt_init_, rttvar_init_ ;
		double rtxcur_init_ ;

		int InitRate_ ;
       double incrrate_;
       int seqno_, psize_;
       TfrmSendTimer send_timer_;
       int run_;
       double df_;       // decay factor for RTT EWMA
       int version_;
       int slowincr_;
       int k_;
       double last_change_;
       TracedInt ndatapack_;   // number of packets sent
};
