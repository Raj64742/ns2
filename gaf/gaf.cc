// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.


#include <template.h>
#include <gaf/gaf.h>
#include <random.h>
#include <address.h>
#include <mobilenode.h>
#include <god.h>
#include <phy.h>
#include <wireless-phy.h>
#include <energy-model.h>



int hdr_gaf::offset_;
static class GAFHeaderClass : public PacketHeaderClass {
public:
	GAFHeaderClass() : PacketHeaderClass("PacketHeader/GAF",
					     sizeof(hdr_gaf)) {
		bind_offset(&hdr_gaf::offset_);
	}
} class_gafhdr;

static class GAFAgentClass : public TclClass {
public:
	GAFAgentClass() : TclClass("Agent/GAF") {}
	TclObject* create(int argc, const char*const* argv) {
	        assert(argc == 5);
		return (new GAFAgent((nsaddr_t) atoi(argv[4])));
	}
} class_gafagent;

static class GAFPartnerClass : public TclClass {
public:
        GAFPartnerClass() : TclClass("GAFPartner") {}
        TclObject* create(int, const char*const*) {
                return (new GAFPartner());
        }
} class_gafparnter;


static inline double 
gafjitter (double max, int be_random_)
{
  return (be_random_ ? Random::uniform(max) : max);
}


GAFAgent::GAFAgent(nsaddr_t id) : Agent(PT_GAF), beacon_(1), randomflag_(1), timer_(this), stimer_(this), dtimer_(this), maxttl_(5), state_(GAF_FREE),total_exist_cnt_(0)
{
        double x = 0.0, y = 0.0, z = 0.0;

	seqno_ = -1;
	nid_ = id;
	thisnode = Node::get_node_by_address(nid_); 
	// gid_ = nid_; // temporary setting, MUST BE RESET

	// grid caculation
	// no need to update location becasue getLoc will do it

	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
	gid_ = God::instance()->getMyGrid(x,y);
        
	if (gid_ < 0) {
	    printf("fatal error: node is outside topography\n");
	}
	
	gn_[0].gid = gid_;
	gn_[0].head = NULL;
	
	/*
	printf("My neighbor info: (%f %f) %d %d %d %d %d \n",
	       x,y,gn_[0].gid,gn_[1].gid, gn_[2].gid, gn_[3].gid,
	       gn_[4].gid);
	*/

	// start handler to update neighborhood table
	// a soft table
	gnhandler_ = new GafNeighborshipHandler(this);
	gnhandler_->start();

}

void GAFAgent::setGn(double x, double y)
{
  	gn_[1].gid = God::instance()->getMyTopGrid(x,y);
	gn_[2].gid = God::instance()->getMyBottomGrid(x,y);
	gn_[3].gid = God::instance()->getMyLeftGrid(x,y);
	gn_[4].gid = God::instance()->getMyRightGrid(x,y);

        gn_[1].head = NULL;
        gn_[2].head = NULL;
        gn_[3].head = NULL;
        gn_[4].head = NULL;     
}

void GAFAgent::recv(Packet* p, Handler *)
{
	hdr_gaf *gafh = hdr_gaf::access(p);

	//int nodeaddr = Address::instance().get_nodeaddr(addr());
	
	switch (gafh->type_) {

	case GAF_DISCOVER:
	  // printf("%d gets discovery msg\n",nodeaddr);
	  processExistenceMsg(p);
	  Packet::free(p);
	  break;
	case GAF_SELECT:
	  // printf("%d gets selectiin msg\n", nodeaddr);
	  processSelectionMsg(p);
	  Packet::free(p);
	  break;
	default:
	  Packet::free(p);
	  break;
	}
}

