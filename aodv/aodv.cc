/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
 * Reserved. 
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation is hereby granted (including for
 * commercial or for-profit use), provided that both the copyright notice
 * and this permission notice appear in all copies of the software,
 * derivative works, or modified versions, and any portions thereof, and
 * that both notices appear in supporting documentation, and that credit
 * is given to Carnegie Mellon University in all publications reporting
 * on direct or indirect use of this code or its derivatives.
 *
 * ALL CODE, SOFTWARE, PROTOCOLS, AND ARCHITECTURES DEVELOPED BY THE CMU
 * MONARCH PROJECT ARE EXPERIMENTAL AND ARE KNOWN TO HAVE BUGS, SOME OF
 * WHICH MAY HAVE SERIOUS CONSEQUENCES. CARNEGIE MELLON PROVIDES THIS
 * SOFTWARE OR OTHER INTELLECTUAL PROPERTY IN ITS ``AS IS'' CONDITION,
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
 * INTELLECTUAL PROPERTY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Carnegie Mellon encourages (but does not require) users of this
 * software or intellectual property to return any improvements or
 * extensions that they make, and to grant Carnegie Mellon the rights to
 * redistribute these changes without encumbrance.
 *
 * The AODV code developed by the CMU/MONARCH group was optimized
 * and tuned by Samir Das (UTSA) and Mahesh Marina (UTSA). The 
 * work was partially done in Sun Microsystems.
 * 
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/aodv/aodv.cc,v 1.10 2001/02/07 10:25:36 yaxu Exp $
 */

#include "aodv/aodv.h"
#include "aodv/aodv_packet.h"
#include "ip.h"
#include "random.h"
#include "cmu-trace.h"
#include "energy-model.h"
//#include "mobilenode.h"

#define max(a,b)        a > b ? a : b
#define CURRENT_TIME    Scheduler::instance().clock()

//#define DEBUG

#ifdef DEBUG
static int extra_route_reply = 0;
static int limit_route_request = 0;
static int route_request = 0;
#endif


/* ===================================================================
   TCL Hooks
   ================================================================= */
int hdr_aodv::offset_;
static class AODVHeaderClass : public PacketHeaderClass {
public:
        AODVHeaderClass() : PacketHeaderClass("PacketHeader/AODV",
                                              AODV_HDR_LEN) {
		bind_offset(&hdr_aodv::offset_);
	} 
} class_rtProtoAODV_hdr;

static class AODVclass : public TclClass {
public:
        AODVclass() : TclClass("Agent/AODV") {}
        TclObject* create(int argc, const char*const* argv) {
                assert(argc == 5);
                return (new AODV((nsaddr_t) atoi(argv[4])));
        }
} class_rtProtoAODV;



/* ================================================================
   Timers
    ============================================================= */
