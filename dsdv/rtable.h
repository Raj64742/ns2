/* rtable.h -*- c++ -*-
   $Id: rtable.h,v 1.1 1998/12/08 19:18:12 haldar Exp $
   */
#ifndef cmu_rtable_h_
#define cmu_rtable_h_

#include "config.h"
#include "scheduler.h"
#include "queue.h"

#define BIG   250

#define NEW_ROUTE_SUCCESS_NEWENT       0
#define NEW_ROUTE_SUCCESS_OLDENT       1
#define NEW_ROUTE_METRIC_TOO_HIGH      2
#define NEW_ROUTE_ILLEGAL_CANCELLATION 3
#define NEW_ROUTE_INTERNAL_ERROR       4

typedef unsigned int uint;

/* NOTE: we depend on bzero setting the booleans to ``false''
   but if false != 0, so many things are screwed up, I don't
   know what to say... */

class rtable_ent {
public:
  rtable_ent() { bzero(this, sizeof(struct rtable_ent));}
  nsaddr_t     dst;     // destination
  nsaddr_t     hop;     // next hop
  uint         metric;  // distance

  uint         seqnum;  // last sequence number we saw
  double       advertise_ok_at; // when is it okay to advertise this rt?
  bool         advert_seqnum;  // should we advert rte b/c of new seqnum?
  bool         advert_metric;  // should we advert rte b/c of new metric?

  /*

  bool          needs_advertised; // must this rte go in out in an update?
  bool          needs_repeated_advert; // must this rte go out in all updates
 		 		      // until the next periodic update?
	 		 	      */
  Event        *trigger_event;

  uint          last_advertised_metric; // metric carried in our last advert
  double       changed_at; // when route last changed
  double       new_seqnum_at;	// when we last heard a new seq number
  double       wst;     // running wst info
  Event       *timeout_event; // event used to schedule timeout action
  PacketQueue *q;		//pkts queued for dst
};

// AddEntry adds an entry to the routing table with metric ent->metric+em.
//   You get to free the goods.
//
// GetEntry gets the entry for an address.

class RoutingTable {
  public:
    RoutingTable();
    void AddEntry(const rtable_ent &ent);
    int RemainingLoop();
    void InitLoop();
    rtable_ent *NextLoop();
    rtable_ent *GetEntry(nsaddr_t dest);

  private:
    rtable_ent *rtab;
    int         maxelts;
    int         elts;
    int         ctr;
};
    
#endif
