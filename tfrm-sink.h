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
#include "tfrm.h"

#define LARGE_DOUBLE 9999999999.99 
#define SAMLLFLOAT 0.0000001

class TfrmSinkAgent;

class TfrmNackTimer : public TimerHandler {
public:
  TfrmNackTimer(TfrmSinkAgent *a) : TimerHandler() 
		{ a_ = a; }
  virtual void expire(Event *e);
protected:
  TfrmSinkAgent *a_;
};

class TfrmSinkAgent : public Agent {
  friend TfrmNackTimer;
public:
  TfrmSinkAgent();
  void recv(Packet*, Handler*);
protected:
  void sendpkt();
  void nextpkt();
  void increase_pvec(int);

  TfrmNackTimer nack_timer_;

	/* size of received packet */
  int psize_;

	/* sender's reproted send rate */ 
  double rate_;

	/* rtt and timeout value reported by sender */
  double rtt_, tzero_;

	/* frequency of loss events computed by the receuver */
  double flost_;

	/* all these assit in keep track of incming packets and calculate flost_ */

  double last_timestamp_, last_arrival_, last_nack_;
	int InitHistorySize_ ;
  int *pvec_;
  double *tsvec_;
  double *rtvec_;
  double *RTTvec_;
  int pveclen_;
  int pvecfirst_, pveclast_;
  int prevpkt_;

	/* controls the size of congestion window over which flost is computed */
  double SampleSizeMult_;

	/* minimum number of loss events that must be observed to compute flost*/
	int MinNumLoss_ ;

	/* version # of protocol used by sender */
  int version_;

	/* total # of pkts rcvd by rcvr */
  int total_received_;

	/* bounds on loss rate for signal changes in version 0 */
	double HysterisisLower_ ;
	double HysterisisUpper_ ;

	/* value of B used in the formula */
	int bval_ ;

	/* set to 0 until first loss is seen. Used to end slow start */
	int loss_seen_yet ;

	/* time last report was sent */
	double last_report_sent ; 

	/* send feedback these many times per rtt */
	int NumFeedback_;

}; 