void
BroadcastTimer::handle(Event*)
{
        agent->id_purge();

        Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

void
HelloTimer::handle(Event*)
{
	agent->sendHello();

    double interval = MinHelloInterval +
            ((MaxHelloInterval - MinHelloInterval) * Random::uniform());

    assert(interval >= 0);

    Scheduler::instance().schedule(this, &intr, interval);
}

void
NeighborTimer::handle(Event*)
{
        agent->nb_purge();

        Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}



void
RouteCacheTimer::handle(Event*)
{
        agent->rt_purge();

#define FREQUENCY 0.5 // sec
        Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

void
LocalRepairTimer::handle(Event* p)    // SRD: 5/4/99
{
	rt_entry *rt;

	/* you get here after the timeout in a local repair attempt */
	/*	fprintf(stderr, "%s\n", __FUNCTION__); */

	struct hdr_ip *ih = HDR_IP( (Packet *)p);
	//	rt = agent->rtable.rt_lookup(ih->dst_);

	rt = agent->rtable.rt_lookup(ih->daddr());

	if (rt && rt->rt_flags != RTF_UP) {  // route is yet to be repaired
		// I will be conservative and bring down the route
		// and send triggered replies upstream.

		/* The following assert fails, not sure why */
		/* assert (rt->rt_flags == RTF_IN_REPAIR); */
		
		agent->rt_down(rt);
/* 
 printf("Node %d: Dst - %d, failed local repair\n",index, rt->rt_dst);
*/
	}
	Packet::free((Packet *)p);
}

/*================================================================== */

AODV::AODV(nsaddr_t id) : Agent(PT_AODV),
        btimer(this), htimer(this), ntimer(this), rtimer(this), lrtimer(this),
        rqueue()
{
        index = id;
        seqno = 1;
        bid = 1;

        LIST_INIT(&nbhead);
        LIST_INIT(&bihead);

        logtarget = 0;
        ifqueue = 0;
}

int
AODV::command(int argc, const char*const* argv)
{
        if(argc == 2) {
                Tcl& tcl = Tcl::instance();

                if(strncasecmp(argv[1], "id", 2) == 0) {
                        tcl.resultf("%d", index);
                        return TCL_OK;
                }
                if(strncasecmp(argv[1], "start", 2) == 0) {
                        btimer.handle((Event*) 0);
#ifndef AODV_LINK_LAYER_DETECTION
                        htimer.handle((Event*) 0);
                        ntimer.handle((Event*) 0);
#endif
                        rtimer.handle((Event*) 0);
                        return TCL_OK;
                }               
        }
        else if(argc == 3) {

                if(strcmp(argv[1], "index") == 0) {
                        index = atoi(argv[2]);
                        return TCL_OK;
                }
                else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0 ) {
                        logtarget = (Trace*) TclObject::lookup(argv[2]);
                        if(logtarget == 0)
                                return TCL_ERROR;
                        return TCL_OK;
                }
                else if(strcmp(argv[1], "drop-target") == 0) {
		        int stat = rqueue.command(argc,argv);
			if (stat != TCL_OK) return stat;
			return Agent::command(argc, argv);
                }
                else if(strcmp(argv[1], "if-queue") == 0) {
                        ifqueue = (PriQueue*) TclObject::lookup(argv[2]);
                        if(ifqueue == 0)
                                return TCL_ERROR;
                        return TCL_OK;
                }
        }
        return Agent::command(argc, argv);
}


/* =====================================================================
   Neighbor Management Functions
   ===================================================================== */
void
AODV::nb_insert(nsaddr_t id)
{
        Neighbor *nb = new Neighbor(id);
        assert(nb);

        nb->nb_expire = CURRENT_TIME +
                (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
        LIST_INSERT_HEAD(&nbhead, nb, nb_link);
        seqno += 1;             // set of neighbors changed
}


Neighbor*
AODV::nb_lookup(nsaddr_t id)
{
        Neighbor *nb = nbhead.lh_first;

        for(; nb; nb = nb->nb_link.le_next) {
                if(nb->nb_addr == id)
                        break;
        }
        return nb;
}


/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
 */
void
AODV::nb_delete(nsaddr_t id)
{
        Neighbor *nb = nbhead.lh_first;
        rt_entry *rt;

        log_link_del(id);

        seqno += 1;     // Set of neighbors changed

        for(; nb; nb = nb->nb_link.le_next) {
                if(nb->nb_addr == id) {
                        LIST_REMOVE(nb,nb_link);
                        delete nb;
                        break;
                }
        }

        for(rt = rtable.head(); rt; rt = rt->rt_link.le_next) {
                if(rt->rt_nexthop == id) {
                        rt_down(rt);
                }
        }
}


/*
 * Purges all timed-out Neighbor Entries - runs every
 * HELLO_INTERVAL * 1.5 seconds.
 */
void
AODV::nb_purge()
{
        Neighbor *nb = nbhead.lh_first;
        Neighbor *nbn;
        double now = CURRENT_TIME;

        for(; nb; nb = nbn) {
                nbn = nb->nb_link.le_next;

                if(nb->nb_expire <= now) {
                        nb_delete(nb->nb_addr);
                }
        }
}


/* =====================================================================
   Broadcast ID Management  Functions
   ===================================================================== */

void
AODV::id_insert(nsaddr_t id, u_int32_t bid)
{
        BroadcastID *b = new BroadcastID(id, bid);
        assert(b);

        b->expire = CURRENT_TIME + BCAST_ID_SAVE;
        LIST_INSERT_HEAD(&bihead, b, link);
}

/* I changed this, SRD */
u_int32_t
AODV::id_lookup(nsaddr_t id, u_int32_t bid)
{
        BroadcastID *b = bihead.lh_first;

	// Search the list for a match of source and bid
        for( ; b; b = b->link.le_next) {
                if ((b->src == id) && (b->id == bid))
		  return ID_FOUND;     
        }
        return ID_NOT_FOUND;
}

void
AODV::id_purge()
{
        BroadcastID *b = bihead.lh_first;
        BroadcastID *bn;
        double now = CURRENT_TIME;

        for(; b; b = bn) {
                bn = b->link.le_next;

                if(b->expire <= now) {
                        LIST_REMOVE(b,link);
                        delete b;
                }
        }
}

/* ================================================================= */

static void
aodv_rt_failed_callback(Packet *p, void *arg)
{
        ((AODV*) arg)->rt_ll_failed(p);
}


/*
 * This routine is invoked when the link-layer reports a route failed.
 */
void
AODV::rt_ll_failed(Packet *p)
{
#ifndef AODV_LINK_LAYER_DETECTION
        drop(p, DROP_RTR_MAC_CALLBACK);
#else
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        rt_entry *rt;

        /*
         * Non-data packets and Broadcast Packets can be dropped.
         */
        if(! DATA_PACKET(ch->ptype()) ||
	   //           (u_int32_t) ih->dst_ == IP_BROADCAST) {
	   ih->daddr() == (nsaddr_t)IP_BROADCAST) {
                drop(p, DROP_RTR_MAC_CALLBACK);
                return;
        }

        log_link_broke(p);

        // if((rt = rtable.rt_lookup(ih->dst_)) == 0) {
	if((rt = rtable.rt_lookup(ih->daddr())) == 0) {
                drop(p, DROP_RTR_MAC_CALLBACK);
                return;
        }

        log_link_del(ch->next_hop_);

		/* if the broken link is closer to the dest than source, 
		 attempt a local repair. Otherwise, bring down the route. */

#ifdef AODV_LOCAL_REPAIR

		if (ch->num_forwards() > rt->rt_hops) {
			local_rt_repair(rt, p); // local repair
			// Mahesh 09/11/99
			// retrieve all the packets in the ifq using this link,
			// queue the packets for which local repair is done, 
			// drop the rest of the packets and send triggered replies
			return;
		}
		else	
#endif
		{
		  /* Increment the sequence no. and bring down the route */
		  rt->rt_seqno++;
		  rt_down(rt);
		  
		  //		  if (index == ih->src_) {
		  if (index == ih->saddr()) {
		    // If I am the source,
		    // queue the packet since rt_down tries to send a request
		    // Mahesh 09/11/99
		         rqueue.enque(p);
		  }
		  else {
		  	drop(p,DROP_RTR_NO_ROUTE);
		  }
		}


#endif /* AODV_LINK_LAYER_DETECTION */
}


void
AODV::local_rt_repair(rt_entry *rt, Packet *p)
{
  	/* fprintf(stderr,"%s: Dst - %d\n", __FUNCTION__, rt->rt_dst); */

	/* Buffer the packet */
	rqueue.enque(p);

	/* mark the route as under repair */
	rt->rt_flags = RTF_IN_REPAIR;

	/* start a route discovery */
	/* Mahesh 09/11/99
	   Note that the following does not ensure that route request 
	   is actually sent.
	*/
	sendRequest(rt->rt_dst);

	/* set up a timer interrupt */
	Scheduler::instance().schedule(&lrtimer, p->copy(),
						rt->rt_req_timeout);
}



void
AODV::rt_down(rt_entry *rt)
{
	/*
  	 *  Make sure that you don't "down" a route more than once.
  	 */

 	if(rt->rt_flags == RTF_DOWN) {
 		return;
 	}
	/*
	// Mahesh 09/11/99 
	// This function has changed considerably from the last version.
	// Need to check whether the next hop for the destination
	// is changed before bringing down the route, for the time being
	// we ignore this case.
	*/

 	rt->rt_flags = RTF_DOWN;
 	rt->rt_expire = 0;
#ifndef ERROR_BROADCAST
    {
   	Neighbor *nb = rt->rt_nblist.lh_first;
	Neighbor *nbn;

    	for( ; nb; nb = nbn) {
       	 nbn = nb->nb_link.le_next;
		 if (nb->nb_expire > CURRENT_TIME)
		 // If this neighbor is still considered active
	     	sendTriggeredReply(nb->nb_addr,
           		                 rt->rt_dst,
               		             rt->rt_seqno);
            LIST_REMOVE(nb, nb_link);
            delete nb;
     	}
	}
#else
	// Broadcast an unsolicited route reply to the upstream neighbors
	sendTriggeredReply(rt->rt_dst, rt->rt_seqno);
#endif


	/*
	 *  Now purge the Network Interface queues that
	 *  may have packets destined for this broken
	 *  neighbor.
	 */

     {
     Packet *p;

      	 while((p = ifqueue->filter(rt->rt_nexthop))) {
	   //         struct hdr_cmn *ch = HDR_CMN(p);
         struct hdr_ip *ih = HDR_IP(p);         

	 //	     if (index == ih->src_) {
	     if (index == ih->saddr()) {
	       // If I am the source of the packet,
	       // queue this packet and send route request.
	       
            	rqueue.enque(p);
		//                sendRequest(ih->dst_);
		sendRequest(ih->daddr());
	     }
	     else {
	       // else drop the packet. We don't try to salvage anything
	       // on an intermediate node.
     	 	drop(p, DROP_RTR_NO_ROUTE);
	     }

         } /* while */
     } /* purge ifq */

} /* rt_down function */


void
AODV::rt_resolve(Packet *p)
{
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        rt_entry *rt;

        /*
         *  Set the transmit failure callback.  That
         *  won't change.
         */
        ch->xmit_failure_ = aodv_rt_failed_callback;
        ch->xmit_failure_data_ = (void*) this;

	//        rt = rtable.rt_lookup(ih->dst_);
	rt = rtable.rt_lookup(ih->daddr());
        if(rt == 0) {
	  //                rt = rtable.rt_add(ih->dst_);
	  rt = rtable.rt_add(ih->daddr());
        }

	/*
	 * If the route is up, forward the packet 
     */
	
        if(rt->rt_flags == RTF_UP) {
                forward(rt, p, NO_DELAY);
        }
	/*
	 *	A local repair is in progress. Buffer the packet. 
	 */
        else if (rt->rt_flags == RTF_IN_REPAIR) {
			rqueue.enque(p);
		}

        /*
         *  if I am the source of the packet, then do a Route Request.
         */
	//        else if(ih->src_ == index) {
	else if(ih->saddr() == index) {
                rqueue.enque(p);
                sendRequest(rt->rt_dst);
        }
        /*
         * I am trying to forward a packet for someone else to which
         * I don't have a route.
         */
		else {
#ifndef ERROR_BROADCAST
		/* 
	 	 * For now, drop the packet and send triggered reply upstream.
		 * Now the unsolicited route replies are broadcast to upstream
		 * neighbors - Mahesh 09/11/99
	 	 */	
        	sendTriggeredReply(ch->prev_hop_,rt->rt_dst, rt->rt_seqno);
#else
        	sendTriggeredReply(rt->rt_dst, rt->rt_seqno);
#endif
            drop(p, DROP_RTR_NO_ROUTE);
        }
}


void
AODV::rt_purge()
{
        rt_entry *rt, *rtn;
        double now = CURRENT_TIME;
	double delay = 0.0;
        Packet *p;

        for(rt = rtable.head(); rt; rt = rtn) {  // for each rt entry
                rtn = rt->rt_link.le_next;

                if ((rt->rt_flags == RTF_UP)
		    && (rt->rt_expire < now)) {
		  // if a valid route has expired, purge all packets from 
		  // send buffer and invalidate the route.
                    
                        while((p = rqueue.deque(rt->rt_dst))) {
#ifdef DEBUG
                                fprintf(stderr, "%s: calling drop()\n",
                                        __FUNCTION__);
#endif
				//                                drop(p, DROP_RTR_RTEXPIRE);
				drop(p, DROP_RTR_NO_ROUTE);
                        }
			rt->rt_flags = RTF_DOWN;

			/*  LIST_REMOVE(rt, rt_link);
                        delete rt; 
			*/
                }
		else if (rt->rt_flags == RTF_UP) {
		  // If the route is not expired,
		  // and there are packets in the sendbuffer waiting,
		  // forward them. This should not be needed, but this extra check
		  // does no harm.
		         while((p = rqueue.deque(rt->rt_dst))) {
			         forward (rt, p, delay);
				 delay += ARP_DELAY;
			 }
		} 

	        else if (rqueue.find(rt->rt_dst))
		  // If the route is down and 
		  // if there is a packet for this destination waiting in
		  // the sendbuffer, then send out route request. sendRequest
		  // will check whether it is time to really send out request
		  // or not.

		  // This may not be crucial to do it here, as each generated 
		  // packet will do a sendRequest anyway.

		          sendRequest(rt->rt_dst); 
        }
}


/* =====================================================================
   Packet Reception Routines
   ===================================================================== */
void
AODV::recv(Packet *p, Handler*)
{
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
	
        assert(initialized());
        //assert(p->incoming == 0);

        if(ch->ptype() == PT_AODV) {

                ih->ttl_ -= 1;

                recvAODV(p);
                return;
        }

	//simply forward GAF packet. It is a hack!!!

	if (ch->ptype() == PT_GAF) {
		send(p,0);
		return;
	}

        /*
         *  Must be a packet I'm originating...
         */
	//        if(ih->src_ == index && ch->num_forwards() == 0) {
	if(ih->saddr() == index && ch->num_forwards() == 0) {
                /*
                 * Add the IP Header
                 */
                ch->size() += IP_HDR_LEN;

                ih->ttl_ = NETWORK_DIAMETER;
        }
        /*
         *  I received a packet that I sent.  Probably
         *  a routing loop.
         */
        //else if(ih->src_ == index) {
	else if(ih->saddr() == index) {
                drop(p, DROP_RTR_ROUTE_LOOP);
                return;
        }
        /*
         *  Packet I'm forwarding...
         */
        else {
                /*
                 *  Check the TTL.  If it is zero, then discard.
                 */
                if(--ih->ttl_ == 0) {
                        drop(p, DROP_RTR_TTL);
                        return;
                }
        }

        rt_resolve(p);
}


void
AODV::recvAODV(Packet *p)
{
        struct hdr_ip *ih = HDR_IP(p);
        struct hdr_aodv *ah = HDR_AODV(p);

        //assert(ih->sport_ == RT_PORT);
        //assert(ih->dport_ == RT_PORT);

        assert(ih->sport() == RT_PORT);
        assert(ih->dport() == RT_PORT);

        /*
         * Incoming Packets.
         */
        switch(ah->ah_type) {

        case AODVTYPE_HELLO:
             recvHello(p);
             break;

        case AODVTYPE_RREQ:
             recvRequest(p);
             break;

        case AODVTYPE_RREP:
             recvReply(p);
             break;
        case AODVTYPE_UREP:
             recvTriggeredReply(p);
             break;
        default:
             fprintf(stderr, "Invalid AODV type (%x)\n", ah->ah_type);
             exit(1);
        }
}


void
AODV::recvRequest(Packet *p)
{
        Node *thisnode;
        struct hdr_ip *ih = HDR_IP(p);
        struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
        rt_entry *rt;

        /*
         * Drop if:
         *      - I'm the source
         *      - I recently heard this request.
         */
        if(rq->rq_src == index) {
#ifdef DEBUG
       fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
#endif
                Packet::free(p);
                return;
        } 

	if (id_lookup(rq->rq_src,rq->rq_bcast_id) == ID_FOUND) {

#ifdef DEBUG
    fprintf(stderr, "%s: discarding request\n", __FUNCTION__);
#endif
           Packet::free(p);
           return;
        }

        /*
         * Cache the broadcast ID
         */
        id_insert(rq->rq_src, rq->rq_bcast_id);

        rt = rtable.rt_lookup(rq->rq_dst);

        /* 
         * We are either going to forward the REQUEST or generate a
         * REPLY. Before we do anything, we make sure that the REVERSE
	 * route is in the route table.
         */
        { 
	    rt_entry *rt0; // rt0 is the reverse route 
	    Packet *buffered_pkt;

        	rt0 = rtable.rt_lookup(rq->rq_src);
        	if(rt0 == 0) { /* if not in the route table */
			// create an entry for the reverse route.
            	rt0 = rtable.rt_add(rq->rq_src);
        	}
                
			/* 
		 	* Changed to make sure the expiry times are set 
		 	* appropriately - Mahesh 09/11/99
		 	*/
	     	if (rt0->rt_flags != RTF_UP) { // Route wasn't up
		       if (rt0->rt_req_timeout > 0.0) {
			 // Reset the soft state and 
			 // Set expiry time to CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT
			 // This is because route is used in the forward direction,
			 // but only sources get benefited by this change
			      rt0->rt_req_cnt = 0;
			      rt0->rt_req_timeout = 0.0; 
			      if (rq->rq_hop_count != INFINITY2)
						rt0->rt_req_last_ttl = rq->rq_hop_count;
			      rt0->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
		       }
		       else {
			// Set expiry time to CURRENT_TIME + REV_ROUTE_LIFE
			      rt0->rt_expire = CURRENT_TIME + REV_ROUTE_LIFE;
		       }
				// In any case, change your entry 
		       rt0->rt_flags = RTF_UP;
#ifdef ERROR_BROADCAST
				rt0->error_propagate_counter = 0; // Mahesh 09/11/99
#endif
		       rt0->rt_hops = rq->rq_hop_count;
		       rt0->rt_seqno = rq->rq_src_seqno;
		       //		       rt0->rt_nexthop = ih->src_;
		        rt0->rt_nexthop = ih->saddr();
		}
		else // Route was up
		  if ((rq->rq_src_seqno > rt0->rt_seqno ) ||
			((rt0->rt_seqno == rq->rq_src_seqno) && 
				(rt0->rt_hops > rq->rq_hop_count))) {
		
		  // If we have a fresher seq no. or lesser #hops for the 
		  // same seq no., update the rt entry. Else don't bother.
	
			rt0->rt_expire = max(rt0->rt_expire, 
				     	CURRENT_TIME + REV_ROUTE_LIFE);
			rt0->rt_flags = RTF_UP;
#ifdef ERROR_BROADCAST
				// Only if next hop is changed
				// rt0->error_propagate_counter = 0;  Mahesh 09/11/99
#endif
			rt0->rt_hops = rq->rq_hop_count;
			rt0->rt_seqno = rq->rq_src_seqno;
			//rt0->rt_nexthop = ih->src_;
			rt0->rt_nexthop = ih->saddr();
		} 
	
			/* Find out whether any buffered packet can benefit from the 
		 	 * reverse route.
		 	 */
		assert (rt0->rt_flags == RTF_UP);
		while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) {
		   	if (rt0 && (rt0->rt_flags == RTF_UP))
				/* need to do the above check again,
			 	* as deque has side-effects */
			   	forward(rt0, buffered_pkt, NO_DELAY);
			}
	
       	}  // End for putting reverse route in rt table


        /*
         * We have taken care of the reverse route stuff. Now see whether 
	 	 * we can send a route reply. 
         */

        // First check if I am the destination ..

        if(rq->rq_dst == index) {
#ifdef DEBUG
		fprintf(stderr, "%d - %s: destination sending reply\n",
			index, __FUNCTION__);
#endif
		// Just to be safe, I use the max. Somebody may have
		// incremented the dst seqno.
		//
		seqno = max((unsigned)seqno, (unsigned)rq->rq_dst_seqno) + 1;
		sendReply(rq->rq_src,           // IP Destination
			  0,                    // Hop Count
			  index,                // Dest IP Address
			  seqno,                // Dest Sequence Num
			  MY_ROUTE_TIMEOUT,     // Lifetime
			  rq->rq_timestamp);    // timestamp
		Packet::free(p);

		// Sending replying, I am in the route
		thisnode = Node::get_node_by_address(index);
		if (thisnode->energy_model() && 
		    thisnode->energy_model()->powersavingflag()) {
			thisnode->energy_model()->set_node_sleep(0);
			thisnode->energy_model()->set_node_state(EnergyModel::INROUTE);
		}
        }

	// I am not the destination, but I may have a fresh enough route.

        else if (rt && (rt->rt_flags == RTF_UP)
		  && (rt->rt_seqno >= rq->rq_dst_seqno)) {

                sendReply(rq->rq_src,
                          rt->rt_hops + 1,
                          rq->rq_dst,
                          rt->rt_seqno,
                          (u_int32_t) (rt->rt_expire - CURRENT_TIME),
                          rq->rq_timestamp);
                Packet::free(p);
		
		thisnode = Node::get_node_by_address(index);
		if (thisnode->energy_model() &&
		    thisnode->energy_model()->powersavingflag()) {
			thisnode->energy_model()->set_node_sleep(0);
			thisnode->energy_model()->set_node_state(EnergyModel::INROUTE);
		}
        }
        /*
         * Can't reply. So forward the  Route Request
         */
        else {
		ih->daddr() = IP_BROADCAST;
		ih->saddr() = index;
		rq->rq_hop_count += 1;
		forward((rt_entry*) 0, p, DELAY);
		thisnode = Node::get_node_by_address(index);
		if (thisnode->energy_model() && 
		    thisnode->energy_model()->powersavingflag()) {
			thisnode->energy_model()->set_node_sleep(0);
			thisnode->energy_model()->set_node_state(EnergyModel::WAITING);
		}
        }
}


void
AODV::recvReply(Packet *p)
{
        Node *thisnode;
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
        rt_entry *rt;
	//	char suppress_reply = 0;
	double delay = 0.0;

#ifdef DEBUG
        fprintf(stderr, "%d - %s: received a REPLY\n", index, __FUNCTION__);
#endif
       
        /*
         *  Handle "Route Errors" separately...
         */
        if(rp->rp_hop_count == INFINITY2) {
                recvTriggeredReply(p);
                return;
        }


        /*
         *  Got a reply. So reset the "soft state" maintained for 
	 *  route requests in the request table. We don't really have
	 *  have a separate request table. It is just a part of the
	 *  routing table itself. 
         */

	// Note that rp_dst is the dest of the data packets, not the
	// the dest of the reply, which is the src of the data packets.

        if((rt = rtable.rt_lookup(rp->rp_dst))) {
		// reset the soft state
	        rt->rt_req_cnt = 0;
            rt->rt_req_timeout = 0.0; 
            if (rp->rp_hop_count != INFINITY2)
				rt->rt_req_last_ttl = rp->rp_hop_count;
        }
        
        /*
         *  If I don't have a rt entry to this host... adding
         */
        if(rt == 0) {
                rt = rtable.rt_add(rp->rp_dst);
        }

        /*
         * Add a forward route table entry... here I am following 
	 	 * Perkins-Royer AODV paper almost literally - SRD 5/99
         */

        if ((rt->rt_flags != RTF_UP) ||  // no route before
            ((rt->rt_seqno < rp->rp_dst_seqno) ||   // newer route 
             ((rt->rt_seqno == rp->rp_dst_seqno) &&  
		   	  (rt->rt_hops > rp->rp_hop_count)))) { // shorter or equal route
	 
	  // Update the rt entry 
             rt->rt_expire = CURRENT_TIME + rp->rp_lifetime;
#ifdef ERROR_BROADCAST
			 if (rt->rt_flags != RTF_UP) // or next hop changed
			 	rt->error_propagate_counter = 0; // Mahesh 09/11/99
#endif
             rt->rt_flags = RTF_UP;
             rt->rt_hops = rp->rp_hop_count;
             rt->rt_seqno = rp->rp_dst_seqno;
             rt->rt_nexthop = ch->prev_hop_;
#ifdef AODV_LINK_LAYER_DETECTION
             rt->rt_errors = 0;
             rt->rt_error_time = 0.0;
#endif
	     //			if (ih->dst_ == index) { // If I am the original source
	             if (ih->daddr() == index) {
	     
				//assert(rp->rp_hop_count);

			// Update the route discovery latency statistics
		    // rp->rp_timestamp is the time of request origination
		
				rt->rt_disc_latency[rt->hist_indx] =
					 (CURRENT_TIME - rp->rp_timestamp)
					 / (double) rp->rp_hop_count;
			
			// increment indx for next time
				rt->hist_indx = (rt->hist_indx + 1) % MAX_HISTORY;
			}	
        }

        /*
         * If reply is for me, discard it.
         */

        //if(ih->dst_ == index) {
	if(ih->daddr() == index) {
                Packet::free(p);
        }

        /*
         * Otherwise, forward the Route Reply.
         */
        else {
	  	// Find the rt entry
	  //rt_entry *rt0 = rtable.rt_lookup(ih->dst_);
            rt_entry *rt0 = rtable.rt_lookup(ih->daddr());
		// If the rt is up, forward
            if (rt0 && (rt0->rt_flags == RTF_UP)) {
#ifdef ERROR_BROADCAST
        rt_entry *rt_dst = rtable.rt_lookup(rp->rp_dst);
			 	(rt_dst->error_propagate_counter)++; // Mahesh 09/11/99
#endif
       		     rp->rp_hop_count += 1;
		     forward(rt0, p, NO_DELAY);
		     thisnode = Node::get_node_by_address(index);
		     if (thisnode->energy_model() &&
			 thisnode->energy_model()->powersavingflag()) {
			     thisnode->energy_model()->set_node_sleep(0);
			     thisnode->energy_model()->set_node_state(EnergyModel::INROUTE);
		     }
            } else {
		    // I don't know how to forward .. drop the reply. 
#ifdef DEBUG
    fprintf(stderr, "%s: droping Route Reply\n", __FUNCTION__);
#endif
	            drop(p, DROP_RTR_NO_ROUTE);
            }
        }

        /*
         * Send all packets queued in the sendbuffer destined for
	 	 * this destination. 
         * XXX - observe the "second" use of p.
         */
        while((p = rqueue.deque(rt->rt_dst))) {
        	if(rt->rt_flags == RTF_UP) {
			// Delay them a little to help ARP. Otherwise ARP 
			// may drop packets. -SRD 5/23/99
            	forward(rt, p, delay);
				delay += ARP_DELAY;
            }
        }
}


void
AODV::recvTriggeredReply(Packet *p)
{
       struct hdr_ip *ih = HDR_IP(p);
       struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
       rt_entry *rt = rtable.rt_lookup(rp->rp_dst);

	if(rt && (rt->rt_flags == RTF_UP)) {
    
	  // We bring down the route if the source of the 
	  // triggered reply is my next hop. We don't check
	  // sequence numbers (!!!) as this next hop won't
	  // forward packets for me anyways.

	  //     	if (rt->rt_nexthop == ih->src_) {
	  if (rt->rt_nexthop == ih->saddr()) {
	    rt->rt_seqno = rp->rp_dst_seqno; 
            rt_down(rt);
        }
    }
    Packet::free(p);
}


void
AODV::recvHello(Packet *p)
{
        struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
        Neighbor *nb;

        /*
         *  XXX: I use a separate TYPE for hello messages rather than
         *  a bastardized Route Reply.
         */
        nb = nb_lookup(rp->rp_dst);
        if(nb == 0) {
                nb_insert(rp->rp_dst);
        }
        else {
                nb->nb_expire = CURRENT_TIME +
                        (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
        }
        Packet::free(p);
}


/* ======================================================================
   Packet Transmission Routines
   ===================================================================== */
void
AODV::forward(rt_entry *rt, Packet *p, double delay)
{
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        Neighbor *nb;
	Node *thisnode;
	
        if(ih->ttl_ == 0) {
#ifdef DEBUG
        fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif
           drop(p, DROP_RTR_TTL);
           return;
        }

#ifndef ERROR_BROADCAST
        /*
         * Keep the "official" Neighbor List up-to-date.
         */
        if(nb_lookup(ch->prev_hop_) == 0) {

                nb_insert(ch->prev_hop_);
        }
#endif

        if(rt) {
	  	// If it is a unicast packet to be forwarded
            assert(rt->rt_flags == RTF_UP);
            rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
#ifndef ERROR_BROADCAST
                /*
                 *  Need to maintain the per-route Neighbor List.  This is
                 *  kept separate from the Neighbor list that is the list
                 *  of ACTIVE neighbors.
                 */
                if(rt->nb_lookup(ch->prev_hop_) == 0) {
                        rt->nb_insert(ch->prev_hop_);
                }
		// set the expiry times
		nb = rt->nb_lookup(ch->prev_hop_);
		nb->nb_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
#endif

            ch->prev_hop_ = index;
            ch->next_hop_ = rt->rt_nexthop;
            ch->addr_type() = NS_AF_INET;
	    ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction

	    //XXX unicast forwarding, log the time, this is
	    //absolutely a HACK for GAF

	    //thisnode = Node::get_node_by_address(index);
	    //((MobileNode *) thisnode)->logrttime(Scheduler::instance().clock());

        }
        else { // if it is a broadcast packet
            assert(ch->ptype() == PT_AODV);
            //assert(ih->dst_ == (nsaddr_t) IP_BROADCAST);
	    assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
            ch->addr_type() = NS_AF_NONE;
	    ch->direction() = hdr_cmn::DOWN;
        }

	//if (ih->dst_ == (nsaddr_t) IP_BROADCAST) {
	if (ih->daddr() == (nsaddr_t) IP_BROADCAST) {
	  	// If it is a broadcast packet
	        assert(rt == 0);
		/*
		 *  Jitter the sending of broadcast packets by 10ms
		 */
		Scheduler::instance().schedule(target_, p, 0.01 * Random::uniform());
	}
	else // Not a broadcast packet 
		if(delay > 0.0) {
	        Scheduler::instance().schedule(target_, p, delay);
	}
	else {
	  	// Not a broadcast packet, no delay, send immediately
            //target_->recv(p, (Handler*) 0);
	        Scheduler::instance().schedule(target_, p, 0.);
	}
}


void
AODV::sendRequest(nsaddr_t dst)
{
Node *thisnode = Node::get_node_by_address(index);
// Allocate a RREQ packet 
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
rt_entry *rt = rtable.rt_lookup(dst);

	assert(rt);

    /*
     *  Rate limit sending of Route Requests. We are very conservative
	 *  about sending out route requests. 
     */

	if ((rt->rt_flags == RTF_UP) ||
	    (rt->rt_req_timeout > CURRENT_TIME)) 
   		return;
      
	// rt_req_cnt is the no. of times we did network-wide broadcast
	// RREQ_RETRIES is the maximum number we will allow before 
	// going to a long timeout.

	if (rt->rt_req_cnt > RREQ_RETRIES) {
		rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
		rt->rt_req_cnt = 0;
		return;
	}

#ifdef DEBUG
        fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d\n",
                ++route_request, index, rt->rt_dst);
#endif

	// set node into active state and remain it for MAX_RREQ_TIMEOUT
	if (thisnode->energy_model() && 
	    thisnode->energy_model()->powersavingflag()) {
		thisnode->energy_model()->set_node_sleep(0);
		thisnode->energy_model()->set_node_state(EnergyModel::WAITING);
	}
	// Fill out the RREQ packet 
	ch->ptype() = PT_AODV;
	ch->size() = IP_HDR_LEN + sizeof(*rq);
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->prev_hop_ = index;          // AODV hack

	ih->saddr() = index;
	ih->daddr() = IP_BROADCAST;
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;

	// Determine the TTL to be used this time. 
	if (0 == rt->rt_req_last_ttl) {
		// first time query broadcast
		 ih->ttl_ = TTL_START;
	} else {
		// Expanding ring search.
		if (rt->rt_req_last_ttl < TTL_THRESHOLD)
			ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
		else {
			// network-wide broadcast
			ih->ttl_ = NETWORK_DIAMETER;
        		rt->rt_req_cnt += 1;
		}
	}

	// PerHopTime is the roundtrip time per hop for route requests.
	// The factor 2.0 is just to be safe .. SRD 5/22/99

	// Also note that we are making timeouts to be larger if we have 
	// done network wide broadcast before. 

	rt->rt_req_timeout = 2.0 * (double) ih->ttl_ * PerHopTime(rt); 
	if (rt->rt_req_cnt > 0)
		rt->rt_req_timeout *= rt->rt_req_cnt;

	//printf("timeout=%f\n",rt->rt_req_timeout);
	rt->rt_req_timeout += CURRENT_TIME;



	// Don't let the timeout to be too large, however .. SRD 6/8/99

	if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
			rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
        
	rt->rt_expire = 0;

#ifdef DEBUG
	 fprintf(stderr, 
	"(%2d) - %2d sending Route Request, dst: %d, tout %f ms\n",
                ++route_request, 
		index, rt->rt_dst, 
		rt->rt_req_timeout - CURRENT_TIME);
#endif	
	
	// remember the TTL used  for the next time
	rt->rt_req_last_ttl = ih->ttl_;

	// Fill up some more fields. 
    rq->rq_type = AODVTYPE_RREQ;
    rq->rq_hop_count = 0;
    rq->rq_bcast_id = bid++;
    rq->rq_dst = dst;
    rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
    rq->rq_src = index;
    rq->rq_src_seqno = seqno++; // Mahesh - 09/11/99
    rq->rq_timestamp = CURRENT_TIME;

    // target_->recv(p, (Handler*) 0);
	Scheduler::instance().schedule(target_, p, 0.);
}

void
AODV::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
                u_int32_t rpseq, u_int32_t lifetime, double timestamp)
{
Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
rt_entry *rt = rtable.rt_lookup(ipdst);

	assert(rt);

    // ch->uid() = 0;
    ch->ptype() = PT_AODV;
    ch->size() = IP_HDR_LEN + sizeof(*rp);
    ch->iface() = -2;
    ch->error() = 0;
    ch->addr_type() = NS_AF_INET;
    ch->next_hop_ = rt->rt_nexthop;
    ch->prev_hop_ = index;          // AODV hack
    ch->direction() = hdr_cmn::DOWN;
    /*
    ih->src_ = index;
    ih->dst_ = ipdst;
    ih->sport_ = RT_PORT;
    ih->dport_ = RT_PORT;
    */
    ih->saddr() = index;
    ih->daddr() = ipdst;
    ih->sport() = RT_PORT;
    ih->dport() = RT_PORT;

    ih->ttl_ = NETWORK_DIAMETER;

    rp->rp_type = AODVTYPE_RREP;
    rp->rp_flags = 0x00;
    rp->rp_hop_count = hop_count;
    rp->rp_dst = rpdst;
    rp->rp_dst_seqno = rpseq;
    rp->rp_lifetime = lifetime;
    rp->rp_timestamp = timestamp;

    //target_->recv(p, (Handler*) 0);     
	Scheduler::instance().schedule(target_, p, 0.);
}

