// Author: Satish Kumar, kkumar@isi.edu

extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "landmark.h"
#include <random.h>
#include <cmu-trace.h>
#include <address.h>

#define LM_STARTUP_JITTER 10.0 // secs to jitter LM's periodic adverts
#define IP_HDR_SIZE 20 // 20 byte IP header as in the Internet
#define WAIT_TIME 10 // 5s for each hop; round-trip is 2 x 5 = 10
#define MAX_TIMEOUT 2000 // Value divided by local population density to 
                      // compute promotion timeout at node
#define CONST_TIMEOUT 500 // Constant portion of timeout across levels
#define UPDATE_PERIOD 40 // Period of LM advertisements; Have to bind the
// variable in ParentChildrenList to Tcl variable
#define PROMO_TIMEOUT_MULTIPLES 40 // Used in promotion timer

// Returns a random number between 0 and max
static inline double 
jitter (double max, int be_random_)
{
  return (be_random_ ? Random::uniform(max) : 0);
}


void LandmarkAgent::
trace (char *fmt,...)
{
  va_list ap; // Define a variable ap that will refer to each argument in turn

  if (!tracetarget_)
    return;

  // Initializes ap to first argument
  va_start (ap, fmt); 
  // Prints the elements in turn
  vsprintf (tracetarget_->buffer (), fmt, ap); 
  tracetarget_->dump ();
  // Does the necessary clean-up before returning
  va_end (ap); 
}


static class LandmarkClass:public TclClass
{
  public:
  LandmarkClass ():TclClass ("Agent/landmark")
  {
  }
  TclObject *create (int, const char *const *)
  {
    return (new LandmarkAgent ());
  }
} class_landmark;




LandmarkAgent::LandmarkAgent (): Agent(PT_MESSAGE), promo_start_time_(0), promo_timeout_(50), promo_timeout_decr_(1), promo_timer_running_(0), seqno_(0), highest_level_(0), parent_children_list_(NULL), ll_queue(0), be_random_(1), wait_state_(0), total_wait_time_(0), debug_(1) 
 {

  promo_timer_ = new PromotionTimer(this);

  //  bind_time ("promo_timeout_", "&promo_timeout_");
  num_resched_ = 0;

  bind ("be_random_", &be_random_);
  //  bind ("myaddr_", &myaddr_);
  bind ("debug_", &debug_);
}




int
ParentChildrenList::UpdatePotlParent(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int delete_flag)
{
  LMNode *potl_parent, *list_ptr;
  double now = Scheduler::instance().clock();

  // cannot delete from an empty list!
  if(delete_flag) assert(pparent_);

  if((a_->debug_) && (a_->myaddr_ == 24)) {
    a_->trace("Updating Potl Parent level %d, id %d, delete_flag %d, time %f",level_,id,delete_flag,now);
  }

  if(pparent_ == NULL) {
    pparent_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
    parent_ = pparent_;
    return(NEW_ENTRY);
  }
  
  list_ptr = pparent_;
  potl_parent = list_ptr;
  while(list_ptr != NULL) {
    if(list_ptr->id_ == id) {
      // Check if this is a old message floating around in the network
      if(list_ptr->last_upd_origin_time_ >= origin_time) {
	// Check if we got the old update on a shorter path
	if(list_ptr->num_hops_ > num_hops) {
	  list_ptr->next_hop_ = next_hop;
	  list_ptr->num_hops_ = num_hops;
	}
	return(OLD_MESSAGE); 
      }
      if(!delete_flag) {
	// Make this node as parent if it's closer than current parent
	if(parent_->num_hops_ > num_hops) {
	  parent_ = list_ptr;
	}
	list_ptr->next_hop_ = next_hop;
	list_ptr->num_hops_ = num_hops;
	list_ptr->level_ = level;
	list_ptr->energy_ = energy;
	list_ptr->last_upd_origin_time_ = origin_time;
	list_ptr->last_update_rcvd_ = Scheduler::instance().clock();
      }
      else { // delete the entry
	if(pparent_ == list_ptr)
	  pparent_ = list_ptr->next_;
	else
	  potl_parent->next_ = list_ptr->next_;

	if(parent_->id_ == list_ptr->id_)
	  assert(parent_ == list_ptr);

	// No parent if potl parent list is empty
	if(pparent_ == NULL) {
	  parent_ = NULL;
	}
	else if(parent_ == list_ptr) {
	// Select new parent if current parent is deleted and
	// potl parent is not empty; closest potl parent is new parent
	  LMNode *tmp = pparent_;
	  int best_num_hops = pparent_->num_hops_;
	  LMNode *best_parent = pparent_;
	  while(tmp != NULL) {
	    if(tmp->num_hops_ < best_num_hops) {
	      best_num_hops = tmp->num_hops_;
	      best_parent = tmp;
	    }
	    tmp = tmp->next_;
	  }
	  parent_ = best_parent;
	}
	delete list_ptr;
      }
      return(OLD_ENTRY);
    }
    potl_parent = list_ptr;
    list_ptr = list_ptr->next_;
  }

  // delete flag must be FALSE if we are here
  assert(!delete_flag);

  potl_parent->next_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
  // Make this node as parent if it's closer than current parent
  if(parent_->num_hops_ > num_hops) {
    parent_ = potl_parent->next_;
  }
  return(NEW_ENTRY);
}



