// Author: Satish Kumar, kkumar@isi.edu

#ifndef landmark_h_
#define landmark_h_

#include <agent.h>
#include <ip.h>
#include <delay.h>
#include <scheduler.h>
#include <queue.h>
#include <trace.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>
#include <priqueue.h>
#include <mobilenode.h>
#include "tags.h"


typedef double Time;

#define ROUTER_PORT 255

#define NOT_CHILD 0
#define IS_CHILD 1
#define NOT_POTL_CHILD 2 // Not within appropriate vicinity

#define NO_PARENT 60000

#define OLD_ENTRY 0     // Object already exists in list
#define NEW_ENTRY 1     // New object created in list
#define OLD_MESSAGE 2   // Old information; Object not added to list
#define ENTRY_NOT_FOUND 3 // Object not found
#define IGNORE_ENTRY 4 // Node is not potential child;
                       // returned by UpdatePotlChild

#define TRUE 1
#define FALSE 0

//class TagCache {
//public:
//  int obj_name_;
//  int origin_time_;
//  double X_;
//  double Y_;
//};



class NodeIDList {
public:
  NodeIDList() {
    next_ = NULL;
  }
  nsaddr_t dst_node_;
  nsaddr_t dst_next_hop_;
  int next_hop_level_;
  NodeIDList *next_;
};


class LMNode {
  friend class ParentChildrenList;
  friend class LandmarkAgent;
  friend class PromotionTimer;
public:
  LMNode(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, double update_time) 
  { 
    id_ = id;
    next_hop_ = next_hop;
    child_flag_ = NOT_CHILD;
    num_hops_ = num_hops;
    level_ = level;
    energy_ = energy;
    last_upd_origin_time_ = origin_time;
    last_update_rcvd_ = update_time;
    tag_list_ = NULL;
    next_ = NULL;
  }
  
  ~LMNode() {
    compr_taglist *tag_ptr1, *tag_ptr2;
    tag_ptr1 = tag_list_;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
  }

private:
  nsaddr_t id_;             // ID of this node
  nsaddr_t next_hop_;	    // Next hop to reach this node
  int child_flag_;	    // indicates whether this node is a child
  int num_hops_;            // number of hops away
  int level_;               // highest level of the child node
  int energy_;              // Energy reserves of the child node
  int last_upd_origin_time_; // Origin time of last update
  double last_update_rcvd_; // Time at which last update received
  compr_taglist *tag_list_;  // Tag advertisements of this node
  LMNode *next_;     
};



class LMEvent : public Event {
public:
  int level_;
  LMEvent(int level) : Event() {level_ = level;}
};


class ParentChildrenList {
  friend class LandmarkAgent;
  friend class LMNode;
  friend class PromotionTimer;
  friend class LMPeriodicAdvtHandler;
public:
  ParentChildrenList(int level, LandmarkAgent *a);
private:
  int level_;          // my level
  LMNode *parent_;    // points to the appropriate object in the pparent_ list
  int num_heard_;     // number of nodes heard at one level lower than 
                      // our level; superset of potential children heard
                      // with the "election" vicinity condition
  int num_children_;   // number of children
  int num_potl_children_; // number of potential children
  int num_pparent_;     // number of potential parents			       
  LMNode *pchildren_;  // list of children and potential children
  LMNode *pparent_;   // list of potential parents			    

  // Periodic advertisement stuff				       
  int seqnum_;          // Sequence number of last advertisement	   
  double last_update_sent_; // Time at which last update was sent 
  double update_period_;    // period between updates		 
  double update_timeout_;   // Updates are deleted after this timeout	 
  Event *periodic_update_event_;  // event used to schedule periodic 
                                    // landmark updates
  LMPeriodicAdvtHandler *periodic_handler_; // handler called by the scheduler

  ParentChildrenList *next_;  // pointer to next list element
  int update_round_;        // To be used for demotion

  // Update/add/delete info abt a potential parent or child
  // Returns 1 if parent or child already present else adds the relevant
  // object and returns 0
  // Deletes the specified parent or child if delete flag is set to 1;
  // should be set to 0 otherwise
  // One method might be enough but have two in case different
  // actions have to be taken for adding parent and child in the future
  int UpdatePotlParent(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int delete_flag);
  int UpdatePotlChild(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int child_flag, int delete_flag, compr_taglist *taglist);

