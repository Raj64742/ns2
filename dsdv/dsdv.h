/* dsdv.h -*- c++ -*-
   $Id: dsdv.h,v 1.1 1998/12/08 19:18:11 haldar Exp $

   */

#ifndef cmu_dsdv_h_
#define cmu_dsdv_h_

#include <agent.h>
#include <ip.h>
#include <delay.h>
#include <scheduler.h>
#include <queue.h>
#include <trace.h>
#include <arp.h>
#include <new-ll.h>
#include <new-mac.h>
#include <priqueue.h>

#include "rtable.h"

typedef double Time;

#define MAX_QUEUE_LENGTH 5
#define ROUTER_PORT      0xff

class DSDV_Helper;
class DSDVTriggerHandler;

class DSDV_Agent : public Agent {
  friend class DSDV_Helper;
  friend class DSDVTriggerHandler;
public:
  DSDV_Agent();
  virtual int command(int argc, const char * const * argv);
  void lost_link(Packet *p);
  
protected:
  void helper_callback(Event *e);
  Packet* rtable(int);
  virtual void recv(Packet *, Handler *);
  void trace(char* fmt, ...);
  void tracepkt(Packet *, double, int, const char *);
  void needTriggeredUpdate(rtable_ent *prte, Time t);
  // if no triggered update already pending for route prte, make one so
  void cancelTriggersBefore(Time t);
  // Cancel any triggered events scheduled to take place *before* time
  // t (exclusive)
  Packet * makeUpdate(int& periodic);
  // return a packet advertising the state in the routing table
  // makes a full ``periodic'' update if requested, or a ``triggered''
  // partial update if there are only a few changes and full update otherwise
  // returns with periodic = 1 if full update returned, or = 0 if partial
  // update returned
  void updateRoute(struct rtable_ent *old_rte, struct rtable_ent *new_rte);
  void processUpdate (Packet * p);
  void forwardPacket (Packet * p);
  void startUp();
  
  // update old_rte in routing table to to new_rte
  Trace *tracetarget;       // Trace Target
  DSDV_Helper  *helper_;    // DSDV Helper, handles callbacks
  DSDVTriggerHandler *trigger_handler;
  RoutingTable *table_;     // Routing Table
  PriQueue *ll_queue;       // link level output queue
  int seqno_;               // Sequence number to advertise with...
  int myaddr_;              // My address...
  Event *periodic_callback_;           // notify for periodic update
  
  // Randomness/MAC/logging parameters
  int be_random_;
  int use_mac_;
  int verbose_;
  int trace_wst_;
  
  // last time a periodic update was sent...
  double lasttup_;		// time of last triggered update
  double next_tup;		// time of next triggered update
  //  Event *trigupd_scheduled;	// event marking a scheduled triggered update
  
  // DSDV constants:
  double alpha_;  // 0.875
  double wst0_;   // 6 (secs)
  double perup_;  // 15 (secs)  period between updates
  int    min_update_periods_;    // 3 we must hear an update from a neighbor
  // every min_update_periods or we declare
  // them unreachable
  
  void output_rte(const char *prefix, rtable_ent *prte, DSDV_Agent *a);
  
};

class DSDV_Helper : public Handler {
  public:
    DSDV_Helper(DSDV_Agent *a_) { a = a_; }
    virtual void handle(Event *e) { a->helper_callback(e); }

  private:
    DSDV_Agent *a;
};

#endif