int
ParentChildrenList::UpdatePotlChild(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int child_flag, int delete_flag)
{
  LMNode *potl_child, *list_ptr;
  double now = Scheduler::instance().clock();

  //  if(a_->debug_) printf("Node %d: Number of potl children %d",a_->myaddr_,num_potl_children_);

  // cannot delete from an empty list!
  if(delete_flag) assert(pchildren_);

  assert(num_potl_children_ >= 0);
  assert(num_children_ >= 0);

  if(pchildren_ == NULL) {
    pchildren_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
    pchildren_->child_flag_ = child_flag;
    if(child_flag == IS_CHILD) ++(num_children_);
    ++(num_potl_children_);
    return(NEW_ENTRY);
  }
  
  list_ptr = pchildren_;
  potl_child = list_ptr;
  while(list_ptr != NULL) {
    if(list_ptr->id_ == id) {
      // Check if this is a old message floating around in the network
      if(list_ptr->last_upd_origin_time_ >= origin_time) {
	// Check if we got the old update on a shorter path
	if(list_ptr->num_hops_ > num_hops) {
	  list_ptr->next_hop_ = next_hop;
	  list_ptr->num_hops_ = num_hops;
	}
	return(OLD_MESSAGE);
      }
      if(!delete_flag) {
	if((list_ptr->child_flag_ == NOT_CHILD) && (child_flag == IS_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  ++(num_children_);
	}
	else if((list_ptr->child_flag_ == IS_CHILD) && (child_flag == NOT_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  --(num_children_);
	}
	list_ptr->next_hop_ = next_hop;
	list_ptr->num_hops_ = num_hops;
	list_ptr->level_ = level;
	list_ptr->energy_ = energy;
	list_ptr->last_upd_origin_time_ = origin_time;
	list_ptr->last_update_rcvd_ = Scheduler::instance().clock();
      }
      else {
	if(pchildren_ == list_ptr)
	  pchildren_ = list_ptr->next_;
	else
	  potl_child->next_ = list_ptr->next_;

	if(list_ptr->child_flag_ == IS_CHILD) --num_children_;
	--num_potl_children_;
	delete list_ptr;
      }
      return(OLD_ENTRY);
    }
    potl_child = list_ptr;
    list_ptr = list_ptr->next_;
  }

  // delete flag must be FALSE if we are here
  assert(!delete_flag);

  potl_child->next_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
  (potl_child->next_)->child_flag_ = child_flag;
  if(child_flag == IS_CHILD) ++(num_children_);
  ++(num_potl_children_); 
  return(NEW_ENTRY);
}



void
LandmarkAgent::ProcessHierUpdate(Packet *p)
{
  hdr_ip *iph = (hdr_ip *) p->access(off_ip_);
  hdr_cmn *hdrc = HDR_CMN(p);
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  int origin_time = 0;
  unsigned char *walk;
  nsaddr_t origin_id, parent, next_hop;
  int i, level, vicinity_radius, num_hops, potl_parent_flag = FALSE;
  nsaddr_t *potl_children;
  int num_children = 0;
  int num_potl_children = 0;
  // Packet will have seqnum, level, vicinity radii, parent info
  // and possibly potential children (if the node is at level > 0)

  //  trace("Node %d: Processing Hierarchy Update Packet", myaddr_);
  //  if(debug_) printf("Node %d: Processing Hierarchy Update Packet", myaddr_);

  //  if(debug_)
  //    trace("Node %d: Received packet from %d with ttl %d", myaddr_,iph->src_,iph->ttl_);

  walk = p->accessdata();

  // Originator of the LM advertisement and the next hop to reach originator
  origin_id = iph->src_;    

  // Free and return if we are seeing our own packet again
  if(origin_id == myaddr_) {
    Packet::free(p);
    return;
  }

  // level of the originator
  level = *walk++;

  //  if((myaddr_ == 30) && (origin_id == 10)) {
  //    trace("Node 30: Receiving level %d update from node 10 at time %f",level,s.clock());
  //  }

  // next hop info
  next_hop = *walk++;
  next_hop = (next_hop << 8) | *walk;

  // Change next hop to ourselves for the outgoing packet
  --walk;
  (*walk++) = (myaddr_ >> 8) & 0xFF;
  (*walk++) = (myaddr_) & 0xFF;
  
  // vicinity radius
  vicinity_radius = *walk++;
  vicinity_radius = (vicinity_radius << 8) | *walk++;
  // number of hops away from advertising LM
  num_hops = vicinity_radius - (iph->ttl_ - 1);

  // origin time of advertisement
  origin_time = *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;

  assert(origin_time < now);

  // Parent of the originator
  parent = *walk++;
  parent = parent << 8 | *walk++;

  if(level > 0) {
    // Number of children
    num_children = *walk++;
    num_children = num_children << 8 | *walk++;

    // Number of potential children
    num_potl_children = *walk++;
    num_potl_children = num_potl_children << 8 | *walk++;

  // If level of advertising LM > 1, check if we are in potl children list.
  // If so, add as potl parent to level - 1

    if(num_potl_children) {
      potl_children = new nsaddr_t[num_potl_children];
      for(i = 0; i < num_potl_children; ++i) {
	potl_children[i] = *walk++;
	potl_children[i] = potl_children[i] << 8 | *walk++;
	if(potl_children[i] == myaddr_) {
	  potl_parent_flag = TRUE;
	}
      }
    }
  }


  ParentChildrenList **pcl1 = NULL;
  ParentChildrenList **pcl2 = NULL;
  int found1 = FALSE;
  int found2 = FALSE;
  ParentChildrenList **pcl = &parent_children_list_;

  // Insert parent-child objects for levels: level-1 (if level > 0) & level + 1
  while((*pcl) != NULL) {
    if((*pcl)->level_ == (level-1)) {
      found1 = TRUE;
      pcl1 = pcl;
    }
    if((*pcl)->level_ == (level+1)) {
      found2 = TRUE;
      pcl2 = pcl;
    }
    pcl = &((*pcl)->next_);
  }


  // check if level > 0
  if(!found1 && level) {
    *pcl = new ParentChildrenList(level-1, this);
    pcl1 = pcl;
    pcl = &((*pcl)->next_);
  }

  if(!found2) {
    *pcl = new ParentChildrenList(level+1, this);
    pcl2 = pcl;
    pcl = &((*pcl)->next_);
  }

  // If level is same as our level, we can decrease the promotion timer 
  // if it's running provided we havent already heard advertisements from
  // this node

  int energy = 0; // Ignoring energy for now
  int delete_flag = FALSE; // Add the child/potl parent entry
  int child_flag = NOT_CHILD; // Indicates whether this node is our child
  if(parent == myaddr_) child_flag = IS_CHILD;
  int ret_value = (*pcl2)->UpdatePotlChild(origin_id, next_hop, num_hops, level, energy, origin_time, child_flag, delete_flag);
  
  // Free packet and return if we have seen this packet before
  if(ret_value == OLD_MESSAGE) {
    Packet::free(p);
    return;
  }
  

  // If promotion timer is running, decrement by constant amount
  if((ret_value != OLD_ENTRY) && (level == highest_level_)) {

    if(promo_timer_running_ && wait_state_) {
      ++(num_resched_);
    }
    else if(promo_timer_running_) {
      ++(num_resched_);
      double resched_time = promo_timeout_ - (now - promo_start_time_) - (num_resched_ * promo_timeout_decr_);
      if(resched_time < 0) resched_time = 0;
      //      trace("Node %d: Rescheduling timer, now %f, st %f, decr %f, resch %f\n", myaddr_, now, promo_start_time_, promo_timeout_decr_, resched_time);
      promo_timer_->resched(resched_time);
    }
  }

  
  // If the advertising LM is a potl parent, add to level-1 
  // ParentChildrenList object
  if(potl_parent_flag) {
    (*pcl1)->UpdatePotlParent(origin_id, next_hop, num_hops, level, energy, origin_time, FALSE);

  // If a parent has been selected for highest_level_, cancel promotion timer
  // (for promotion to highest_level_+1) if it's running
    if((level == (highest_level_ + 1)) && ((*pcl1)->parent_ != NULL)) {
      if(promo_timer_running_) {
	trace("Node %d: Promotion timer cancelled\n",myaddr_);
	promo_timer_->cancel();
	promo_timer_running_ = 0;
      }
    }
    else if(level == highest_level_) {
      // Check if the potl parent for highest_level_-1 that we see covers our
      // potential children. If so, we can demote ourselves and cancel our 
      // current promotion timer
      pcl = &parent_children_list_;
      while((*pcl) != NULL) {
	if((*pcl)->level_ == level) {
	  break;
	}
	pcl = &((*pcl)->next_);
      }
      assert(*pcl);

      LMNode *lm = (*pcl)->pchildren_;
      int is_subset = TRUE;
      if((*pcl)->num_potl_children_ > num_potl_children) {
	is_subset = FALSE;
	lm = NULL;
      }

      int is_element = FALSE;
      while(lm) {
	is_element = FALSE;
	for(i = 0; i < num_potl_children; ++i) 
	  if(lm->id_ == potl_children[i]) {
	    is_element = TRUE;
	    break;
	  }
	if(is_element == FALSE) {
	  is_subset = FALSE;
	  break;
	}
	lm = lm->next_;
      }
      if(is_subset == TRUE) {
    	--(highest_level_);
    	delete_flag = TRUE;
	if((*pcl1)->parent_)
	  trace("Node %d: Num potl ch %d, Node %d: Num potl ch %d",myaddr_, (*pcl)->num_potl_children_,origin_id,num_potl_children);
	  trace("Node %d: Parent before demotion: %d, msg from %d",myaddr_, ((*pcl1)->parent_)->id_,origin_id);
    	(*pcl1)->UpdatePotlParent(myaddr_, 0, 0, 0, 0, (int)now + 1, delete_flag);
	if((*pcl1)->parent_)
	  trace("Node %d: Parent after demotion: %d",myaddr_, ((*pcl1)->parent_)->id_);
    	(*pcl2)->UpdatePotlChild(myaddr_, 0, 0, 0, 0, (int)now + 1, IS_CHILD, delete_flag);
    	if(promo_timer_running_) {
    	  trace("Node %d: Promotion timer cancelled due to demotion at time %d\n",myaddr_,(int)now);
    	  promo_timer_->cancel();
    	  promo_timer_running_ = 0;
    	}
      }
    }      
  }

  if(num_potl_children) delete[] potl_children;

  // Do not forward packet if ttl is lower
  if(--iph->ttl_ == 0) {
    //    drop(p, DROP_RTR_TTL);
    Packet::free(p);
    return;
  }
  
  // Need to send the packet down the stack
  hdrc->direction() = -1;
  
  //  if(debug_) printf("Node %d: Forwarding Hierarchy Update Packet", myaddr_);
  s.schedule(target_, p, now);
}



void
LandmarkAgent::periodic_callback(Event *e, int level)
{
  //  if(debug_) printf("Periodic Callback for level %d", level);
  Scheduler &s = Scheduler::instance();

  // Should always have atleast the level 0 object
  assert(parent_children_list_ != NULL); 
  ParentChildrenList *pcl = parent_children_list_;
  ParentChildrenList *cur_pcl = NULL;
  ParentChildrenList *new_pcl = NULL;
  ParentChildrenList *pcl1 = NULL;
  ParentChildrenList *pcl2 = NULL;

  // return if we have been demoted from this level
  if(highest_level_ < level) 
    return; 
  
  while(pcl != NULL) {
    new_pcl = pcl;
    if(pcl->level_ == level){
      cur_pcl = pcl;
    }
    if(pcl->level_ == (level - 1)){
      pcl1 = pcl;
    }
    if(pcl->level_ == (level + 1)){
      pcl2 = pcl;
    }
    pcl = pcl->next_;
  }
  
  assert(cur_pcl);
  if(level)
    assert(pcl1);

  // Create level+1 object if it doesnt exist
  if(!pcl2) {
    new_pcl->next_ = new ParentChildrenList(level+1, this);
    pcl2 = new_pcl->next_;
  }

  assert(pcl2);

  double now = Scheduler::instance().clock();
  if(level)
    pcl1->UpdatePotlParent(myaddr_, myaddr_, 0, level, 0, (int)now, FALSE);
  pcl2->UpdatePotlChild(myaddr_, myaddr_, 0, level, 0, (int)now, IS_CHILD, FALSE);

  //  if(((now - cur_pcl->last_update_sent_) >= cur_pcl->update_period_) || (cur_pcl->last_update_sent_ == 0)) {
  //    if(debug_ && highest_level_) printf("Node %d: Gen Hier Upd Pkt for level %d, time %f\n", myaddr_, cur_pcl->level_, now);
    Packet *p = makeUpdate(cur_pcl, HIER_ADVS);
    s.schedule(target_, p, now);
    //  }
  
  cur_pcl->last_update_sent_ = now;

  // Delete stale potential parent entries
  LMNode *lmnode = cur_pcl->pparent_;
  int delete_flag = TRUE;
  while(lmnode) {
    if(((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_)) {
      //      if(debug_) trace("Node %d: Deleting stale entry for %d at time %d",myaddr_,lmnode->id_,(int)now);
      cur_pcl->UpdatePotlParent(lmnode->id_, 0, 0, 0, 0, (int)now + 1, delete_flag);
    }
    lmnode = lmnode->next_;
  }
  
  // A LM at level 3 is also at levels 0, 1 and 2. For each of these levels,
  // the LM designates itself as parent. At any given instant, only the 
  // level 3 (i.e., highest_level_) LM may not have a parent and may need to 
  // promote itself. But if the promotion timer is running, then the election
  // process for the next level has already begun.

  if((cur_pcl->parent_ == NULL) && (!promo_timer_running_)) {
    // Start off promotion process i.e., start 'wait period' first
    //    if (debug_) printf("PeriodicCallback method: Scheduling timer\n");
    promo_timer_running_ = 1;
    promo_start_time_ = s.clock();

    wait_state_ = 1;

    // Linearly increase promotion timeout and timeout decrements with level
    //    promo_timeout_ += promo_timeout_;
    // promo_timeout_decr_ += promo_timeout_decr_;

    num_resched_ = 0;

    trace("Node %d: Entering wait state\n", myaddr_);
    double wait_time = WAIT_TIME * radius(highest_level_) + UPDATE_PERIOD;
    total_wait_time_ += wait_time;
    promo_timer_->sched(wait_time);
  }

  // Delete stale potential children entries
  lmnode = cur_pcl->pchildren_;
  delete_flag = TRUE;
  while(lmnode) {
    if((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_)
      cur_pcl->UpdatePotlChild(lmnode->id_, 0, 0, 0, 0, (int)now + 1, IS_CHILD, delete_flag);
    lmnode = lmnode->next_;
  }

  // Need to later add demotion criteria. If no children after n update rounds
  // demote ourselves. This is so that n*update_period_ time is available to
  // get back "I'm your child" messages from other nodes. This period will have
  // to be lesser than promo_timeout_ though. Need to modify expire function
  // to stop promotion if the node has already been demoted (can use an addl
  // variable in LMAgent to indicate this?)


  s.schedule(cur_pcl->periodic_handler_, cur_pcl->periodic_update_event_, cur_pcl->update_period_ + jitter(LM_STARTUP_JITTER, be_random_));
}



Packet *
LandmarkAgent::makeUpdate(ParentChildrenList *pcl, int pkt_type)
{
  Packet *p = allocpkt();
  hdr_ip *iph = (hdr_ip *) p->access(off_ip_);
  hdr_cmn *hdrc = HDR_CMN(p);
  unsigned char *walk;

  hdrc->next_hop_ = IP_BROADCAST; // need to broadcast packet
  hdrc->addr_type_ = AF_INET;
  iph->dst_ = IP_BROADCAST;  // packet needs to be broadcast
  iph->dport_ = ROUTER_PORT;
  iph->ttl_ = radius(pcl->level_);

  iph->src_ = myaddr_;
  iph->sport_ = ROUTER_PORT;

  if(pkt_type == HIER_ADVS) {
    
    if(pcl->level_ == 0) {
      // No children for level 0 LM
      // totally 11 bytes in packet for now; need to add our energy level later
      p->allocdata(11); 
      walk = p->accessdata();

      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // make ourselves as next hop; 2 bytes
      (*walk++) = (myaddr_ >> 8) & 0xFF;
      (*walk++) = (myaddr_) & 0xFF;

      // Vicinity size in number of hops; Carrying this information allows
      // different LMs at same level to have different vicinity radii; 2 bytes
      (*walk++) = (radius(pcl->level_) >> 8) & 0xFF;
      (*walk++) = (radius(pcl->level_)) & 0xFF;

      // Time at which packet was originated; 4 bytes (just integer portion)
      double now = Scheduler::instance().clock();
      int origin_time = (int)now;
      (*walk++) = (origin_time >> 24) & 0xFF;
      (*walk++) = (origin_time >> 16) & 0xFF;
      (*walk++) = (origin_time >> 8) & 0xFF;
      (*walk++) = (origin_time) & 0xFF;

      // Parent ID; 2 bytes
      if(pcl->parent_ == NULL) {
	(*walk++) = (NO_PARENT >> 8) & 0xFF;
	(*walk++) = (NO_PARENT) & 0xFF;
      }
      else {
	(*walk++) = ((pcl->parent_)->id_ >> 8) & 0xFF;
	(*walk++) = ((pcl->parent_)->id_) & 0xFF;
      }
      // In real life each of the above fields would be 
      // 4 byte integers; 20 bytes for IP addr   
      hdrc->size_ = 20 + 20; 
    }
    else {
      // Need to list all the potential children LMs
      // Pkt size: 11 bytes (as before); 2 each for number of children 
      // and potl_children; 2 byte for each child's id
      int pkt_size = 11 + 4 + (2 * pcl->num_potl_children_);
      p->allocdata(pkt_size);
      walk = p->accessdata();
      
      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // make ourselves as next hop; 2 bytes
      (*walk++) = (myaddr_ >> 8) & 0xFF;
      (*walk++) = (myaddr_) & 0xFF;

      // Vicinity size in number of hops; 2 bytes
      (*walk++) = (radius(pcl->level_) >> 8) & 0xFF;
      (*walk++) = (radius(pcl->level_)) & 0xFF;

      // Time at which packet was originated; 4 bytes (only integer portion)
      double now = Scheduler::instance().clock();
      int origin_time = (int)now;
      (*walk++) = (origin_time >> 24) & 0xFF;
      (*walk++) = (origin_time >> 16) & 0xFF;
      (*walk++) = (origin_time >> 8) & 0xFF;
      (*walk++) = (origin_time) & 0xFF;

      if(origin_time > now) {
	printf("Node %d: id %d, level %d, vicinity_radius %d",myaddr_,myaddr_,pcl->level_,radius(pcl->level_));
	assert(origin_time < now);
      }

      // Parent's id; 2 bytes
      if(pcl->parent_ == NULL) {
	(*walk++) = (NO_PARENT >> 8) & 0xFF;
	(*walk++) = (NO_PARENT) & 0xFF;
      }
      else {
	(*walk++) = ((pcl->parent_)->id_ >> 8) & 0xFF;
	(*walk++) = ((pcl->parent_)->id_) & 0xFF;
      }

      // Number of children; 2 bytes
      (*walk++) = (pcl->num_children_ >> 8) & 0xFF;
      (*walk++) = (pcl->num_children_) & 0xFF;

      // Number of potential children; 2 bytes
      (*walk++) = (pcl->num_potl_children_ >> 8) & 0xFF;
      (*walk++) = (pcl->num_potl_children_) & 0xFF;

      LMNode *potl_ch = pcl->pchildren_;
      while(potl_ch) {
	(*walk++) = (potl_ch->id_ >> 8) & 0xFF;
	(*walk++) = (potl_ch->id_) & 0xFF;
	potl_ch = potl_ch->next_;
      }
      hdrc->size_ = 20 + 8 + (4 * pcl->num_potl_children_) + 20; 
      // In real life each of the above fields would be
      // 4 byte integers; 20 bytes for IP addr
    }
  }

  hdrc->direction() = -1;
  return p;
}



int
LandmarkAgent::radius(int level)
{
  return(int(pow(2,level+1)));
}




ParentChildrenList::ParentChildrenList(int level, LandmarkAgent *a) : parent_(NULL), num_children_(0), num_potl_children_(0), pchildren_(NULL), pparent_(NULL) , seqnum_(0) ,last_update_sent_(0), update_period_(UPDATE_PERIOD), update_timeout_(10*UPDATE_PERIOD), next_(NULL)
{
  level_ = level;
  periodic_update_event_ = new Event;
  periodic_handler_ = new LMPeriodicAdvtHandler(this);
  a_ = a;
}




void
PromotionTimer::expire(Event *e) {

  ParentChildrenList *pcl = a_->parent_children_list_;
  ParentChildrenList *new_pcl, *higher_level_pcl = NULL;
  ParentChildrenList *pcl1 = NULL; // Pointer to new highest_level_-1 object
  ParentChildrenList *pcl2 = NULL; // Pointer to new highest_level_+1 object
  int found = FALSE;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();

  if(a_->wait_state_) {
    if(a_->num_resched_) {
      a_->wait_state_ = 0;
      a_->total_wait_time_ = 0;
      
      // Promotion timeout is num_resched_ times the estimated time for 
      // a message to reach other nodes in its vicinity
      a_->promo_timeout_ = jitter(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_) + MAX_TIMEOUT/(a_->num_resched_ * (a_->highest_level_+1))), a_->be_random_);
      a_->trace("Node %d: Promotion timeout after wait period: %f", a_->myaddr_,a_->promo_timeout_);
      a_->num_resched_ = 0;
      a_->promo_start_time_ = s.clock();
      a_->promo_timer_->resched(a_->promo_timeout_);
    }
    else {
      double wait_time = WAIT_TIME * a_->radius(a_->highest_level_) + UPDATE_PERIOD;
      //      a_->trace("Node %d: Renewing wait period", a_->myaddr_);
      a_->total_wait_time_ += wait_time;
      //      if((a_->total_wait_time_ > DEMOTION_TIMEOUT) && (a_->
      a_->promo_timer_->resched(wait_time);
    }
    return;
  }    

  // Promotion timer is off
  a_->promo_timer_running_ = 0;

  // Only one promotion timer can be running at a node at a given instant. 
  // On expiry, the node will be promoted one level higher to highest_level_+1
  // Add a parentchildrenlist object for the higher level if one doesnt already
  // exist

  while(pcl != NULL) {
    new_pcl = pcl;
    if(pcl->level_ == a_->highest_level_+1){
      found = TRUE;
      higher_level_pcl = pcl;
    }
    if(pcl->level_ == (a_->highest_level_)){
      pcl1 = pcl;
    }
    if(pcl->level_ == (a_->highest_level_+2)){
      pcl2 = pcl;
    }
    pcl = pcl->next_;
  }
  
  // highest_level_-1 object should exist
  assert(pcl1);

  if(pcl1->parent_ != NULL) {
    a_->trace("Node %d: Not promoted to higher level %d\n", a_->myaddr_, a_->highest_level_+1);
    return;
  }

  ++(a_->highest_level_);

  if(!found) {
    new_pcl->next_ = new ParentChildrenList(a_->highest_level_, a_);
    higher_level_pcl = new_pcl->next_;
    new_pcl = new_pcl->next_;
  }

  // Create highest_level_+1 object if it doesnt exist
  if(!pcl2) {
    new_pcl->next_ = new ParentChildrenList((a_->highest_level_)+1, a_);
    pcl2 = new_pcl->next_;
  }

  assert(pcl2);

  if(a_->debug_) {
    a_->trace("Node %d: Promoted to level %d, num_resched %d at time %f\n", a_->myaddr_, a_->highest_level_, a_->num_resched_, now);
    LMNode *lm = higher_level_pcl->pchildren_;
    a_->trace("Potential Children:");
    while(lm) {
      a_->trace("%d (level %d)", lm->id_,lm->level_);
      lm = lm->next_;
    }

    lm = higher_level_pcl->pparent_;
    a_->trace("Potential Parent:");
    while(lm) {
      a_->trace("%d (level %d)", lm->id_,lm->level_);
      lm = lm->next_;
    }
  }
  
  // start off periodic advertisements for this higher level
  s.schedule(higher_level_pcl->periodic_handler_, higher_level_pcl->periodic_update_event_, jitter(LM_STARTUP_JITTER, a_->be_random_));
  
  // add myself as potential parent for highest_level-1 and child for 
  // highest_level+1 
  int num_hops = 0;
  int energy = 0;
  int child_flag = IS_CHILD;
  int delete_flag = FALSE;
  pcl1->UpdatePotlParent(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_, energy, (int)now, delete_flag);
  pcl2->UpdatePotlChild(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_, energy, (int)now, child_flag, delete_flag);
  
  // If we havent seen a LM that can be our parent at this higher level, start
  // promotion timer for promotion to next level
  //  if((higher_level_pcl->parent_ == NULL) && (!(a_->promo_timer_running_))) {
    //    if (a_->debug_) printf("PromotionTimer's expire method: Scheduling timer for promo to level %d\n",a_->highest_level_);
    // Timer's status is TIMER_HANDLING in handle; so we just reschedule to
    // avoid "cant start timer" abort if sched is called
  //    a_->promo_timer_running_ = 1;
  //    a_->wait_state_ = 1;

    // Linearly increase promotion timeout and timeout decrements with level
    //    a_->promo_timeout_ += a_->promo_timeout_;
    //    a_->promo_timeout_decr_ += a_->promo_timeout_decr_;

  //    double wait_time = 5 * a_->radius(a_->highest_level_) + 10;
  //    a_->num_resched_ = 0;
  //  a_->promo_timer_->resched(wait_time);
  //  }
}



  

int 
LandmarkAgent::command (int argc, const char *const *argv)
{
  if (argc == 2)
    {
      if (strcmp (argv[1], "start-landmark") == 0)
	{
	  startUp();
	  return (TCL_OK);
	}
      else if (strcmp (argv[1], "dumprtab") == 0)
	{
	  Packet *p2 = allocpkt ();
	  hdr_ip *iph2 = (hdr_ip *) p2->access (off_ip_);
	  //	  rtable_ent *prte;

          trace ("Table Dump %d[%d]\n----------------------------------\n",
	    myaddr_, iph2->sport_);
	  trace ("VTD %.5f %d:%d\n", Scheduler::instance ().clock (),
		 myaddr_, iph2->sport_);

	  /*
	   * Freeing a routing layer packet --> don't need to
	   * call drop here.
	   */
	  trace("Highest Level: %d", highest_level_);
	  Packet::free (p2);
	  ParentChildrenList *pcl = parent_children_list_;
	  while(pcl) {
	    trace("Level %d:", pcl->level_);
	    if(pcl->parent_)
	      trace("Parent: %d", (pcl->parent_)->id_);
	    else
	      trace("Parent: NULL");
	    trace("Number of children: %d\n", pcl->num_children_);
	    pcl = pcl->next_;
	  }

	//	p2 = 0;
	//	  for (table_->InitLoop (); (prte = table_->NextLoop ());)
	//	    output_rte ("\t", prte, this);

	//	  printf ("\n");

	  return (TCL_OK);
	}
      else if (strcasecmp (argv[1], "ll-queue") == 0)
	{
	if (!(ll_queue = (PriQueue *) TclObject::lookup (argv[2])))
	    {
	      fprintf (stderr, "DSDV_Agent: ll-queue lookup of %s failed\n", argv[2]);
	      return TCL_ERROR;
	    }

	  return TCL_OK;
	}
    }
  else if (argc == 3)
    {
      if (strcasecmp (argv[1], "tracetarget") == 0)
	{
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  tracetarget_ = (Trace *) obj;
	  return TCL_OK;
	}
      else if (strcasecmp (argv[1], "addr") == 0) {
	int temp;
	temp = Address::instance().str2addr(argv[2]);
	myaddr_ = temp;
	return TCL_OK;
      }
    }

return (Agent::command (argc, argv));
}




void
LandmarkAgent::startUp() {

  int i;
  Scheduler &s = Scheduler::instance();

  assert(highest_level_ >= 0);
  assert(parent_children_list_ == NULL);
  parent_children_list_ = new ParentChildrenList(i, this);
  ParentChildrenList **pcl = &parent_children_list_;

  // Start off periodic LM advertisements
  for ( i = 0; i <= highest_level_; ++i) {
    *pcl = new ParentChildrenList(i, this);
    s.schedule((*pcl)->periodic_handler_, (*pcl)->periodic_update_event_, jitter(LM_STARTUP_JITTER, be_random_));
    (*pcl)->next_ = NULL;
    pcl = &((*pcl)->next_);
  }
  
  // Start off promotion timer
  //  if (debug_) printf("startUp: Scheduling timer\n");
  promo_timer_running_ = 1;
  //  promo_start_time_ = s.clock();
  num_resched_ = 0;

  // Node enters "wait" state where it waits to receive other node's 
  // advertisements; Wait for 5 * radius(level) seconds; Should be
  // atleast the same as the period of LM advts (10s)
  wait_state_ = 1;
  double wait_time = WAIT_TIME * radius(highest_level_) + UPDATE_PERIOD;
  total_wait_time += wait_time;
  if(debug_) trace("Node %d: Wait time: %f\n",myaddr_,wait_time);
  promo_timer_->sched(wait_time);

  //  promo_timer_->sched(promo_timeout_);
}




void
LandmarkAgent::recv(Packet *p, Handler *)
{
  hdr_ip *iph = (hdr_ip *) p->access (off_ip_);
  hdr_cmn *cmh = (hdr_cmn *) p->access (off_cmn_);

  /*
   *  Must be a packet being originated from some other agent at my node?
   */
  if(iph->src_ == myaddr_ && cmh->num_forwards() == 0) {
    /*
     * Add the IP Header
     */
    cmh->size() += IP_HDR_LEN;    
  }
  /*
   *  I received a packet that I sent.  Probably
   *  a routing loop.
   */
  else if(iph->src_ == myaddr_) {
    drop(p, DROP_RTR_ROUTE_LOOP);
    return;
  }
  /*
   *  Packet I'm forwarding...
   */

  // Move the ttl check to the following methods?
  //    if(--iph->ttl_ == 0) {
  //      drop(p, DROP_RTR_TTL);
  //      return;
  //    }
  
  // Packet will be forwarded down (if it's not dropped)
  cmh->direction_ = -1;
  
  if ((iph->src_ != myaddr_) && (iph->dport_ == ROUTER_PORT))
    {
      ProcessHierUpdate(p);
    }
  //  else
  //    {
  //      ForwardPacket(p);
  //    }
}

