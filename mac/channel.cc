/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
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
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/channel.cc,v 1.38 2002/06/14 23:15:03 yuri Exp $ (UCB)";
#endif

//#include "template.h"
#include <float.h>

#include "trace.h"
#include "delay.h"
#include "object.h"
#include "packet.h"
#include "mac.h"
#include "channel.h"
#include "lib/bsd-list.h"
#include "phy.h"
#include "wireless-phy.h"
#include "mobilenode.h"
#include "ip.h"
#include "dsr/hdr_sr.h"
#include "gridkeeper.h"


static class ChannelClass : public TclClass {
public:
	ChannelClass() : TclClass("Channel") {}
	TclObject* create(int, const char*const*) {
		return (new Channel);
	}
} class_channel;

/*static class DuplexChannelClass : public TclClass {
public:
	DuplexChannelClass() : TclClass("Channel/Duplex") {}
	TclObject* create(int, const char*const*) {
		return (new DuplexChannel);
	}
	} class_channel_duplex; */

static class WirelessChannelClass : public TclClass {
public:
        WirelessChannelClass() : TclClass("Channel/WirelessChannel") {}
        TclObject* create(int, const char*const*) {
                return (new WirelessChannel);
        }
} class_Wireless_channel;


/* ==================================================================
   NS Initialization Functions
   =================================================================*/
static int ChannelIndex = 0;
Channel::Channel() : TclObject()
{
	index_ = ChannelIndex++;
	LIST_INIT(&ifhead_);
	bind_time("delay_", &delay_);
}

int Channel::command(int argc, const char*const* argv)
{
	
	if (argc == 3) {
		TclObject *obj;

		if( (obj = TclObject::lookup(argv[2])) == 0) {
			fprintf(stderr, "%s lookup failed\n", argv[1]);
			return TCL_ERROR;
		}
		if (strcmp(argv[1], "trace-target") == 0) {
			trace_ = (Trace*) obj;
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "addif") == 0) {
			((Phy*) obj)->insertchnl(&ifhead_);
			((Phy*) obj)->setchnl(this);
			return TCL_OK;
		}
		// add interface for grid_keeper_
		/*else if(strncasecmp(argv[1], "grid_keeper", 5) == 0) {
			grid_keeper_ = (GridKeeper*)obj;
			return TCL_OK;
			}*/
	} else if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		if (strcmp(argv[1], "trace-target") == 0) {
			tcl.resultf("%s", trace_->name());
			return (TCL_OK);
		}
		else if(strcmp(argv[1], "id") == 0) {
			tcl.resultf("%d", index_);
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}


void Channel::recv(Packet* p, Handler* h)
{
//      Scheduler& s = Scheduler::instance();
// 	hdr_cmn::access(p)->direction() = hdr_cmn::UP;
// 	s.schedule(target_, p, txstop_ + delay_ - s.clock());
	sendUp(p, (Phy*)h);
}



void
Channel::sendUp(Packet* p, Phy *tifp)
{
	Scheduler &s = Scheduler::instance();
	Phy *rifp = ifhead_.lh_first;
	Node *tnode = tifp->node();
	Node *rnode = 0;
	Packet *newp;
	double propdelay = 0.0;
	struct hdr_cmn *hdr = HDR_CMN(p);
	
	hdr->direction() = hdr_cmn::UP;
	
        if (GridKeeper::instance()) {
	    int i;
	    GridKeeper* gk = GridKeeper::instance();
	    int size = gk->size_; 
	    
	    MobileNode **outlist = new MobileNode *[size];
	 
       	    int out_index = gk->get_neighbors((MobileNode*)tnode,
						         outlist);
	    for (i=0; i < out_index; i ++) {
		
		  newp = p->copy();
		  rnode = outlist[i];
		  propdelay = get_pdelay(tnode, rnode);

		  rifp = (rnode->ifhead()).lh_first; 
		  for(; rifp; rifp = rifp->nextnode()){
			  if (rifp->channel() == this){
				 s.schedule(rifp, newp, propdelay); 
				 break;
			  }
		  }
 	    }
	    delete [] outlist; 
	} else {
	    for( ; rifp; rifp = rifp->nextchnl()) {
		rnode = rifp->node();
		if(rnode == tnode)
			continue;
		/*
		 * Each node needs to get their own copy of this packet.
		 * Since collisions occur at the receiver, we can have
		 * two nodes canceling and freeing the *same* simulation
		 * event.
		 *
		 */
		newp = p->copy();
		propdelay = get_pdelay(tnode, rnode);
		
		/*
		 * Each node on the channel receives a copy of the
		 * packet.  The propagation delay determines exactly
		 * when the receiver's interface detects the first
		 * bit of this packet.
		 */
		s.schedule(rifp, newp, propdelay);
	    }
	}
	Packet::free(p);
}


