/*
 * Copyright (c) 1990, 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef lint
static char rcsid[] =
"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-sack1.cc,v 1.8.2.1 1997/04/16 03:21:25 padmanab Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "scoreboard.h"
#include "random.h"

#define TRUE    1
#define FALSE   0
#define RECOVER_DUPACK  1
#define RECOVER_TIMEOUT 2
#define RECOVER_QUENCH  3

class Sack1TcpAgent : public TcpAgent {
 public:
	Sack1TcpAgent();
	virtual int window();
	virtual void recv(Packet *pkt, Handler*);
	virtual void timeout(int tno);
	void plot();
	void send(int force, int reason, int maxburst);
 protected:
	u_char timeout_;        /* boolean: sent pkt from timeout? */
	u_char fastrecov_;      /* boolean: doing fast recovery? */
	int pipe_;              /* estimate of pipe size (fast recovery) */ 
	ScoreBoard scb_;
};

static class Sack1TcpClass : public TclClass {
public:
	Sack1TcpClass() : TclClass("Agent/TCP/Sack1") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new Sack1TcpAgent());
	}
} class_sack;

int Sack1TcpAgent::window()
{
        return(int(cwnd() < wnd_ ? cwnd() : wnd_));
}

Sack1TcpAgent::Sack1TcpAgent() : pipe_(-1), fastrecov_(FALSE)
{
}

void Sack1TcpAgent::recv(Packet *pkt, Handler*)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);
	int xmit_seqno;
	int dontSend = 0;

#ifdef notdef
	if (pkt->type_ != PT_ACK) {
		Tcl::instance().evalf("%s error \"received non-ack\"",
				      name());
		Packet::free(pkt);
		return;
	}
#endif

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_)
		quench(1);
        if (!fastrecov_) {
                /* normal... not fast recovery */
                if ((int)tcph->seqno() > last_ack_) {
                        /*
                         * regular ACK not in fast recovery... normal
                         */
                        newack(pkt);
                        opencwnd();
                        timeout_ = FALSE;
			scb_.ClearScoreBoard();
                } else if (timeout_ == FALSE)  {
                        if (tcph->seqno() != last_ack_) {
                                fprintf(stderr, "pkt seq %d should be %d\n" ,
                                        tcph->seqno(), last_ack_);
                                abort();
                        }
			scb_.UpdateScoreBoard (last_ack_, tcph);
                        /*
                         * a duplicate ACK
                         */
                        if (++dupacks() == NUMDUPACKS) {
                                /*
                                 * Assume we dropped just one packet.
                                 * Retransmit last ack + 1
                                 * and try to resume the sequence.
                                 */
                                if ((highest_ack() > recover_) ||
                                    (recover_cause_ != RECOVER_TIMEOUT)) {
                                        recover_cause_ = RECOVER_DUPACK;
                                        recover_ = maxseq();
                                        pipe_ = int(cwnd()) - NUMDUPACKS;
                                        closecwnd(1);
                                        reset_rtx_timer(1);
					fastrecov_ = TRUE;
					scb_.MarkRetran (last_ack_+1);
                                        output(last_ack_ + 1, TCP_REASON_DUPACK);
                                }
                        }
                }
                if (dupacks() == 0)
                        send(FALSE, 0, 0);
        } else {
                /* we are in fast recovery */
                --pipe_;
                if ((int)tcph->seqno() >= recover_) {
                        /* ACK indicates fast recovery is over */
			recover_ = 0;
			fastrecov_ = FALSE;
                        newack(pkt);
                        timeout_ = FALSE;
			scb_.ClearScoreBoard();

                        /* New window: W/2 - K or W/2? */
                } else if ((int)tcph->seqno() > highest_ack()) {
                        /* Not out of fast recovery yet.
                         * Update highest_ack_, but not last_ack_. */
			--pipe_;
                        highest_ack() = (int)tcph->seqno();
		 	scb_.UpdateScoreBoard (highest_ack(), tcph);
                        t_backoff() = 1;
                        newtimer(pkt);
                } else if (timeout_ == FALSE) {
                        /* got another dup ack */
			scb_.UpdateScoreBoard (last_ack_, tcph);
                        if (dupacks() > 0)
                                dupacks()++;
                }
                send(FALSE, 0, 0);
        }

	Packet::free(pkt);
#ifdef notyet
	if (trace_)
		plot();
#endif
}

void Sack1TcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupacks() = 0;
		fastrecov_ = FALSE;
		timeout_ = TRUE;
		if (highest_ack() > last_ack_)
			last_ack_ = highest_ack();
#ifdef DEBUGSACK1A
		printf ("timeout. highest_ack: %d seqno: %d\n", 
		  highest_ack(), t_seqno());
#endif
		recover_ = maxseq();
		scb_.ClearScoreBoard();
	}
	TcpAgent::timeout(tno);
}

void Sack1TcpAgent::send(int force, int reason, int maxburst)
{
        register int pktno, found, nextpktno, npacket = 0;
        int win = window();
	int xmit_seqno;

        found = 1;
		if (!force && pending_[TCP_TIMER_DELSND])
			return;
        /*
         * as long as the pipe is open and there is app data to send...
         */
        while (((!fastrecov_  && (t_seqno() <= last_ack_ + win)) ||
            (fastrecov_ && (pipe_ < int(cwnd())))) 
		&& t_seqno() < curseq_ && found) {

                if (overhead_ == 0 || force) {
                        found = 0;

			xmit_seqno = scb_.GetNextRetran ();
#ifdef DEBUGSACK1A
			printf("highest_ack: %d xmit_seqno: %d\n", 
			highest_ack(), xmit_seqno);
#endif
			if (xmit_seqno == -1) { 
			    if ((!fastrecov_ && t_seqno()<=highest_ack()+win)||
			     (fastrecov_ && t_seqno()<=last_ack_+int(2*wnd_))){ 
				found = 1;
				xmit_seqno = t_seqno()++;
#ifdef DEBUGSACK1A
				printf("sending %d fastrecovery: %d win %d\n",
				  xmit_seqno, fastrecov_, win);
#endif
			    }
			}
			else if (recover_>0 &&
				xmit_seqno<=last_ack_+int(2*wnd_)) {
				found = 1;
				scb_.MarkRetran (xmit_seqno);
				win = window();
			}
			if (found) {
                        	output(xmit_seqno, reason);
                                if (t_seqno() <= xmit_seqno)
                                        t_seqno() = xmit_seqno + 1;
				npacket++;
				pipe_++;
			}
                } else if (!pending_[TCP_TIMER_DELSND]) {
                        /*
                         * Set a delayed send timeout.
			 * This is only for the simulator,to add some
			 *   randomization if speficied.
                         */
                        sched(Random::uniform(overhead_), TCP_TIMER_DELSND);
                        return;
                }
                if (maxburst && npacket == maxburst)
                        break;
        } /* while */
}

void Sack1TcpAgent::plot()
{
#ifdef notyet
	double t = Scheduler::instance().clock();
	sprintf(trace_->buffer(), "t %g %d rtt %g\n", 
		t, class_, t_rtt() * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d dev %g\n", 
		t, class_, t_rttvar() * tcp_tick_);
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d win %f\n", t, class_, cwnd());
	trace_->dump();
	sprintf(trace_->buffer(), "t %g %d bck %d\n", t, class_, t_backoff());
	trace_->dump();
#endif
}
