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
#define ECNLOST 4
#define ECNNOLOSS 5

#define DEFAULT_NUMSAMPLES  8

#define WALI 1
#define EWMA 2 
#define RBPH 3
#define EBPH 4 

class TfrcSinkAgent;

class TfrcNackTimer : public TimerHandler {
public:
	TfrcNackTimer(TfrcSinkAgent *a) : TimerHandler() { 
		a_ = a; 
	}
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
	double adjust_history(double);
	double est_loss();
	double est_thput(); 
	int command(int argc, const char*const* argv);
	void print_loss(int sample, double ave_interval);
	void print_loss_all(int *sample);

	// algo specific
	double est_loss_WALI();
	void shift_array(int *a, int sz, int defval) ;
	void shift_array(double *a, int sz, double defval) ;
	void multiply_array(double *a, int sz, double multiplier);
	void init_WALI();
	double weighted_average(int start, int end, double factor, double *m, double *w, int *sample);

	double est_loss_EWMA () ;
	
	double est_loss_RBPH () ;

	double est_loss_EBPH() ;

	//comman variables

	TfrcNackTimer nack_timer_;

	int psize_;		// size of received packet
	double rtt_;		// rtt value reported by sender
	double tzero_;		// timeout value reported by sender
	int smooth_;		// for the smoother method for incorporating
					//  incorporating new loss intervals
	int UrgentFlag ;	// send loss report immediately
	int total_received_;	// total # of pkts rcvd by rcvr
	int bval_;		// value of B used in the formula
	double last_report_sent; 	// when was last feedback sent
	double NumFeedback_; 	// how many feedbacks per rtt
	int rcvd_since_last_report; 	// # of packets rcvd since last report
	int losses_since_last_report;	// # of losses since last report
	int printLoss_;		// to print estimated loss rates
	int maxseq; 		// max seq number seen

	// these assist in keep track of incming packets and calculate flost_
	double last_timestamp_, last_arrival_;
	int hsz;
	char *lossvec_;
	double *rtvec_;
	double *tsvec_;
	int lastloss_round_id ;
	int round_id ;
	double lastloss; 	// when last loss occured

	// WALI specific
	int numsamples ;
	int *sample;
	double *weights ;
	double *mult ;
	double mult_factor_;	// most recent multiple of mult array
	int sample_count ;
	int last_sample ;  
	int init_WALI_flag;

	// these are for "faking" history after slow start
	int loss_seen_yet; 	// have we seen the first loss yet?
	int adjust_history_after_ss; // fake history after slow start? (0/1)
	int false_sample; 	// by how much?
	
	int algo;	// algo for loss estimation 
	int discount ;		// emphasize most recent loss interval
				//  when it is very large

	// EWMA
	double history ;
	double avg_loss_int ; 
	int loss_int ; 
	
	// RBPH, EBPH
	double sendrate ;
	int minlc ; 

}; 