void GAFAgent::processExistenceMsg(Packet* p)
{
	struct ExistenceMsg emsg;
	u_int32_t dst;
	unsigned char *w = p->accessdata ();
	double x = 0.0, y = 0.0, z = 0.0;
	gaf_neighbor_item *keepHead=NULL;
	int i;
	
	
	dst = *(w++);
	dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
	
	emsg.gid = dst;
	
	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);

	emsg.nid = dst;

	//printf("Node %d received gid/nid = %d %d \n",nid_,emsg.gid, emsg.nid);

	// first, check if this node has changed its grid
	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
        gid_  = God::instance()->getMyGrid(x,y);

	if (gn_[0].gid != gid_) {
				
	    // This node has been move out of its previous grid
	    // we must release all of its neighbor list, however,
	    // if this node moves to its adjacent grid which will most
            // probably happen, we can keep the list of this neighbor
	    
	    for (i=0; i<MAXQUEUE; i++) {
		if (gn_[i].gid != gid_) {
		    releaseGn(&gn_[i]);
		} else {
		    keepHead = gn_[i].head;
		}
	    }
	    
	    // update my grid and adjacent grid
	    
	    gn_[0].gid = gid_;
	    gn_[0].head = keepHead;
	    
	    if (MAXQUEUE > 1) setGn(x,y);

	} else {
	  
	  // if I am the leader, claim it right away
	    if (state_ == GAF_LEADER) {
	        send_selection();
	    }
	}

	// Is the emsg from my current adjacent grid ?

	for (i=0; i<MAXQUEUE; i++) {
            if (gn_[i].gid == emsg.gid) {
                insertGn(&gn_[i], emsg.nid);
            } else {
		// remove the unsync nid
		releaseGnItem(&gn_[i], emsg.nid);
	    }
        }
	
	// if get enough existence msg, try to start selection

	if (state_ == GAF_FREE && gid_ == emsg.gid) {

	  if (total_exist_cnt_++ > MAX_DISCOVERY) {
	    //  send_selection();
	    timeout(GAF_SELECT);
	  }

	}


	// debug

	// printGn();

}

void 
GAFAgent::processSelectionMsg(Packet* p)
{
        struct SelectionMsg smsg;
        u_int32_t dst;
        unsigned char *w = p->accessdata ();
        double x = 0.0, y = 0.0, z = 0.0;
        gaf_neighbor_item *keepHead=NULL;
        int i;

        dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);

        smsg.gid = dst;

        dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);

        smsg.nid = dst;

	dst = *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);
        dst = dst << 8 | *(w++);

	smsg.ttl = dst;	
	
	// do I care ? If I am not in this grid, discard
	// but first, check my grid
	
	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
        gid_  = God::instance()->getMyGrid(x,y);

	// If I have changed my grid, modify my gn info first
	if (gn_[0].gid != gid_) {

            // This node has been move out of its previous grid
            // we must release all of its neighbor list, however,
            // if this node moves to its adjacent grid which will most
            // probably happen, we can keep the list of this neighbor

            for (i=0; i<MAXQUEUE; i++) {
                if (gn_[i].gid != (int)gid_) {
                    releaseGn(&gn_[i]);
                } else {
                    keepHead = gn_[i].head;
                }
            }

            // update my grid and adjacent grid
            gn_[0].gid = gid_;
            gn_[0].head = keepHead;

	    if (MAXQUEUE>1) setGn(x,y);
        }
	
	// Then check if this msg is from my grid
	if (gid_ != smsg.gid) return;

	// if I am the leader already, give up by comparing the
	// the nid. If the incoming guy has a smaller id, give up
	// to be a leader

	if (state_ == GAF_LEADER) {
	  if (smsg.nid > nid_) {
	    // claim again that I am the leader
	    send_selection();
	    return;
	  }
	}

	// Now I am not the leader within this grid, go sleep 
	// May also check if I need to wait for a while or duplicated
	// MSG
	// GOSLEEP (right away ???)
	
	// before I go sleep, schedule an event to wake up myself
	// using the ttl as a clue to decide how long I can sleep.

	dtimer_.resched(Random::uniform(smsg.ttl/2, smsg.ttl));

	// turn off myself

	//printf ("Node %d at grid %d goes sleep between (%f %f)\n",
	//	nid_, gid_, Scheduler::instance().clock(),
	//	Scheduler::instance().clock()+smsg.ttl);

	node_off();

}