#ifndef ERROR_BROADCAST
void
AODV::sendTriggeredReply(nsaddr_t ipdst, nsaddr_t rpdst, u_int32_t rpseq)
{
	// 08/28/98 - added this extra check
	// 11/15/00 - do not understand why return when ipdst == 0
	// a reasonable explanation is when ipdst = BROADCAST, should quit
	//if(ipdst == 0 || ipdst == index) return;

	if (ipdst == IP_BROADCAST || ipdst == index) return;

        Packet *p = Packet::alloc();
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);

        // ch->uid() = 0;
        ch->ptype() = PT_AODV;
        ch->size() = IP_HDR_LEN + sizeof(*rp);
        ch->iface() = -2;
        ch->error() = 0;
        ch->addr_type() = NS_AF_NONE;
        ch->next_hop_ = 0;
        ch->prev_hop_ = index;          // AODV hack
	/*
        ih->src_ = index;
        ih->dst_ = ipdst;
        ih->sport_ = RT_PORT;
        ih->dport_ = RT_PORT;
	*/
        ih->saddr() = index;
        ih->daddr() = ipdst;
        ih->sport() = RT_PORT;
        ih->dport() = RT_PORT;

        ih->ttl_ = 1;

        rp->rp_type = AODVTYPE_UREP;
        rp->rp_flags = 0x00;
        rp->rp_hop_count = INFINITY2;
        rp->rp_dst = rpdst;
	
        rp->rp_dst_seqno = rpseq;
        rp->rp_lifetime = 0;            // XXX

        target_->recv(p, (Handler*) 0);     
}
#else
void
AODV::sendTriggeredReply(nsaddr_t rpdst, u_int32_t rpseq) {
// Broadcast unsolicited route replies - Mahesh 09/11/99
rt_entry *rt = rtable.rt_lookup(rpdst);

	if (rt->error_propagate_counter == 0) return;

Packet *p = Packet::alloc();
struct hdr_cmn *ch = HDR_CMN(p);
struct hdr_ip *ih = HDR_IP(p);
struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);

    // ch->uid() = 0;
    ch->ptype() = PT_AODV;
    ch->size() = IP_HDR_LEN + sizeof(*rp);
    ch->iface() = -2;
    ch->error() = 0;
    ch->addr_type() = NS_AF_NONE;
    ch->next_hop_ = 0;
    ch->prev_hop_ = index;          // AODV hack

	ih->src_ = index;
	ih->dst_ = IP_BROADCAST;
	ih->sport_ = RT_PORT;
	ih->dport_ = RT_PORT;
    ih->ttl_ = 1;

    rp->rp_type = AODVTYPE_UREP;
    rp->rp_flags = 0x00;
    rp->rp_hop_count = INFINITY2;
    rp->rp_dst = rpdst;
    rp->rp_dst_seqno = rpseq;
    rp->rp_lifetime = 0;        

    //target_->recv(p, (Handler*) 0);     
	// Do we need any jitter?
	Scheduler::instance().schedule(target_, p, 0.);
}
#endif


