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
 * We separate TCP functionality into two parts: that having to do with providing
 * a reliable, ordered byte-stream service, and that having to do with congestion
 * control and loss recovery. The former is done on a per-connection basis and is
 * implemented as part of IntTcpAgent ("integrated TCP"). The latter is done in an 
 * integrated fashion across multiple TCP connections, and is implemented as part 
 * of TcpSessionAgent ("TCP session"). TcpSessionAgent is derived from CorresHost 
 * ("correspondent host"), which keeps track of the state of all TCP (TCP/Int) 
 * connections to a host that it is corresponding with.
 *
 * The motivation for this separation of functionality is to make an ensemble of
 * connection more well-behaved than a set of independent TCP connections and improve
 * the chances of losses being recovered via data-driven techniques (rather than
 * via timeouts). At the same time, we do not introduce any unnecessary between the
 * logically-independent byte-streams that the set of connections represents. This is
 * in contrast to the coupling that is inherent in the multiplexing at the application
 * layer of multiple byte-streams onto a single TCP connection.
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
#include "tcp-session.h"
#include "random.h"

class Islist<TcpSessionAgent> TcpSessionAgent::sessionList_;

static class IntTcpClass : public TclClass {
public:
	IntTcpClass() : TclClass("Agent/TCP/Int") {}
	TclObject* create(int, const char*const*) {
		return (new IntTcpAgent());
	}
} class_tcp_int;

IntTcpAgent::IntTcpAgent() : TcpAgent(), slink(), 
	rxmitPend_(0), closecwTS_(0), session_(0), count_(0), lastTS_(-1), 
	wt_(1), wndIncSeqno_(0), num_thresh_dupack_segs_(0)
{
	bind("rightEdge_", &rightEdge_);
	bind("uniqTS_", &uniqTS_);
	bind("winInc_", &winInc_);
	bind("winMult_", &winMult_);
}

int
IntTcpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (!strcmp(argv[1], "setwt")) {
			if (!session_)
				createTcpSession();
			session_->set_weight(this,atoi(argv[2]));
			return (TCL_OK);
		}
	}
	return (TcpAgent::command(argc,argv));
}

/* 
 * Update the ack information and reset the timer. RTT update is done in 
 * TcpSessionAgent.
 */
void
IntTcpAgent::newack(Packet* pkt)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	last_ack_ = tcph->seqno();
	highest_ack_ = last_ack_;

	if (t_seqno_ < last_ack_ + 1)
		t_seqno_ = last_ack_ + 1;
	/* if the connection is done, call finish() */
	if ((highest_ack_ >= curseq_-1) && !closed_) {
		closed_ = 1;
		finish();
	}
}

void 
IntTcpAgent::recv(Packet *pkt, Handler *)
{
	hdr_tcp *tcph = (hdr_tcp*)pkt->access(off_tcp_);
	int amt_data_acked = 0;

	if (tcph->seqno() > last_ack_) {
		amt_data_acked = tcph->seqno() - last_ack_;
		newack(pkt);
#if 0
		if (rxmitPend_) {
			rxmitPend_ = 0;
			session_->agent_frcov(this); /* XXX needed ? */
		}
#endif
	} 
	session_->recv(this, pkt, amt_data_acked);
}

void
IntTcpAgent::createTcpSession()
{
	Tcl& tcl = Tcl::instance();

	tcl.evalf("%s set node_", name());
	tcl.evalf("%s createTcpSession %d", tcl.result(), (dst_/256)*256);
	Islist_iter<TcpSessionAgent> session_iter(TcpSessionAgent::sessionList_);
	TcpSessionAgent *cur;

	while ((cur = session_iter()) != NULL) {
		if (cur->addr()/256 == addr_/256 && cur->dst()/256 == dst_/256) {
			session_ = cur;
			break;
		}
	}
	if (!session_) {
		printf("In IntTcpAgent::createTcpSession(): failed\n");
		abort();
	}
	session_->add_agent(this, size_, winMult_, winInc_, ssthresh_);
}