void GAFAgent::insertGn(gaf_neighbor *gridp, u_int32_t nid)
{
	gaf_neighbor_item *np, *mp, *sp;
	np = gridp->head;

	// if this node is in the list, just change its ttl.
	for (; np; np = np->next) {
                if (np->nid == nid) {
                        np->ttl = maxttl_;
                        break;
                }
        }

	// otherwise, insert it in order from small to big
	if (!np) {      // insert this new entry

		np = gridp->head;
		mp = new gaf_neighbor_item;
		mp->nid = nid;

		if (np == NULL) {
		    gridp->head = mp;
		    mp->next = NULL;

		} else {
		    sp = np;
		    for (; np; np = np->next) {
		        if (np->nid > mp->nid) break;
			sp = np;
		    }
	
		    if (np) {
			if (np == sp) {  // only one element
			   gridp->head = mp;
			   mp->next = np; 
			} else {
			   sp->next = mp;
			   mp->next = np;
			}

		    } else {	// end of the list
			sp->next = mp;
			mp->next = NULL;
		    }
		}		
        }
}

void GAFAgent::releaseGn(gaf_neighbor *gridp)
{
	gaf_neighbor_item *np, *mp;
        np = gridp->head;
	
	while(np) {

	    mp = np->next;
	    delete np;
	    np = mp;
	}
	
	gridp->head = NULL;
}

// remove the specific item from Gn list. The reason for this
// is that when node changes grid, it may lead to some unsynchronized
// state (the same node may appear in two differt Gn) in the Gn list 
// for some period of time. Although this condition is not really
// critical, it is better to keep the state synchronized. 

void GAFAgent::releaseGnItem(gaf_neighbor *gridp, u_int32_t nid)
{
	gaf_neighbor_item *np, *mp;
        mp = gridp->head;

	if (!mp) return;
	
	np = mp->next;

	for (; np; np = np->next) {
	    if (np->nid == nid) {
		mp->next = np->next;
		delete np;
		return;
	    }
	    
	    if (np->nid > nid) return;
	    mp = np;	
	}

	np = gridp->head;
	if (np->nid == nid) {
	    gridp->head = np->next;
	    delete np;
	}	

}

void GAFAgent::scanGn()
{
	gaf_neighbor *gridp;
	gaf_neighbor_item *np, *mp;
	int i;

	for (i=0; i < MAXQUEUE; i++) {
	    gridp = &gn_[i];
	    mp = gridp->head;

	    if (!mp) continue;

	    np = mp->next;
	    for (; np; np = np->next) {
                    np->ttl--;
                    if (np->ttl <= 0){
                        mp->next = np->next;
                        delete np;
                        np = mp;
                    }
                    mp = np;
             }
	
	     // process the first element
             np = gridp->head;
             np->ttl--;
             if (np->ttl <= 0) {
                 gridp->head = np->next;
                 delete np;
             }
	}
}

void GAFAgent::printGn()
{
	gaf_neighbor *gridp;
        gaf_neighbor_item *np;
        int i;

	printf("At node %d of grid %d:\n",nid_, gid_);

        for (i=0; i < MAXQUEUE; i++) {
	    gridp = &gn_[i];
            np = gridp->head;

	    printf("Grid %d : ",gridp->gid);
	    
	    for (; np; np = np->next) {
		printf("%d -> ",np->nid);
	    }

	    printf("\n");
	}

}