double 
Channel::get_pdelay(Node* /*tnode*/, Node* /*rnode*/)
{
	// Dummy function
	return delay_;
}


void
Channel::dump(void)
{
	Phy *n;
	
	fprintf(stdout, "Network Interface List\n");
 	for(n = ifhead_.lh_first; n; n = n->nextchnl() )
		n->dump();
	fprintf(stdout, "--------------------------------------------------\n");
}

/* NoDupChannel------------------------------------------------------------
 * NoDupChannel is currently acting the same as Channel but with one
 * important difference: it uses reference-copying of the packet,
 * thus, a lot of time is saved (e.g. for 49 senders and 1 receiver
 * overflowing mac802.3 and running for 10 seconds sim time, factors
 * of 60 and more of savings in actual running time have been
 * observed).
 *
 * DRAWBACKS: 
 *
 *	- No propagation model supported (uses constant prop delay for 
 *        all nodes), although it should be easy to change that.
 *	- Macs should be EXTREMELY careful handling these reference 
 *        copies: essentially they all are expected not to modify them 
 *	  in any way (including scheduling!) while reference counter is
 *        positive.  802.3 seems to work with it, other macs may need
 *	  some changes.
 */

struct ChannelDelayEvent : public Event {
public:
	ChannelDelayEvent(Packet *p, Phy *txphy) : , p_(p), txphy_(txphy) {};
	Packet *p_;
	Phy *txphy_;
};

class NoDupChannel : public Channel, public Handler {
public:
	void recv(Packet* p, Handler*);	
	void handle(Event*);
protected:
	int phy_counter_;
private:
	void sendUp(Packet *p, Phy *txif);

};

void NoDupChannel::recv(Packet* p, Handler* h) {
	assert(hdr_cmn::access(p)->direction() == hdr_cmn::DOWN);
	// Delay this packet
	Scheduler &s = Scheduler::instance();
	ChannelDelayEvent *de = new ChannelDelayEvent(p, (Phy *)h);
	s.schedule(this, de, delay_);
}
void NoDupChannel::handle(Event *e) {
	ChannelDelayEvent *cde = (ChannelDelayEvent *)e;
	sendUp(cde->p_, cde->txphy_);
	delete cde;
}

void NoDupChannel::sendUp(Packet *p, Phy *txif) {
	struct hdr_cmn *hdr = HDR_CMN(p);
	hdr->direction() = hdr_cmn::UP;

	for(Phy *rifp = ifhead_.lh_first; rifp; rifp = rifp->nextchnl()) {
		if(rifp == txif)
			continue;
		rifp->recv(p->refcopy(), 0);
	}
	Packet::free(p);
}

static class NoDupChannelClass : public TclClass {
public:
	NoDupChannelClass() : TclClass("Channel/NoDup") {}
	TclObject* create(int, const char*const*) {
		return (new NoDupChannel);
	}
} class_nodupchannel;
/* NoDupChannel------------------------------------------------------------
 */