void
AODV::sendHello()
{
        Packet *p = Packet::alloc();
        struct hdr_cmn *ch = HDR_CMN(p);
        struct hdr_ip *ih = HDR_IP(p);
        struct hdr_aodv_reply *rh = HDR_AODV_REPLY(p);

        // ch->uid() = 0;
        ch->ptype() = PT_AODV;
        ch->size() = IP_HDR_LEN + sizeof(*rh);
        ch->iface() = -2;
        ch->error() = 0;
        ch->addr_type() = NS_AF_NONE;
        ch->prev_hop_ = index;          // AODV hack
	/*
        ih->src_ = index;
        ih->dst_ = IP_BROADCAST;
        ih->sport_ = RT_PORT;
        ih->dport_ = RT_PORT;
	*/

	ih->saddr() = index;
        ih->daddr() = IP_BROADCAST;
        ih->sport() = RT_PORT;
        ih->dport() = RT_PORT;


        ih->ttl_ = 1;

        rh->rp_type = AODVTYPE_HELLO;
        rh->rp_flags = 0x00;
        rh->rp_hop_count = 0;
        rh->rp_dst = index;
        rh->rp_dst_seqno = seqno;
        rh->rp_lifetime = (1 + ALLOWED_HELLO_LOSS) * HELLO_INTERVAL;

        target_->recv(p, (Handler*) 0);
}


/*
======================================================================
                        Helper routines
=======================================================================
*/


double
AODV::PerHopTime(rt_entry *rt)
{
	int num_non_zero = 0, i;
	double total_latency = 0.0;

	if (!rt)
		 return ((double) NODE_TRAVERSAL_TIME );
	
	for (i=0; i < MAX_HISTORY; i++) {
		if (rt->rt_disc_latency[i] > 0.0) {
			 num_non_zero++;
			 total_latency += rt->rt_disc_latency[i];
		}
	}
	if (num_non_zero > 0)
		return(total_latency / (double) num_non_zero);
	else
		return((double) NODE_TRAVERSAL_TIME);
}