// timeout process for discovery phase
void GAFAgent::timeout(GafMsgType msgt)
{

  //  printf("Node %d times out for Msgtype %d at %f\n", nid_, msgt, Scheduler::instance().clock());

    switch (msgt) {
    case GAF_DISCOVER:

        if (state_ != GAF_SLEEP) {
	    send_discovery(-1);
	    if (state_ == GAF_LEADER) 
	        timer_.resched(Random::uniform(MAX_DISCOVERY_TIME-1,MAX_DISCOVERY_TIME));
	    if (state_ == GAF_FREE)
	        timer_.resched(Random::uniform(0,MIN_DISCOVERY_TIME));
        }

    break;

    case GAF_SELECT:
        if (state_ == GAF_SLEEP) return;

	if (state_ == GAF_LEADER) {
	    send_selection();
	    stimer_.resched(Random::uniform(MIN_SELECT_TIME,MAX_SELECT_TIME));
	} else {
	    // I am in GAF_FREE state
	    // decide if I am the least one in my grid
	    if (gn_[0].head == NULL || (gn_[0].head)->nid > nid_) {
	      
	        

	        // claiming I am the leader of this grid
	        // set state into GAF_LEADER
	      
	        state_ = GAF_LEADER;

	        //printf (" node %d claims to be leader at grid %d at time %f\n",
		//    nid_, gid_, Scheduler::instance().clock());

	        send_selection();
	        stimer_.resched(Random::uniform(MIN_SELECT_TIME,MAX_SELECT_TIME));

	        return;
	    }
	    // if not the leader, go back to check some time later
	    stimer_.resched(gafjitter(MAX_SELECT_TIME, 0));	

	}
	
	break;
 
    case GAF_DUTY:
        duty_timeout();
        break;
    default:
        printf("Wrong GAF msg time!\n");
    }

}

/* timeout process for selecting phase
void GAFAgent::select_timeout(int )
{
        if (state_ == GAF_SLEEP) return;

	// decide if I am the least one in my grid
	if (gn_[0].head == NULL || (gn_[0].head)->nid > nid_) {
	    // claiming I am the leader of this grid
	    // set state into GAF_LEADER
	    state_ = GAF_LEADER;
	    send_selection();
	    stimer_.resched(gafjitter(GAF_BROADCAST_JITTER, 0));
	    return;
	}
	// if not the leader, go back to check some time later
	stimer_.resched(gafjitter(GAF_BROADCAST_JITTER, 0));	
}
*/

// adaptive fidelity timeout

void GAFAgent::duty_timeout()
{
    int i;
    double x=0.0, y=0.0, z=0.0;

    // find where I am
    
    ((MobileNode *)thisnode)->getLoc(&x, &y, &z);
    gid_ = God::instance()->getMyGrid(x,y);

    // clear up my neighbor table

    for (i=0; i < MAXQUEUE; i++) {
         releaseGn(&gn_[i]);
    }

    // wake up myself
    node_on();

    //printf("Node %d at grid %d wakes up at time %f\n",
    //	   nid_, gid_, Scheduler::instance().clock());

    // start discovery timeout and select timeout

    // schedule the discovery timer randomly

    timer_.resched(gafjitter(GAF_STARTUP_JITTER, 1));

    // schedule the select phase after enough time
    // of discovery msg exchange

    stimer_.resched(Random::uniform(MIN_SELECT_TIME, MAX_SELECT_TIME));

}


int GAFAgent::command(int argc, const char*const* argv)
{
        if (argc == 2) {
	  if (strcmp (argv[1], "start-gaf") == 0) {
	    // schedule the discovery timer randomly
	    timer_.resched(gafjitter(GAF_STARTUP_JITTER, 1));
	    // schedule the select phase after certain time
	    // of discovery msg exchange, as fast as possible
	    stimer_.resched(Random::uniform(GAF_STARTUP_JITTER,GAF_STARTUP_JITTER+1));	

	    return (TCL_OK); 
	  }
	  if (strcmp (argv[1], "stop-gaf") == 0) {
		int i;
		for (i=0; i < MAXQUEUE; i++) {
            	    releaseGn(&gn_[i]);
        	}
		return(TCL_OK);
	  }
	      
	}
	if (argc == 3) {
	    if (strcmp(argv[1], "beacon-period") == 0) {
			beacon_ = atoi(argv[2]);
			//timer_.resched(Random::uniform(0, beacon_));
			return TCL_OK;
	    }
	
	    if (strcmp(argv[1], "maxttl") == 0) {
		maxttl_ = atoi(argv[2]);
		return TCL_OK;
	    }

	}
	return (Agent::command(argc, argv));
}

