/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

/* Token Bucket filter which has  3 parameters :
   a. Token Generation rate
   b. Token bucket depth
   c. Max. Queue Length (a finite length would allow this to be used as  policer as packets are dropped after queue gets full)
   */

#include "connector.h" 
#include "packet.h"
#include "queue.h"
#include "tbf.h"

TBF::TBF() :tokens_(0),tbf_timer_(this), init_(1)
{
	q_=new PacketQueue();
	bind_bw("rate_",&rate_);
	bind("bucket_",&bucket_);
	bind("qlen_",&qlen_);
}
	
TBF::~TBF()
{
	if (q_->length() != 0) {
		//Clear all pending timers
		tbf_timer_.cancel();
		//Free up the packetqueue
		for (Packet *p=q_->head();p!=0;p=p->next_) 
			Packet::free(p);
	}
	delete q_;
}


void TBF::recv(Packet *p, Handler *)
{
	//start with a full bucket
	if (init_) {
		tokens_=bucket_;
		lastupdatetime_ = Scheduler::instance().clock();
		init_=0;
	}

	
	hdr_cmn *ch=hdr_cmn::access(p);

	//enque packets appropriately if a non-zero q already exists
	if (q_->length() !=0) {
		if (q_->length() < qlen_) {
			q_->enque(p);
			return;
		}

		drop(p);
		return;
	}

	double tok;
	tok = getupdatedtokens();

	int pktsize = ch->size()<<3;
	if (tokens_ >=pktsize) {
		target_->recv(p);
		tokens_-=pktsize;
	}
	else {
		
		if (qlen_!=0) {
			q_->enque(p);
			tbf_timer_.resched((pktsize-tokens_)/rate_);
		}
		else {
			drop(p);
		}
	}
}

double TBF::getupdatedtokens(void)
{
	double now=Scheduler::instance().clock();
	
	tokens_ += (now-lastupdatetime_)*rate_;
	if (tokens_ > bucket_)
		tokens_=bucket_;
	lastupdatetime_ = Scheduler::instance().clock();
	return tokens_;
}

void TBF::timeout(int)
{
	if (q_->length() == 0) {
		fprintf (stderr,"ERROR in tbf\n");
		abort();
	}
	
	Packet *p=q_->deque();
	double tok;
	tok = getupdatedtokens();
	hdr_cmn *ch=hdr_cmn::access(p);
	int pktsize = ch->size()<<3;

	//We simply send the packet here without checking if we have enough tokens
	//because the timer is supposed to fire at the right time
	target_->recv(p);
	tokens_-=pktsize;

	if (q_->length() !=0 ) {
		p=q_->head();
		hdr_cmn *ch=hdr_cmn::access(p);
		pktsize = ch->size()<<3;
		tbf_timer_.resched((pktsize-tokens_)/rate_);
	}
}

void TBF_Timer::expire(Event* /*e*/)
{
	tbf_->timeout(0);
}


static class TBFClass : public TclClass {
public:
	TBFClass() : TclClass ("TBF") {}
	TclObject* create(int,const char*const*) {
		return (new TBF());
	}
}class_tbf;
