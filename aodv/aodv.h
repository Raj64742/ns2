/* -*- c++ -*-
   aodv.h
   $I`d$
   */

/* The AODV code developed by the CMU/MONARCH group was optimized
 * and tuned by Samir Das (UTSA) and Mahesh Marina (UTSA). The 
 * work was partially done in Sun Microsystems.
 * 
 * The original CMU copyright is below. 
 */

/*
Copyright (c) 1997, 1998 Carnegie Mellon University.  All Rights
Reserved. 

Permission to use, copy, modify, and distribute this
software and its documentation is hereby granted (including for
commercial or for-profit use), provided that both the copyright notice
and this permission notice appear in all copies of the software,
derivative works, or modified versions, and any portions thereof, and
that both notices appear in supporting documentation, and that credit
is given to Carnegie Mellon University in all publications reporting
on direct or indirect use of this code or its derivatives.

ALL CODE, SOFTWARE, PROTOCOLS, AND ARCHITECTURES DEVELOPED BY THE CMU
MONARCH PROJECT ARE EXPERIMENTAL AND ARE KNOWN TO HAVE BUGS, SOME OF
WHICH MAY HAVE SERIOUS CONSEQUENCES. CARNEGIE MELLON PROVIDES THIS
SOFTWARE OR OTHER INTELLECTUAL PROPERTY IN ITS ``AS IS'' CONDITION,
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
INTELLECTUAL PROPERTY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

Carnegie Mellon encourages (but does not require) users of this
software or intellectual property to return any improvements or
extensions that they make, and to grant Carnegie Mellon the rights to
redistribute these changes without encumbrance.

*/

#ifndef __aodv_h__
#define __aodv_h__

#include <sys/types.h>
#include <list.h>

#include <agent.h>
#include <packet.h>
#include <scheduler.h>

#include <cmu-trace.h>
#include <priqueue.h>
#include <rtqueue.h>
#include <rttable.h>


//#define AODV_LOCAL_REPAIR
/*
 * Allows local repair of routes. Did not show significant performance
 * improvements so far. Make it off until we have a chance to improve
 * it.
 */

#define AODV_LINK_LAYER_DETECTION

/*
 * Allows AODV to use link-layer (802.11) feedback in determining when
 * links are up/down. Use it.
 */

//#define AODV_USE_LL_METRIC
/*
 *  Causes AODV to apply a "smoothing" function to the link layer feedback
 *  that is generated by 802.11.  In essence, it requires that RT_MAX_ERROR
 *  errors occurs within a window of RT_MAX_ERROR_TIME before the link
 *  is considered bad. Don't use it.
 */

//#define AODV_USE_GOD_FEEDBACK

/*
 *  Only applies if AODV_USE_LL_METRIC is defined.
 *
 *  Causes AODV to apply omniscient knowledge to the feedback received
 *  from 802.11.  This may be flawed, because it does not account for
 *  congestion.
 */

//#define ERROR_BROADCAST
/*  We are trying to experiment with broadcasting triggered replies
 *  for route errors, rather than unicasting. It has not been tested 
 * fully yet. Don't use it for now.
 */

class AODV;

#define AODV_HDR_LEN    64      // amount of space allocated in the pkt hdr

#define ID_NOT_FOUND    0x00
#define ID_FOUND        0x01

//#define INFINITY        0xff

/*
 * Constants defined in AODV internet draft. May need changes for performance
 * tuning. But the following has worked well. 
 */

#define MY_ROUTE_TIMEOUT        60             	// 60 seconds
#define ACTIVE_ROUTE_TIMEOUT    50				// 50 seconds
#define REV_ROUTE_LIFE          10				// 10 seconds
#define BCAST_ID_SAVE           6				// 6 seconds


// Should be set by the user using best guess (conservative) 
#define NETWORK_DIAMETER        30             // hops

// This should be somewhat related to arp timeout
#define NODE_TRAVERSAL_TIME     0.03             // 30 ms

#define HELLO_INTERVAL          1               // 1000 ms
#define ALLOWED_HELLO_LOSS      3               // packets
#define BAD_LINK_LIFETIME       3               // 3000 ms

// Must be larger than the time difference between a node propagates a route request 
// and gets the route reply back.


// #define RREP_WAIT_TIME          (3 * NODE_TRAVERSAL_TIME * NETWORK_DIAMETER) // ms
//#define RREP_WAIT_TIME          (2 * REV_ROUTE_LIFE)  // seconds

#define RREP_WAIT_TIME         1.0  // sec

#define LOCAL_REPAIR_WAIT_TIME  0.15 //sec

#define RREQ_RETRIES            3  
// No. of times to do network-wide search before timing out for 
// MAX_RREQ_TIMEOUT sec. 

#define MAX_RREQ_TIMEOUT	10.0 //sec
// timeout after doing network-wide search RREQ_RETRIES times


#define MaxHelloInterval        (1.25 * HELLO_INTERVAL)
#define MinHelloInterval        (0.75 * HELLO_INTERVAL)

// The followings are used for the forward() function. Controls pacing.
#define DELAY 1.0           // random delay
#define NO_DELAY -1.0       // no delay 

// think it should be 30 ms
#define ARP_DELAY 0.01      // fixed delay to keep arp happy

/* Various constants used for the expanding ring search */
#define TTL_START     1
#define TTL_THRESHOLD 7
#define TTL_INCREMENT 2