void GAFAgent::send_discovery(int dst)
{
        Packet *p = allocpkt();
	double x=0.0, y=0.0, z=0.0;

	hdr_gaf *h = hdr_gaf::access(p);
	hdr_ip *iph = hdr_ip::access(p);

	h->type_ = GAF_DISCOVER;
	h->seqno_ = ++seqno_;

	// update my grid infomation
	
	((MobileNode *)thisnode)->getLoc(&x, &y, &z);
	gid_ = God::instance()->getMyGrid(x,y);
    
	if (dst != -1) {
	  iph->daddr() = dst;
	  iph->dport() = 254;
	}
	else {
	  // if bcast pkt
	  makeUpExistenceMsg(p);
	}

	//hdrc->direction() = hdr_cmn::DOWN;

	//ch->num_forwards() = 0; 

	send(p,0);
}

// broadcast selecting msg

void GAFAgent::send_selection()
{
	Packet *p = allocpkt();
        hdr_gaf *h = hdr_gaf::access(p);
	//        hdr_ip *iph = hdr_ip::access(p);

        h->type_ = GAF_SELECT;

        makeSelectionMsg(p);

	send(p,0);
}

/* 
 * First phase: tell around who I am
 *              w/ info (gid,nid)
 */
void
GAFAgent::makeUpExistenceMsg(Packet *p)
{
  hdr_ip *iph = hdr_ip::access(p);
  hdr_cmn *hdrc = hdr_cmn::access(p);
  unsigned char *walk;

  // fill up the header
  hdrc->next_hop_ = IP_BROADCAST;
  hdrc->addr_type_ = NS_AF_INET;
  iph->daddr() = IP_BROADCAST << Address::instance().nodeshift();
  iph->dport() = 254;

  // hdrc->direction() = hdr_cmn::DOWN;

  // fill up the data

  p->allocdata(sizeof(ExistenceMsg));
  walk = p->accessdata ();
  hdrc->size_ = sizeof(ExistenceMsg) + IP_HDR_LEN; // Existence Msg + IP

  *(walk++) = gid_ >> 24;
  *(walk++) = (gid_ >> 16) & 0xFF;
  *(walk++) = (gid_ >> 8) & 0xFF;
  *(walk++) = (gid_ >> 0) & 0xFF;
  *(walk++) = nid_ >> 24;
  *(walk++) = (nid_ >> 16) & 0xFF;
  *(walk++) = (nid_ >> 8) & 0xFF;
  *(walk++) = (nid_ >> 0) & 0xFF;

  //printf("Node %d send out Exitence msg gid/nid %d %d\n", nid_, gid_, nid_);

}

void
GAFAgent::makeSelectionMsg(Packet *p)
{
  hdr_ip *iph = hdr_ip::access(p);
  hdr_cmn *hdrc = hdr_cmn::access(p);
  Phy *phyp;
  double maxp,ce;
  unsigned char *walk;
  u_int32_t ttl;

  // fill up the header
  hdrc->next_hop_ = IP_BROADCAST;
  hdrc->addr_type_ = NS_AF_INET;
  iph->daddr() = IP_BROADCAST << Address::instance().nodeshift();
  iph->dport() = 254;

  hdrc->direction() = hdr_cmn::DOWN;

  // estimate TTL, XXX change is needed
  // get Pt_consumer from wireless-phy, assuming only one interface
  // by default, Pt_consumer is the maxp

  phyp = (thisnode->ifhead()).lh_first;
  if (phyp) {
	maxp = ((WirelessPhy *)phyp)->getPtconsume();
  }
  
  // get current energy level 
 
  ce = (thisnode->energy_model())->energy();

  // ttl tells the receiver that how much longer the sender can
  // survive
  ttl = (u_int32_t) ce/maxp;

  // fill up the data
  p->allocdata(sizeof(SelectionMsg));
  walk = p->accessdata ();
  hdrc->size_ = sizeof(SelectionMsg) + IP_HDR_LEN; // Selection Msg + IP

  *(walk++) = gid_ >> 24;
  *(walk++) = (gid_ >> 16) & 0xFF;
  *(walk++) = (gid_ >> 8) & 0xFF;
  *(walk++) = (gid_ >> 0) & 0xFF;
  *(walk++) = nid_ >> 24;
  *(walk++) = (nid_ >> 16) & 0xFF;
  *(walk++) = (nid_ >> 8) & 0xFF;
  *(walk++) = (nid_ >> 0) & 0xFF;
  *(walk++) = ttl >> 24;
  *(walk++) = (ttl >> 16) & 0xFF;
  *(walk++) = (ttl >> 8) & 0xFF;
  *(walk++) = (ttl >> 0) & 0xFF;
}

