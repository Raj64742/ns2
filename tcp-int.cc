/*
 * Copyright (c) 1997 Regents of the University of California.
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
 * 	This product includes software developed by the Daedalus Research
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

/*
 * tcp-int.cc is a version of TCP that maintains and uses various TCP 
 * control block parameters on a per-host, rather than per-connection,
 * basis.  This makes the host a better net-citizen and also improves
 * the loss recovery of the ensemble of connections.  This is because
 * of more data-driven retransmissions and a reduced dependence on timers.
 * 
 * chost.cc contains the per-host stuff; the per-connection parameters are
 * mostly useless for this version of TCP.  
 * nilist.cc contains some general-purpose linked list routines used by chost
 * and tcp-int.
 * ns-node.tcl has an instproc addCorresHost that tcp-int uses.  It is an 
 * array of all the CorresHost's from a given node to other peer nodes.
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "flags.h"
#include "nilist.h"
#include "tcp-int.h"
#include "chost.h"
#include "random.h"


static class IntTcpClass : public TclClass {
public:
	IntTcpClass() : TclClass("Agent/TCP/Int") {}
	TclObject* create(int, const char*const*) {
		return (new IntTcpAgent());
	}
} class_tcp_int;


IntTcpAgent::IntTcpAgent() : rxmitPend_(0), closecwTS_(0), peer_(0), 
	count_(0), lastTS_(-1)
{
	bind("rightEdge_", &rightEdge_);
	bind("uniqTS_", &uniqTS_);
	bind("winInc_", &winInc_);
	bind("winMult_", &winMult_);
	bind("shift_", &shift_);
	bind("mask_", &mask_);
}

int
IntTcpAgent::window()
{
	return(int(wnd_));
}

void 
IntTcpAgent::recv(Packet *pkt, Handler *)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);

	if (((hdr_flags*)pkt->access(off_flags_))->ecn_ && ecn_)
		quench(1);
	recv_helper(pkt);
	if (tcph->seqno() > last_ack_) {
		newack(pkt);
		if (rxmitPend_) {
			rxmitPend_ = 0;
			peer_->agent_frcov(this);
		}
		opencwnd();
	} else if (tcph->seqno() == last_ack_)  {
		if (++dupacks_ == NUMDUPACKS) {
		} else if (dupacks_ > NUMDUPACKS) {
		}
	}
	peer_->clean_segs(size_, pkt, this, rightEdge_, uniqTS_);
	Packet::free(pkt);
/*
	if (trace_)
		plot();
*/
	/*
	 * Try to send more data
	 */
	if (peer_->ok_to_snd(size_)) {
		IntTcpAgent *next = peer_->who_to_snd(ROUND_ROBIN);
		next->send_much(0, 0, maxburst_);
	}
}


void
IntTcpAgent::initPeer()
{
	Tcl& tcl = Tcl::instance();

	/* initialize and bind relevant parameters */
	firstsent_ = Scheduler::instance().clock();
	tcl.evalf("%s set node_", name());
	tcl.evalf("%s addCorresHost %d %d %d %d %d", tcl.result(), daddr_, 
		  0, 1500, maxcwnd_*size_, wnd_option_);
	peer_ = (CorresHost *) TclObject::lookup(tcl.result());
	if (peer_ == 0) {
		printf("Unexpected null peer_\n");
		abort();
	}
	peer_->add_agent(this, size_, winMult_, winInc_, ssthresh_);
}


/*
 * Try to send as much data as the window will allow.  The link layer will 
 * do the buffering; we ask the application layer for the size of the packets.
 */
void 
IntTcpAgent::send_much(int force, int reason, int maxburst)
{
	int win = window();
	int npackets = 0;

	daddr_ = dst_ >> shift_;
	dport_ = dst_ & mask_;
	sport_ = addr_ & mask_;

	if (peer_ == NULL) {
		initPeer();
	}
	/* peer_ shouldn't be NULL now */
	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	if (burstsnd_timer_.status() == TIMER_PENDING)
		return;

	while (t_seqno_ <= highest_ack_ + win && t_seqno_ < curseq_) {
		if (!peer_->ok_to_snd(size_) && reason!=TCP_REASON_TIMEOUT && 
		    !force)
			break;
		if (overhead_ == 0 || force) {
			TcpAgent::output(t_seqno_++, reason);
			peer_->add_pkts(size_,t_seqno_-1,daddr_,dport_,sport_, 
					lastTS_, this);
			npackets++;
		} else if (!(delsnd_timer_.status() == TIMER_PENDING)) {
			/*
			 * Set a delayed send timeout.
			 */
			delsnd_timer_.resched(Random::uniform(overhead_));
			return;
		}
		if (maxburst && npackets == maxburst)
			break;
		if (reason == TCP_REASON_TIMEOUT || force)
			break;
	}
}

void 
IntTcpAgent::timeout(int tno)
{
	if (tno == TCP_TIMER_RTX) {
		dupacks_ = 0;
		if (bug_fix_) recover_ = maxseq_;
		TcpAgent::timeout(tno);
	}
}

/*
 * open up the congestion window
 */
void 
IntTcpAgent::opencwnd()
{
       peer_->opencwnd(size_, this);
}

/*
 * close down the congestion window
 */
void 
IntTcpAgent::closecwnd(int how)
{   
    peer_->closecwnd(how, this);
}

int
IntTcpAgent::rxmit_last(int reason, int seqno, double ts)
{
	if (uniqTS_ || 
	    (seqno == last_ack_+1 && (ts == rxmitPend_ || rxmitPend_ == 0))) {
		rxmitPend_ = Scheduler::instance().clock();
		peer_->agent_rcov(this);
		if (seqno > recover_) {	/* not a partial ack retransmission */
			peer_->closecwnd(1, ts, this);    
			recover_ = maxseq_;
		}
		reset_rtx_timer(1);
		TcpAgent::output(seqno, reason);
		daddr_ = dst_ >> shift_;
		dport_ = dst_ & mask_;
		sport_ = addr_ & mask_;
		peer_->add_pkts(size_, seqno, dst_, dport_, sport_, 
				lastTS_, this);
		return 1;
	}
	return 0;
}

void
IntTcpAgent::output_helper(Packet *p)
{
	double now = Scheduler::instance().clock();
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	/* This is to make sure that we get unique times for each xmission */
	while (uniqTS_ && now <= lastTS_) {
		now += 0.000001; // something arbitrarily small
	}
	lastTS_ = now;
	tcph->ts() = now;
	return;
}