/* =====================================================================
   Timers (Broadcast ID, Hello, Neighbor Cache, Route Cache)
   ===================================================================== */
class BroadcastTimer : public Handler {
public:
        BroadcastTimer(AODV* a) : agent(a) {}
        void	handle(Event*);
private:
        AODV    *agent;
	Event	intr;
};

class HelloTimer : public Handler {
public:
        HelloTimer(AODV* a) : agent(a) {}
        void	handle(Event*);
private:
        AODV    *agent;
	Event	intr;
};

class NeighborTimer : public Handler {
public:
        NeighborTimer(AODV* a) : agent(a) {}
        void	handle(Event*);
private:
        AODV    *agent;
	Event	intr;
};

class RouteCacheTimer : public Handler {
public:
        RouteCacheTimer(AODV* a) : agent(a) {}
        void	handle(Event*);
private:
        AODV    *agent;
	Event	intr;
};

class LocalRepairTimer : public Handler {
public:
        LocalRepairTimer(AODV* a) : agent(a) {}
        void	handle(Event*);
private:
        AODV    *agent;
	Event	intr;
};

/* =====================================================================
   Broadcast ID Cache
   ===================================================================== */
class BroadcastID {
        friend class AODV;
 public:

        BroadcastID(nsaddr_t i, u_int32_t b) { src = i; id = b;  }

 protected:
        LIST_ENTRY(BroadcastID) link;
        nsaddr_t        src;
        u_int32_t       id;
        double          expire;         // now + BCAST_ID_SAVE ms
};

LIST_HEAD(bcache, BroadcastID);


/* =====================================================================
   The Routing Agent
   ===================================================================== */
class AODV: public Agent {

  /*
   * make some friends first 
   */

        friend class rt_entry;
        friend class BroadcastTimer;
        friend class HelloTimer;
        friend class NeighborTimer;
        friend class RouteCacheTimer;
        friend class LocalRepairTimer;
 public:
        AODV(nsaddr_t id);

        void		recv(Packet *p, Handler *);

        /*
         * HDR offsets
         */
        int             off_AODV_;

 protected:
        int             command(int, const char *const *);
        int             initialized() { return 1 && target_; }

        /*
         * Route Table Management
         */
        void            rt_resolve(Packet *p);
        void            rt_down(rt_entry *rt);
        void            local_rt_repair(rt_entry *rt, Packet *p);
 public:
        void            rt_ll_failed(Packet *p);
 protected:
        void            rt_purge(void);

        void            enque(rt_entry *rt, Packet *p);
        Packet*         deque(rt_entry *rt);

        /*
         * Neighbor Management
         */
        void            nb_insert(nsaddr_t id);
        Neighbor*       nb_lookup(nsaddr_t id);
        void            nb_delete(nsaddr_t id);
        void            nb_purge(void);

        /*
         * Broadcast ID Management
         */

        void            id_insert(nsaddr_t id, u_int32_t bid);
        u_int32_t       id_lookup(nsaddr_t id, u_int32_t bid);

        void            id_purge(void);

        /*
         * Packet TX Routines
         */
        void            forward(rt_entry *rt, Packet *p, double delay);
        void            sendHello(void);
        void            sendRequest(nsaddr_t dst);
        void            sendReply(nsaddr_t ipdst, u_int32_t hop_count,
                                  nsaddr_t rpdst, u_int32_t rpseq,
                                  u_int32_t lifetime, double timestamp);
        void            sendTriggeredReply(nsaddr_t ipdst,
								nsaddr_t rpdst, u_int32_t rqseq);
		// Overloaded to have the route error broadcast case
        void            sendTriggeredReply(nsaddr_t rpdst, u_int32_t rqseq);
                                          

        /*
         * Packet RX Routines
         */
        void            recvAODV(Packet *p);
        void            recvHello(Packet *p);
        void            recvRequest(Packet *p);
        void            recvReply(Packet *p);
        void            recvTriggeredReply(Packet *p);

	/*
	 * History management
	 */
	
	double 		PerHopTime(rt_entry *rt);


        /* ============================================================ */

        nsaddr_t        index;                  // IP Address of this node
        int             seqno;                  // Sequence Number
        int             bid;                    // Broadcast ID

        rttable         rthead;                 // routing table
        ncache          nbhead;                 // Neighbor Cache
        bcache          bihead;                 // Broadcast ID Cache

        /*
         * Timers
         */
        BroadcastTimer  btimer;
        HelloTimer      htimer;
        NeighborTimer   ntimer;
        RouteCacheTimer rtimer;
        LocalRepairTimer lrtimer;


        /*
         * Routing Table
         */
        rttable          rtable;
        /*
         *  A "drop-front" queue used by the routing layer to buffer
         *  packets to which it does not have a route.
         */
        rtqueue         rqueue;

        /*
         * A mechanism for logging the contents of the routing
         * table.
         */
        Trace           *logtarget;

        /*
         * A pointer to the network interface queue that sits
         * between the "classifier" and the "link layer".
         */
        PriQueue        *ifqueue;

        /*
         * Logging stuff
         */
        void            log_link_del(nsaddr_t dst);
        void            log_link_broke(Packet *p);
        void            log_link_kept(nsaddr_t dst);
};

#endif /* __aodv_h__ */