// bring this node ON/OFF by calling node ON/OFF at PHY
// XXX only consider the first phy, Must be changed for
// the case of multiple interface

void
GAFAgent::node_off()
{
    Phy *p;
    EnergyModel *em;
	
    // if I am in the data transfer state, do not turn off

    // if (state_ == GAF_TRAFFIC) return;

    // set node state
    em = thisnode->energy_model();
    em->node_on() = false;
    //((MobileNode *)thisnode->energy_model())->node_on() = false;

    // notify phy
    p  = (thisnode->ifhead()).lh_first;
    if (p) {
	((WirelessPhy *)p)->node_off();
    }
    // change agent state
    state_ = GAF_SLEEP;
}

void
GAFAgent::node_on()
{
    Phy *p;
    EnergyModel* em;

    // set node state

    em = thisnode->energy_model();
    em->node_on() = true;

    //(MobileNode *)thisnode->energy_model()->node_on() = true;

    // notify phy

    p = (thisnode->ifhead()).lh_first;
    if (p) {
        ((WirelessPhy *)p)->node_on();
    }
    
    total_exist_cnt_ = 0;
    state_ = GAF_FREE;
}

GAFPartner::GAFPartner() : Connector(), gafagent_(1),mask_(0xffffffff),
        shift_(8)
{
        bind("addr_", (int*)&(here_.addr_));
        bind("port_", (int*)&(here_.port_));
        bind("shift_", &shift_);
        bind("mask_", &mask_);
}

void GAFPartner::recv(Packet* p, Handler *h)
{
        hdr_ip* hdr = hdr_ip::access(p);
	hdr_cmn *hdrc = hdr_cmn::access(p);

	/* handle PT_GAF packet only */
	/* convert the dst ip if it is -1, change it into node's */
	/* own ip address */	
	if ( hdrc->ptype() == PT_GAF ) {
	  if (gafagent_ == 1) {
	    if (hdr->daddr() == IP_BROADCAST) {
		hdr->daddr() = here_.addr_;
	    }	    
	  } else {
	    /* if gafagent is not installed, drop the packet */
	    drop (p);
	    return;
	  }
	}
	
	target_->recv(p,h);


}

int GAFPartner::command(int argc, const char*const* argv)
{
        if (argc == 3) {
	  if (strcmp (argv[1], "set-gafagent") == 0) {
	    gafagent_ = atoi(argv[2]);
	    return (TCL_OK); 
	  }
	}
	return Connector::command(argc, argv); 
}

// Gaf handler for timely neighborhood infomation update

void GafNeighborshipHandler::start()
{
        Scheduler::instance().schedule(this, &intr, GN_UPDATE_INTERVAL);
}

void GafNeighborshipHandler::handle(Event *)
{
        

        Scheduler &s = Scheduler::instance();
        gaf_->scanGn();
        s.schedule(this, &intr, GN_UPDATE_INTERVAL);
	//printf ("Node %d update softtabe at %f\n",gaf_->nodeid(), s.clock());
}

void GAFDiscoverTimer::expire(Event *) {
  a_->timeout(GAF_DISCOVER);
}

void GAFSelectTimer::expire(Event *) {
  a_->timeout(GAF_SELECT);
}

void GAFDutyTimer::expire(Event *) {
  a_->timeout(GAF_DUTY);
}