  LandmarkAgent *a_; // Agent associated with this object
  compr_taglist *tag_list_; // Aggregated list of tags for each level	     
  int num_tags_;            // Number of tags in tag_list	       

  int adverts_type_;  // Indicates whether adverts should be flooded, unicast
                  // or suppresed when no changes as in a hard-state scheme
};


#define HIER_ADVS 0
#define OBJECT_ADVS 1
#define HIER_AND_OBJECT_ADVS 2

class LandmarkAgent : public Agent {
  friend class LMPeriodicAdvtHandler;
  friend class PromotionTimer;
  friend class ParentChildrenList;
public:
  LandmarkAgent();
  virtual int command(int argc, const char * const * argv);
  
protected:
  //  RoutingTable *table_;     // Routing Table

  // Promotion timer stuff
  PromotionTimer *promo_timer_; // Promotion timer object
  double promo_start_time_;     // Time when the promotion timer was started
  double promo_timeout_;	// Promotion timeout. Same for all levels.
  double promo_timeout_decr_; // Amount by which promotion timer is
                               // decr when another LM's adverts is heard
  int promo_timer_running_;	// indicates that promotion timer is running

  void startUp();           // Starts off the hierarchy construction protocol
  int seqno_;               // Sequence number to advertise with...
  int myaddr_;              // My address...

  // Periodic advertisements stuff				

  void periodic_callback(Event *e, int level); // method to send periodic advts
  
  int highest_level_;       // My highest level in the hierarchy (note
                            // that a LM can be at multiple levels)
  // List of parent and children nodes for each level I'm at. Methods to add
  // and remove parent, child information from this list.
  ParentChildrenList *parent_children_list_;
  void Addparent(const nsaddr_t parent, int level);

  void Addpotentialchild(const nsaddr_t child, int level);

  // pkt_type is one of HIER_ADVS, OBJECT_ADVS, HIER_AND_OBJECT_ADVS
  // action is one of DEMOTION, PERIODIC_ADVERTS
  Packet *makeUpdate(ParentChildrenList *pcl, int pkt_type, int action); 
                                  
  int radius(int level); // returns the LM radius for the specified level

  PriQueue *ll_queue;       // link level output queue

  void recv(Packet *p, Handler *);
  void ProcessHierUpdate(Packet *p);
  void ForwardPacket(Packet *p);

  // Prints neighbour information for this node
  void get_nbrinfo();

  // Tracing stuff
  void trace(char* fmt,...);       
  Trace *tracetarget_;  // Trace target

  // Pointer to global tag database
  tags_database *tag_dbase_;
  compr_taglist *aggregate_taginfo(compr_taglist *unagg_tags, int agg_level, int *num_tags);
  NodeIDList *search_tag(int obj_name, int prev_hop_level, int next_hop_level, nsaddr_t last_hop_id, int *num_dst);
  nsaddr_t get_next_hop(nsaddr_t dst, int next_hop_level);

  // Mobile node to which agent is attached; Used to get position information
  MobileNode *node_;

  // Randomness/MAC/logging parameters
  int be_random_;    // set to 1 on initialization

  int num_resched_; // used in rescheduling timers
  int wait_state_;  // used to indicate that the node is waiting to receive
                    // other LM hierarchy messages
  double total_wait_time_; // total time the node has spent in wait state

  // Debug flags
  int debug_;	  
  int qry_debug_;

  // Tag cache info
  int cache_;      // set to 1 to enable caching
  TagCache *tag_cache_;
  int num_cached_items_;

  // Update period info
  double update_period_;
  double update_timeout_;

  // Option to indicate whether updates are to be flooded, unicast or
  // suppressed when no changes occur as in a hard-state based scheme
  int adverts_type_;  

  // Random number generator
  RNG *rn_;
  inline double jitter(double max, int be_random_);
  inline double random_timer(double max, int be_random_);
};



class LMPeriodicAdvtHandler : public Handler {
public:
  LMPeriodicAdvtHandler(ParentChildrenList *p) { p_ = p; }
  virtual void handle(Event *e) { (p_->a_)->periodic_callback(e,p_->level_); }
private:
  ParentChildrenList *p_;
};


class PromotionTimer : public TimerHandler {
public:
  PromotionTimer(LandmarkAgent *a) : TimerHandler() { a_ = a;}
protected:
  virtual void expire(Event *e);
  LandmarkAgent *a_;
};



#endif
