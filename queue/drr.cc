/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 *
 * This file contributed by Sandeep Bajaj <bajaj@parc.xerox.com>, Mar 1997.
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/drr.cc,v 1.10 2002/05/06 22:23:16 difa Exp $
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/drr.cc,v 1.10 2002/05/06 22:23:16 difa Exp $ (Xerox)";
#endif

#include "config.h"   // for string.h
#include <stdlib.h>
#include "queue.h"

class PacketDRR;
class DRR;

class PacketDRR : public PacketQueue {
	PacketDRR(): pkts(0),src(-1),bcount(0),prev(0),next(0),deficitCounter(0),turn(0) {}
	friend class DRR;
	protected :
	int pkts;
	int src;    //to detect collisions keep track of actual src address
	int bcount; //count of bytes in each flow to find the max flow;
	PacketDRR *prev;
	PacketDRR *next;
	int deficitCounter; 
	int turn;
	inline PacketDRR * activate(PacketDRR *head) {
		if (head) {
			this->prev = head->prev;
			this->next = head;
			head->prev->next = this;
			head->prev = this;
			return head;
		}
		this->prev = this;
		this->next = this;
		return this;
	}
	inline PacketDRR * idle(PacketDRR *head) {
		if (head == this) {
			if (this->next == this)
				return 0;
			this->next->prev = this->prev;
			this->prev->next = this->next;
			return this->next;
		}
		this->next->prev = this->prev;
		this->prev->next = this->next;
		return head;
	}
};

class DRR : public Queue {
	public :
	DRR();
	virtual int command(int argc, const char*const* argv);
	Packet *deque(void);
	void enque(Packet *pkt);
	int hash(Packet *pkt);
	void clear();
protected:
	int buckets_ ; //total number of flows allowed
	int blimit_;    //total number of bytes allowed across all flows
	int quantum_;  //total number of bytes that a flow can send
	int mask_;     /*if set hashes on just the node address otherwise on 
			 node+port address*/
	int bytecnt ; //cumulative sum of bytes across all flows
	int pktcnt ; // cumulative sum of packets across all flows
	int flwcnt ; //total number of active flows
	PacketDRR *curr; //current active flow
	PacketDRR *drr ; //pointer to the entire drr struct

	inline PacketDRR *getMaxflow (PacketDRR *curr) { //returns flow with max pkts
		int i;
		PacketDRR *tmp;
		PacketDRR *maxflow=curr;
		for (i=0,tmp=curr; i < flwcnt; i++,tmp=tmp->next) {
			if (maxflow->bcount < tmp->bcount)
				maxflow=tmp;
		}
		return maxflow;
	}
  
public:
	//returns queuelength in packets
	inline int length () {
		return pktcnt;
	}

	//returns queuelength in bytes
	inline int blength () {
		return bytecnt;
	}
};

static class DRRClass : public TclClass {
public:
	DRRClass() : TclClass("Queue/DRR") {}
	TclObject* create(int, const char*const*) {
		return (new DRR);
	}
} class_drr;

DRR::DRR()
{
	buckets_=16;
	quantum_=250;
	drr=0;
	curr=0;
	flwcnt=0;
	bytecnt=0;
	pktcnt=0;
	mask_=0;
	bind("buckets_",&buckets_);
	bind("blimit_",&blimit_);
	bind("quantum_",&quantum_);
	bind("mask_",&mask_);
}
 
void DRR::enque(Packet* pkt)
{
	PacketDRR *q,*remq;
	int which;

	hdr_cmn *ch= hdr_cmn::access(pkt);
	hdr_ip *iph = hdr_ip::access(pkt);
	if (!drr)
		drr=new PacketDRR[buckets_];
	which= hash(pkt) % buckets_;
	q=&drr[which];

	/*detect collisions here */
	int compare=(!mask_ ? ((int)iph->saddr()) : ((int)iph->saddr()&0xfff0));
	if (q->src ==-1)
		q->src=compare;
	else
		if (q->src != compare)
			fprintf(stderr,"Collisions between %d and %d src addresses\n",q->src,(int)iph->saddr());      

	q->enque(pkt);
	++q->pkts;
	++pktcnt;
	q->bcount += ch->size();
	bytecnt +=ch->size();


	if (q->pkts==1)
		{
			curr = q->activate(curr);
			q->deficitCounter=0;
			++flwcnt;
		}
	while (bytecnt > blimit_) {
		Packet *p;
		hdr_cmn *remch;
		hdr_ip *remiph;
		remq=getMaxflow(curr);
		p=remq->deque();
		remch=hdr_cmn::access(p);
		remiph=hdr_ip::access(p);
		remq->bcount -= remch->size();
		bytecnt -= remch->size();
		drop(p);
		--remq->pkts;
		--pktcnt;
		if (remq->pkts==0) {
			curr=remq->idle(curr);
			--flwcnt;
		}
	}
}

Packet *DRR::deque(void) 
{
	hdr_cmn *ch;
	hdr_ip *iph;
	Packet *pkt=0;
	if (bytecnt==0) {
		//fprintf (stderr,"No active flow\n");
		return(0);
	}
  
	while (!pkt) {
		if (!curr->turn) {
			curr->deficitCounter+=quantum_;
			curr->turn=1;
		}

		pkt=curr->lookup(0);  
		ch=hdr_cmn::access(pkt);
		iph=hdr_ip::access(pkt);
		if (curr->deficitCounter >= ch->size()) {
			curr->deficitCounter -= ch->size();
			pkt=curr->deque();
			curr->bcount -= ch->size();
			--curr->pkts;
			--pktcnt;
			bytecnt -= ch->size();
			if (curr->pkts == 0) {
				curr->turn=0;
				--flwcnt;
				curr->deficitCounter=0;
				curr=curr->idle(curr);
			}
			return pkt;
		}
		else {
			curr->turn=0;
			curr=curr->next;
			pkt=0;
		}
	}
	return 0;    // not reached
}

void DRR::clear()
{
	PacketDRR *q =drr;
	int i = buckets_;

	if (!q)
		return;
	while (i--) {
		if (q->pkts) {
			fprintf(stderr, "Changing non-empty bucket from drr\n");
			exit(1);
		}
		++q;
	}
	delete[](drr);
	drr = 0;
}

/*
 *Allows one to change blimit_ and bucket_ for a particular drrQ :
 *
 *
 */
int DRR::command(int argc, const char*const* argv)
{
	if (argc==3) {
		if (strcmp(argv[1], "blimit") == 0) {
			blimit_ = atoi(argv[2]);
			if (bytecnt > blimit_)
				{
					fprintf (stderr,"More packets in buffer than the new limit");
					exit (1);
				}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "buckets") == 0) {
			clear();
			buckets_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1],"quantum") == 0) {
			quantum_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1],"mask")==0) {
			mask_= atoi(argv[2]);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}

int DRR::hash(Packet* pkt)
{
	hdr_ip *iph=hdr_ip::access(pkt);
	int i;
	if (mask_)
		i = (int)iph->saddr() & (0xfff0);
	else
		i = (int)iph->saddr();
	return ((i + (i >> 8) + ~(i>>4)) % ((2<<23)-1))+1; // modulo a large prime
}
