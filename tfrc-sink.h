/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright(c) 1991-1997 Regents of the University of California.
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
 * DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "timer-handler.h"
#include "random.h"
#include "tfrc.h"

#define LARGE_DOUBLE 9999999999.99 
#define SAMLLFLOAT 0.0000001

/* packet status */
#define UNKNOWN 0
#define RCVD 1
#define LOST 2
#define NOLOSS 3

#define MAXSAMPLES 7

class TfrcSinkAgent;

class TfrcNackTimer : public TimerHandler {
public:
  	TfrcNackTimer(TfrcSinkAgent *a) : TimerHandler() 
		{ a_ = a; }
  	virtual void expire(Event *e);
protected:
  	TfrcSinkAgent *a_;
};

class TfrcSinkAgent : public Agent {
	friend TfrcNackTimer;
public:
	TfrcSinkAgent();
	void recv(Packet*, Handler*);
protected:
	void sendpkt(double);
	void nextpkt(double);
	void increase_pvec(int);
	void add_packet_to_history(Packet *);
	double adjust_history();
	double est_loss();
	double est_thput(); 

	TfrcNackTimer nack_timer_;

	int psize_;		// size of received packet
	double rate_;		// sender's reported send rate
	double rtt_;		// rtt value reported by sender
	double tzero_;		// timeout value reported by sender
	double flost_;		// frequency of loss events computed
				// by the receiver

	// these assist in keep track of incming packets and calculate flost_
	double last_timestamp_, last_arrival_, last_nack_;
	int InitHistorySize_;
	char *pvec_;
	char *lossvec_;
	double *tsvec_;
	double *rtvec_;
	double *RTTvec_;
	int prevpkt_;
	int maxseq; 

	int total_received_;	// total # of pkts rcvd by rcvr

	/* bounds on loss rate for signal changes in version 0 */
	double HysterisisLower_;
	double HysterisisUpper_;

	int bval_;		// value of B used in the formula

	/* set to 0 until first loss is seen. Used to end slow start */
	int loss_seen_yet;

	/* time last report was sent */
	double last_report_sent; 

	/* send feedback these many times per rtt */
	double NumFeedback_;

	int rcvd_since_last_report;
	double lastloss;
	int adjust_history_after_ss;
	int false_sample;
	int round_id ;
	int lastloss_round_id ;
}; 