void
IntTcpAgent::output(int seqno, int reason)
{
	Packet *pkt = allocpkt();
	hdr_tcp *tcph = (hdr_tcp *) pkt->access(off_tcp_);
	tcph->seqno() = seqno;
	tcph->ts() = Scheduler::instance().clock();
	tcph->ts_echo() = ts_peer_;
	tcph->reason() = reason;

	session_->setflags(pkt);
	
	int bytes = ((hdr_cmn*)pkt->access(off_cmn_))->size();

	/* call helper function to fill in additional fields */
	output_helper(pkt);

        ++ndatapack_;
	ndatabytes_ += bytes;
	send(pkt, 0);
	if (seqno > maxseq_) {
		maxseq_ = seqno;
	}
	else {
        	++nrexmitpack_;
        	nrexmitbytes_ += bytes;
	}
	if (wndIncSeqno_ == 0)
		wndIncSeqno_ = maxseq_;
}


/*
 * Unlike in other flavors of TCP, IntTcpAgent does not decide when to send 
 * packets and how many of them to send. That decision is made by 
 * TcpSessionAgent. So send_much() does little more than defer to 
 * TcpSessionAgent.
 */
void
IntTcpAgent::send_much(int force, int reason, int maxburst)
{
	if (!session_)
		createTcpSession();
	if (!force && delsnd_timer_.status() == TIMER_PENDING)
		return;
	if (overhead_ && !force) {
		delsnd_timer_.resched(Random::uniform(overhead_));
		return;
	}
	session_->send_much(this, force,reason);
}

/*
 * Send one new packet.
 */
void
IntTcpAgent::send_one(int sessionSeqno)
{
	int daddr = dst_/256;
	int dport = dst_%256;
	int sport = addr_%256;

	if (!session_)
		createTcpSession();
	/* 
	 * XXX We assume that the session layer has already made sure that
	 * we have data to send.
	 */
	output(t_seqno_++); 
	session_->add_pkts(size_,t_seqno_-1,sessionSeqno,daddr,dport,sport,lastTS_,this);
}


/*
 * open up the congestion window
 */
void 
IntTcpAgent::opencwnd()
{
	session_->opencwnd(size_, this);
}

/*
 * close down the congestion window
 */
void 
IntTcpAgent::closecwnd(int how)
{   
	session_->closecwnd(how, this);
}

Segment *
IntTcpAgent::rxmit_last(int reason, int seqno, int sessionSeqno, double ts)
{
/*	if (seqno == last_ack_ + 1 && (ts >= rxmitPend_ || rxmitPend_ == 0)) {
		rxmitPend_ = Scheduler::instance().clock();*/
		session_->agent_rcov(this);
		/* 
		 * XXX kludge -- IntTcpAgent is not supposed to deal with 
		 * rtx timer 
		 */
		session_->reset_rtx_timer(1,0); 
		output(seqno, reason);
		daddr_ = dst_/256;
		dport_ = dst_%256;
		sport_ = addr_%256;
		return (session_->add_pkts(size_, seqno, sessionSeqno, daddr_, 
					   dport_, sport_, lastTS_, this));
/*	}*/
	return NULL;
}
u_long output_helper_count=0;
double last_clock=0;
void
IntTcpAgent::output_helper(Packet *p)
{
	double now = Scheduler::instance().clock();
	output_helper_count++;
	last_clock = now;
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);

	/* This is to make sure that we get unique times for each xmission */
	while (uniqTS_ && now <= lastTS_) {
		now += 0.000001; // something arbitrarily small
	}

	lastTS_ = now;
	tcph->ts() = now;
	/* if this is a fast start pkt and not a retransmission, mark it */
	if (session_->fs_pkt() && tcph->seqno() > maxseq_)
		((hdr_flags*)p->access(off_flags_))->fs_ = 1;
	return;
}

int 
IntTcpAgent::data_left_to_send() {
	return (curseq_ > t_seqno_);
}