// Wireless extensions
class MobileNode;

WirelessChannel::WirelessChannel(void) : Channel() {}
	
double
WirelessChannel::get_pdelay(Node* tnode, Node* rnode)
{
	// Scheduler	&s = Scheduler::instance();
	MobileNode* tmnode = (MobileNode*)tnode;
	MobileNode* rmnode = (MobileNode*)rnode;
	double propdelay = 0;
	
	propdelay = tmnode->propdelay(rmnode);

	assert(propdelay >= 0.0);
	if (propdelay == 0.0) {
		/* if the propdelay is 0 b/c two nodes are on top of 
		   each other, move them slightly apart -dam 7/28/98 */
		propdelay = 2 * DBL_EPSILON;
		//printf ("propdelay 0: %d->%d at %f\n",
		//	tmnode->address(), rmnode->address(), s.clock());
	}
	return propdelay;
}

	

// send():
//  The packet occupies the channel for the transmission time, txtime
//  If collision occur (>1 pkts overlap), corrupt all pkts involved
//	by setting the error bit or discard them

// int Channel::send(Packet* p, Phy *tifp)
// {
// 	// without collision, return 0
// 	Scheduler& s = Scheduler::instance();
// 	double now = s.clock();

// 	// busy = time when the channel are still busy with earlier tx
// 	double busy = max(txstop_, cwstop_);

// 	// txstop = when the channel is no longer busy from this tx
// 	txstop_ = max(busy, now + txtime);

// 	// now < busy => collision
// 	//	mark the pkt error bit, EF_COLLISION
// 	//	drop if there is a drop target, drop_
// 	if (now < busy) {
// 		// if still transmit earlier packet, pkt_, then corrupt it
// 		if (pkt_ && pkt_->time_ > now) {
// 			hdr_cmn::access(pkt_)->error() |= EF_COLLISION;
// 			if (drop_) {
// 				s.cancel(pkt_);
// 				drop(pkt_);
// 				pkt_ = 0;
// 			}
// 		}

// 		// corrupts the current packet p, and drop if drop_ exists
// 		hdr_cmn::access(p)->error() |= EF_COLLISION;
// 		if (drop_) {
// 			drop(p);
// 			return 1;
// 		}
// 	}

// 	// if p was not dropped, call recv() or hand it to trace_ if present
// 	pkt_ = p;
// 	trace_ ? trace_->recv(p, 0) : recv(p, 0);
// 	return 0;
// }


// contention():
//  The MAC calls this Channel::contention() to enter contention period
//  It determines when the contention window is over, cwstop_,
//	and schedule a callback to the MAC for the actual send()

// void Channel::contention(Packet* p, Handler* h)
// {
// 	Scheduler& s = Scheduler::instance();
// 	double now = s.clock();
// 	if (now > cwstop_) {
// 		cwstop_ = now + delay_;
// 		numtx_ = 0;
// 	}
// 	numtx_++;
// 	s.schedule(h, p, cwstop_ - now);
// }


// jam():
//  Jam the channel for a period txtime
//  Some MAC protocols use this to let other MAC detect collisions

// int Channel::jam(double txtime)
// {
// 	// without collision, return 0
// 	double now = Scheduler::instance().clock();
// 	if (txstop_ > now) {
// 		txstop_ = max(txstop_, now + txtime);
// 		return 1;
// 	}
// 	txstop_ = now + txtime;
// 	return (now < cwstop_);
// }


// int DuplexChannel::send(Packet* p, double txtime)
// {
// 	double now = Scheduler::instance().clock();
// 	txstop_ = now + txtime;
// 	trace_ ? trace_->recv(p, 0) : recv(p, 0);
// 	return 0;
// }


// void DuplexChannel::contention(Packet* p, Handler* h)
// {
// 	Scheduler::instance().schedule(h, p, delay_);
// 	numtx_ = 1;
// }


