// Author: Satish Kumar, kkumar@isi.edu

extern "C" {
#include <stdarg.h>
#include <float.h>
};

#include "landmark.h"
#include <random.h>
#include <cmu-trace.h>
#include <address.h>

#define MAX_ENERGY 100000

#define INITIAL_WAIT_TIME 10.0
#define PERIODIC_WAIT_TIME 60.0
#define LM_STARTUP_JITTER 10.0 // secs to jitter LM's periodic adverts
#define IP_HDR_SIZE 20 // 20 byte IP header as in the Internet
#define WAIT_TIME 2 // 1s for each hop; round-trip is 2
#define MAX_TIMEOUT 2000 // Value divided by local population density and 
            // level to compute promotion timeout at node
#define CONST_TIMEOUT 100 // Constant portion of timeout across levels

#define LOW_FREQ_UPDATE 300
#define PROMO_TIMEOUT_MULTIPLES 5 // Used in promotion timer

#define DEMOTION 0    // update pkt should advertise demotion
#define PERIODIC_ADVERTS 1   // periodic update for each level node is at
#define UNICAST_ADVERT_CHILD 2
#define UNICAST_ADVERT_PARENT 3
#define QUERY_PKT 4        // query/response pkt

#define FLOOD 0
#define UNICAST 1
#define SUPPRESS 2


#define DEMOTION_RATIO 1.3
#define DEMOTION_DIFF  5000

#define NO_NEXT_HOP 50000
#define MAX_CACHE_ITEMS 200

static int lm_index_ = 0;



// Returns a random number between 0 and max
inline double 
LandmarkAgent::jitter(double max, int be_random_)
{
  return (be_random_ ? Random::uniform(max) : 0);
}



// Returns a random number between 0 and max
inline double 
LandmarkAgent::random_timer(double max, int be_random_)
{
  return (be_random_ ? rn_->uniform(max) : 0);
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




LandmarkAgent::LandmarkAgent (): Agent(PT_MESSAGE), promo_start_time_(0), promo_timeout_(50), promo_timeout_decr_(1), promo_timer_running_(0), seqno_(0), highest_level_(0), parent_children_list_(NULL), ll_queue(0), be_random_(1), wait_state_(0), total_wait_time_(0), debug_(1) ,qry_debug_(0)
 {

  promo_timer_ = new PromotionTimer(this);

  //  bind_time ("promo_timeout_", "&promo_timeout_");
  num_resched_ = 0;
  tag_dbase_ = NULL;
  node_ = NULL;

  cache_ = 0; // default is to disable caching
  tag_cache_ = new TagCache[MAX_CACHE_ITEMS];
  num_cached_items_ = 0;

  // default value for the update period
  update_period_ = 400;
  update_timeout_ = 2 * update_period_ + LM_STARTUP_JITTER;

  adverts_type_ = FLOOD;  // default is to flood adverts

  rn_ = new RNG(RNG::RAW_SEED_SOURCE,lm_index_++);;
  // Throw away a bunch of initial values
  for(int i = 0; i < 128; ++i) {
    rn_->uniform(200);
  }

  bind ("be_random_", &be_random_);
  //  bind ("myaddr_", &myaddr_);
  bind ("debug_", &debug_);
}




int
ParentChildrenList::UpdatePotlParent(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int delete_flag)
{
  LMNode *potl_parent, *list_ptr;
  double now = Scheduler::instance().clock();

  assert(num_pparent_ >= 0);

  // cannot delete from an empty list!
  if(delete_flag && !pparent_)
    return(ENTRY_NOT_FOUND);

  //  if((a_->debug_) && (a_->myaddr_ == 24)) {
  //    a_->trace("Node %d: Updating Potl Parent level %d, id %d, delete_flag %d, time %f",a_->myaddr_,level_,id,delete_flag,now);
  //  }

  if(pparent_ == NULL) {
    pparent_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
    parent_ = pparent_;
    ++num_pparent_;
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
	if(num_pparent_)
	  --(num_pparent_);

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

  if(delete_flag)
    return(ENTRY_NOT_FOUND);

  potl_parent->next_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
  ++num_pparent_;
  // Make this node as parent if it's closer than current parent
  if(parent_->num_hops_ > num_hops) {
    parent_ = potl_parent->next_;
  }
  return(NEW_ENTRY);
}



int
ParentChildrenList::UpdatePotlChild(nsaddr_t id, nsaddr_t next_hop, int num_hops, int level, int energy, int origin_time, int child_flag, int delete_flag, compr_taglist *taglist)
{
  LMNode *potl_child, *list_ptr;
  double now = Scheduler::instance().clock();

  //  if(a_->debug_) printf("Node %d: Number of potl children %d",a_->myaddr_,num_potl_children_);

  // cannot delete from an empty list!
  if(delete_flag && !pchildren_) {
    if(taglist) {
      compr_taglist *tag_ptr1, *tag_ptr2;
      tag_ptr1 = taglist;
      while(tag_ptr1) {
	tag_ptr2 = tag_ptr1;
	tag_ptr1 = tag_ptr1->next_;
	delete tag_ptr2;
      }
    }
    return(ENTRY_NOT_FOUND);
  }

  assert(num_potl_children_ >= 0);
  assert(num_children_ >= 0);

  if(pchildren_ == NULL) {
    pchildren_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
    pchildren_->child_flag_ = child_flag;
    if(child_flag == IS_CHILD) ++(num_children_);
    if(child_flag != NOT_POTL_CHILD) ++(num_potl_children_);
    ++(num_heard_);
    pchildren_->tag_list_ = taglist;
    return(NEW_ENTRY);
  }
  
  list_ptr = pchildren_;
  potl_child = list_ptr;
  while(list_ptr != NULL) {
    if(list_ptr->id_ == id) {
      // Check if this is a old message floating around in the network
      if(list_ptr->last_upd_origin_time_ >= origin_time) {
	// Check if we got this update on a shorter path
	if(list_ptr->num_hops_ > num_hops) {
	  list_ptr->next_hop_ = next_hop;
	  list_ptr->num_hops_ = num_hops;
	}
	if(taglist) {
	  compr_taglist *tag_ptr1, *tag_ptr2;
	  tag_ptr1 = taglist;
	  while(tag_ptr1) {
	    tag_ptr2 = tag_ptr1;
	    tag_ptr1 = tag_ptr1->next_;
	    delete tag_ptr2;
	  }
	}
	return(OLD_MESSAGE);
      }
      if(!delete_flag) {

	// Old entry but the status has changed to child or vice-versa
	if((list_ptr->child_flag_ == NOT_CHILD || list_ptr->child_flag_ == NOT_POTL_CHILD) && (child_flag == IS_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  ++(num_children_);
	}
	else if((list_ptr->child_flag_ == IS_CHILD) && (child_flag == NOT_CHILD || child_flag == NOT_POTL_CHILD)) {
	  list_ptr->child_flag_ = child_flag;
	  --(num_children_);
	}
	list_ptr->next_hop_ = next_hop;
	list_ptr->num_hops_ = num_hops;
	list_ptr->level_ = level;
	list_ptr->energy_ = energy;
	list_ptr->last_upd_origin_time_ = origin_time;
	list_ptr->last_update_rcvd_ = Scheduler::instance().clock();
	if(list_ptr->tag_list_ == NULL)
	  list_ptr->tag_list_ = taglist;
	else {
          compr_taglist *tag_ptr1, *tag_ptr2;
	  tag_ptr1 = list_ptr->tag_list_;
	  while(tag_ptr1) {
	    tag_ptr2 = tag_ptr1;
	    tag_ptr1 = tag_ptr1->next_;
	    delete tag_ptr2;
	  }
	  list_ptr->tag_list_ = taglist;
	}
      }
      else {
	if(pchildren_ == list_ptr)
	  pchildren_ = list_ptr->next_;
	else
	  potl_child->next_ = list_ptr->next_;

	if(list_ptr->child_flag_ == IS_CHILD) --num_children_;
	if(list_ptr->child_flag_ != NOT_POTL_CHILD) --num_potl_children_;
	--num_heard_;
	delete list_ptr;
      }
      return(OLD_ENTRY);
    }
    potl_child = list_ptr;
    list_ptr = list_ptr->next_;
  }

  // delete flag must be FALSE if we are here
  // assert(!delete_flag);
  if(delete_flag) {
    // Delete taglist if not empty
    if(taglist) {
      compr_taglist *tag_ptr1, *tag_ptr2;
      tag_ptr1 = taglist;
      while(tag_ptr1) {
	tag_ptr2 = tag_ptr1;
	tag_ptr1 = tag_ptr1->next_;
	delete tag_ptr2;
      }
    }
    return(ENTRY_NOT_FOUND);
  }

  potl_child->next_ = new LMNode(id, next_hop, num_hops, level, energy, origin_time, now);
  (potl_child->next_)->tag_list_ = taglist;
  (potl_child->next_)->child_flag_ = child_flag;
  if(child_flag == IS_CHILD) ++(num_children_);
  if(child_flag != NOT_POTL_CHILD) ++(num_potl_children_);
  ++(num_heard_);
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
  int action, energy = 0;
  nsaddr_t *potl_children;
  int num_children = 0;
  int num_potl_children = 0;
  // Packet will have seqnum, level, vicinity radii, parent info
  // and possibly potential children (if the node is at level > 0)
  int num_tags = 0;
  compr_taglist *adv_tags = NULL, *tag_ptr;
  Packet *newp;

  //  if(now > 412.5) {
  //    purify_printf("ProcessHierUpdate1, %f, %d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }


  //  if(debug_)
  //    trace("Node %d: Received packet from %d with ttl %d", myaddr_,iph->src(),iph->ttl_);

  walk = p->accessdata();

  // Originator of the LM advertisement and the next hop to reach originator
  origin_id = iph->src();    

  // Free and return if we are seeing our own packet again
  if(origin_id == myaddr_) {
    Packet::free(p);
    return;
  }

  // type of advertisement
  action = *walk++;

  // level of the originator
  level = *walk++;

  //  trace("Node %d: Processing Hierarchy Update Packet", myaddr_);


  //  if((myaddr_ == 42) && (origin_id == 6)) {
  //    trace("Node 42: Receiving level %d update from node 6 at time %f",level,s.clock());
  //  }

  // energy level of advertising node
  energy = *walk++;
  energy = (energy << 8) | *walk++;
  energy = (energy << 8) | *walk++;
  energy = (energy << 8) | *walk++;

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

  assert(origin_time <= now);

  //  if(debug_ && myaddr_ == 33)
  //    trace("Node %d: Processing level %d pkt from %d at t=%f, origin %d, num hops %d", myaddr_,level,iph->src_,now,origin_time,num_hops);


  // Parent of the originator
  parent = *walk++;
  parent = parent << 8 | *walk++;

  // Number of hops has to be less than vicinity radius to ensure that atleast
  // 2 level K LMs see each other if they exist
  if(level > 0 && (action == PERIODIC_ADVERTS || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT)) {
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

  num_tags = 0;
  if(action == PERIODIC_ADVERTS || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT) {
    num_tags = *walk++;
    num_tags = (num_tags << 8) | *walk++;
  }


  adv_tags = NULL;
  // Store tag info only if the level of advertising LM is less than 
  // our highest level; otherwise we dont need this information
  if(num_tags && level < highest_level_) {
    adv_tags = new compr_taglist;
    tag_ptr = adv_tags;
    i = 0;
    while(i < num_tags) {
      if(i) {
	tag_ptr->next_ = new compr_taglist;
	tag_ptr = tag_ptr->next_;
      }
      tag_ptr->obj_name_ = *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      tag_ptr->obj_name_ = (tag_ptr->obj_name_ << 8) | *walk++;
      ++i;
      //	trace("tag name: %d.%d.%d",(tag_ptr->obj_name_ >> 24) & 0xFF,(tag_ptr->obj_name_ >> 16) & 0xFF,(tag_ptr->obj_name_) & 0xFFFF);
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

  int delete_flag = FALSE; // Add the child/potl parent entry
  if(action == DEMOTION) delete_flag = TRUE;
  int child_flag = NOT_CHILD; // Indicates whether this node is our child
  if(parent == myaddr_) child_flag = IS_CHILD;
  //  if(num_hops == radius(level)) child_flag = NOT_POTL_CHILD;
  int ret_value = (*pcl2)->UpdatePotlChild(origin_id, next_hop, num_hops, level, energy, origin_time, child_flag, delete_flag,adv_tags);

  // Free packet and return if we have seen this packet before
  if(ret_value == OLD_MESSAGE) {
    if(num_potl_children) delete[] potl_children;
    Packet::free(p);
    return;
  }


  
  //  if(level == highest_level_)
  //     num_resched_ = (*pcl2)->num_potl_children_ - 1;

  // If promotion timer is running, decrement by constant amount
  if((ret_value == NEW_ENTRY) && (level == highest_level_) && (action == PERIODIC_ADVERTS || action == UNICAST_ADVERT_CHILD || action == UNICAST_ADVERT_PARENT) && (num_hops < radius(level))) {
    // Promotion timer is running but is not in wait state
    if(promo_timer_running_ && !wait_state_) {
      double resched_time = promo_timeout_ - (now - promo_start_time_) - promo_timeout_decr_;
      if(resched_time < 0) resched_time = 0;
      //      trace("Node %d: Rescheduling timer in ProcessHierUpdate, now %f, st %f, decr %f, resch %f\n", myaddr_, now, promo_start_time_, promo_timeout_decr_, resched_time);
      promo_timer_->resched(resched_time);
    }
  }

  if(action == DEMOTION) {
    (*pcl1)->UpdatePotlParent(origin_id, next_hop, num_hops, level, energy, origin_time, TRUE);
    if(((*pcl1)->parent_ == NULL) && (!promo_timer_running_) && (highest_level_ == level-1)) {
      //      if (debug_) printf("Node %d: sched timer in ProcessHierUpdate\n",myaddr_);
      ParentChildrenList *tmp_pcl = parent_children_list_;
      while(tmp_pcl) {
	if(tmp_pcl->level_ == level) break;
	tmp_pcl = tmp_pcl->next_;
      }
      assert(tmp_pcl);
      
      num_resched_ = tmp_pcl->num_heard_ - 1;
      if(num_resched_) {
	promo_timer_running_ = 1;
	wait_state_ = 0;
	total_wait_time_ = 0;
	promo_timeout_ = random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * radius(level) + MAX_TIMEOUT/((num_resched_+1) * pow(2,highest_level_+1))), be_random_) + (MAX_ENERGY - node_->energy())*200/MAX_ENERGY;
	trace("Node %d: Promotion timeout after wait period in ProcessHierUpdate: %f", myaddr_,promo_timeout_);
	num_resched_ = 0;
	promo_start_time_ = s.clock();
	promo_timer_->resched(promo_timeout_);
      }
      else {
	double wait_time = PERIODIC_WAIT_TIME;
	promo_timer_running_ = 1;
	wait_state_ = 1;
	total_wait_time_ += wait_time;
	trace("Node %d: Entering wait state in ProcessHierUpdate because of no parent: %f", myaddr_,now);
	promo_timer_->resched(wait_time);
      }
    }
  }
  // If the advertising LM is a potl parent, add to level-1 
  // ParentChildrenList object
  else if(potl_parent_flag) {
    LMNode *old_parent = (*pcl1)->parent_;
    (*pcl1)->UpdatePotlParent(origin_id, next_hop, num_hops, level, energy, origin_time, FALSE);

    // Trigger advertisement if parent has changed
    if((*pcl1)->parent_ != old_parent) {
      newp = makeUpdate((*pcl1), HIER_ADVS, PERIODIC_ADVERTS);
      s.schedule(target_,newp,0);
      (*pcl1)->last_update_sent_ = now;
    }

  // If a parent has been selected for highest_level_, cancel promotion timer
  // (for promotion to highest_level_+1) if it's running
    if((level == (highest_level_ + 1)) && ((*pcl1)->parent_ != NULL)) {
      if(promo_timer_running_) {
	trace("Node %d: Promotion timer cancelled at time %f in ProcessHierUpdate\n",myaddr_,s.clock());
	promo_timer_->cancel();
	promo_timer_running_ = 0;
	wait_state_ = 0;
	total_wait_time_ = 0;
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
	  if(lm->id_ == potl_children[i] && lm->child_flag_ != NOT_POTL_CHILD) {
	    is_element = TRUE;
	    break;
	  }
	if(is_element == FALSE && lm->child_flag_ != NOT_POTL_CHILD) {
	  is_subset = FALSE;
	  break;
	}
	lm = lm->next_;
      }
      // Demotion process
      if(is_subset == TRUE) {
    	--(highest_level_);
    	delete_flag = TRUE;
	if((*pcl1)->parent_)
	  trace("Node %d: Num potl ch %d, Node %d: Num potl ch %d, time %d",myaddr_, (*pcl)->num_potl_children_,origin_id,num_potl_children,(int)now);
	  trace("Node %d: Parent before demotion: %d, msg from %d at time %f",myaddr_, ((*pcl1)->parent_)->id_,origin_id,now);
    	(*pcl1)->UpdatePotlParent(myaddr_, 0, 0, 0, 0, (*pcl1)->seqnum_, delete_flag);
	++((*pcl1)->seqnum_);
	if((*pcl1)->parent_)
	  trace("Node %d: Parent after demotion: %d",myaddr_, ((*pcl1)->parent_)->id_);
    	(*pcl2)->UpdatePotlChild(myaddr_, 0, 0, 0, 0, (*pcl2)->seqnum_, IS_CHILD, delete_flag,NULL);
	++(*pcl2)->seqnum_;
	// Send out demotion messages
	newp = makeUpdate(*pcl, HIER_ADVS, DEMOTION);
	s.schedule(target_, newp, 0);

    	if(promo_timer_running_) {
    	  trace("Node %d: Promotion timer cancelled due to demotion at time %d\n",myaddr_,(int)now);
    	  promo_timer_->cancel();
    	  promo_timer_running_ = 0;
	  wait_state_ = 0;
	  total_wait_time_ = 0;
    	}
      }
    }      
  }


  // If new entry, flood advertisements for level > adv LM's level
  if(ret_value == NEW_ENTRY) {
    ParentChildrenList *tmp_pcl = parent_children_list_;
    while(tmp_pcl) {
      // New nodes should have an initial wait time of atleast 5 seconds
      if(tmp_pcl->level_ <= highest_level_ && tmp_pcl->level_ >= level && (now - tmp_pcl->last_update_sent_) > 5) {
	newp = makeUpdate(tmp_pcl, HIER_ADVS, PERIODIC_ADVERTS);
	s.schedule(target_,newp,0);
	tmp_pcl->last_update_sent_ = now;
      }
      tmp_pcl = tmp_pcl->next_;
    }
  }

  if(num_potl_children) delete[] potl_children;

  // Do not forward packet if ttl is lower
  if(--iph->ttl_ == 0) {
    //    drop(p, DROP_RTR_TTL);
    Packet::free(p);
    return;
  }

  // Do not forward if the advertisement is for this node
  if((iph->dst() >> 8) == myaddr_) {
    //    trace("Node %d: Received unicast advert from %d at level %d for us at time %f",myaddr_,iph->src(),level,now);
    Packet::free(p);
    return;
  }

  if(action == UNICAST_ADVERT_CHILD) {
    //    trace("Node %d: Received child unicast advert from %d to %d at level %d at time %f",myaddr_,iph->src(),iph->dst()>>8,level,now);
    hdrc->next_hop_ = get_next_hop((iph->dst_ >> 8),level);
  }
  else if(action == UNICAST_ADVERT_PARENT) {
    //    trace("Node %d: Received parent unicast advert from %d to %d at level %d at time %f",myaddr_,iph->src(),iph->dst()>>8,level,now);
    hdrc->next_hop_ = get_next_hop((iph->dst_ >> 8),level+2);
  }
  
  assert(hdrc->next_hop_ != NO_NEXT_HOP);

  //  if(now > 412.5) {
  //    purify_printf("ProcessHierUpdate2\n");
  //    purify_new_leaks();
  //  }

  // Need to send the packet down the stack
  hdrc->direction() = -1;
  
  //  if(debug_) printf("Node %d: Forwarding Hierarchy Update Packet", myaddr_);
  s.schedule(target_, p, 0);


}



void
LandmarkAgent::periodic_callback(Event *e, int level)
{
  //  if(debug_) printf("Periodic Callback for level %d", level);
  Scheduler &s = Scheduler::instance();
  compr_taglist *tag_list = NULL, *tag_ptr1 = NULL, *tag_ptr2 = NULL;
  compr_taglist *agg_tags = NULL;
  int num_tags;
  double now = Scheduler::instance().clock(), next_update_delay;
  int energy = (int)(node_->energy());
  int unicast_flag = FALSE, suppress_flag = FALSE;
  Packet *newp;
  hdr_ip *iph, *new_iph;
  hdr_cmn *cmh, *new_cmh;

  //  if(now > 412.5) {
  //    purify_printf("periodic_callback1: %f,%d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }

  //  if(myaddr_ == 12 && now > 402)
  //    purify_new_leaks();

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

  // Delete tag list for this level if it exists; a new tag list will replace
  // this older list later (new list formed by looking at cur set of children
  if(level > 0) {
    tag_ptr1 = cur_pcl->tag_list_;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
    cur_pcl->tag_list_ = NULL;
  }

  // Delete stale potential parent entries
  LMNode *lmnode = cur_pcl->pparent_;
  LMNode *tmp_lmnode;
  int delete_flag = TRUE;
  while(lmnode) {
    // Record next entry in linked list incase the current element is deleted
    tmp_lmnode = lmnode->next_;
    if(((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_)) {
      //      if(debug_) trace("Node %d: Deleting stale entry for %d at time %d",myaddr_,lmnode->id_,(int)now);
      cur_pcl->UpdatePotlParent(lmnode->id_, 0, 0, 0, 0, cur_pcl->seqnum_, delete_flag);
      ++(cur_pcl->seqnum_);
    }
    lmnode = tmp_lmnode;
  }
  

  // Delete stale potential children entries and compute update tag_list_
  // for level advertisements
  lmnode = cur_pcl->pchildren_;
  delete_flag = TRUE;
  tag_list = NULL;
  int demotion = FALSE;
  while(lmnode) {
    // Record next entry in linked list incase the current element is deleted
    tmp_lmnode = lmnode->next_;
    if((now - lmnode->last_update_rcvd_) > cur_pcl->update_timeout_) {
      cur_pcl->UpdatePotlChild(lmnode->id_, 0, 0, 0, 0, cur_pcl->seqnum_, lmnode->child_flag_, delete_flag,NULL);
      ++(cur_pcl->seqnum_);
    }
    else if(level && lmnode->child_flag_ == IS_CHILD) {
      // If this is our child and is not being deleted, aggregate its tag list

      //      if(myaddr_ == 33)
      //	trace("Node 33: Child %d's tags at time %f:",lmnode->id_,s.clock());
      //      if((lmnode->energy_ > energy + DEMOTION_DIFF) && (level == highest_level_))
      //	demotion = TRUE;
      tag_ptr1 = lmnode->tag_list_;
      while(tag_ptr1) {
	if(!tag_list) {
	  tag_list = new compr_taglist;
	  tag_ptr2 = tag_list;
	}
	else {
	  tag_ptr2->next_ = new compr_taglist;
	  tag_ptr2 = tag_ptr2->next_;
	}
	//	if(myaddr_ == 33)
	//	  trace("tag name: %d.%d.%d",(tag_ptr1->obj_name_ >> 24) & 0xFF,(tag_ptr1->obj_name_ >> 16) & 0xFF,(tag_ptr1->obj_name_) & 0xFFFF);
	tag_ptr2->obj_name_ = tag_ptr1->obj_name_;
	tag_ptr1 = tag_ptr1->next_;
      }
    }
    lmnode = tmp_lmnode;
  }

  // Demote ourself if any child's energy > 30 % of our energy
  if(demotion) {
    trace("Node %d: Demotion due to lesser energy than child",myaddr_);
    highest_level_ = level - 1;
    Packet *p = makeUpdate(cur_pcl, HIER_ADVS, DEMOTION);
    s.schedule(target_, p, 0);
  }



  if(level) {
    agg_tags = aggregate_taginfo(tag_list,level,&num_tags);

    //    if(myaddr_ == 33) {
    //      trace("Node 33: agg_tags (num_tags = %d) =",num_tags);
    //      compr_taglist *tmp_tagptr = agg_tags;
    //      while(tmp_tagptr) {
    //    	trace("tag name: %d.%d.%d",(tmp_tagptr->obj_name_ >> 24) & 0xFF,(tmp_tagptr->obj_name_ >> 16) & 0xFF,(tmp_tagptr->obj_name_) & 0xFFFF);
    //    	tmp_tagptr = tmp_tagptr->next_;
    //      }
    //    }
    
    cur_pcl->tag_list_ = agg_tags;
    cur_pcl->num_tags_ = num_tags;
    tag_ptr1 = tag_list;
    while(tag_ptr1) {
      tag_ptr2 = tag_ptr1;
      tag_ptr1 = tag_ptr1->next_;
      delete tag_ptr2;
    }
  }

  // Copy the updated cur_pcl->tag_list_ to tag_list for use in following
  // updates if demotion has not occurred
  if(!demotion) {
    tag_list = NULL;
    tag_ptr1 = cur_pcl->tag_list_;
    while(tag_ptr1) {
      if(!tag_list) {
	tag_list = new compr_taglist;
	tag_ptr2 = tag_list;
      }
      else {
	tag_ptr2->next_ = new compr_taglist;
	tag_ptr2 = tag_ptr2->next_;
      }
      tag_ptr2->obj_name_ = tag_ptr1->obj_name_;
      tag_ptr1 = tag_ptr1->next_;
    }
  }
  
  //  if(myaddr_ == 33 && level) {
  //    trace("Node 33: tag_list =");
  //    compr_taglist *tmp_tagptr = tag_list;
  //    int number_tags = 0;
  //    while(tmp_tagptr) {
  //      trace("tag name: %d.%d.%d",(tmp_tagptr->obj_name_ >> 24) & 0xFF,(tmp_tagptr->obj_name_ >> 16) & 0xFF,(tmp_tagptr->obj_name_) & 0xFFFF);
  //      tmp_tagptr = tmp_tagptr->next_;
  //      ++number_tags;
  //    }
  //    assert(cur_pcl->num_tags_ == number_tags);
  //    trace("end tag_list, %d tags",number_tags);
  //  }

  // Check if a parent exists after updating potl parents. If not, start
  // promotion timer.
  // A LM at level 3 is also at levels 0, 1 and 2. For each of these levels,
  // the LM designates itself as parent. At any given instant, only the 
  // level 3 (i.e., highest_level_) LM may not have a parent and may need to 
  // promote itself. But if the promotion timer is running, then the election
  // process for the next level has already begun.


  if((cur_pcl->parent_ == NULL) && !demotion) {

    // Cancel any promotion timer that is running for promotion from a higher 
    // level and send out demotion messages
    if(promo_timer_running_ && level < highest_level_) {
      wait_state_ = 0;
      total_wait_time_ = 0;
      promo_timer_running_ = 0;
      promo_timer_->cancel();


      trace("Node %d: Demotion from level %d to %d due to loss of parent",myaddr_,highest_level_,level);
      for(int i = level + 1; i <= highest_level_; ++i) {
	ParentChildrenList *tmp_pcl = parent_children_list_;
	while(tmp_pcl->level_ != i) tmp_pcl = tmp_pcl->next_;
	assert(tmp_pcl);
	Packet *p = makeUpdate(tmp_pcl, HIER_ADVS, DEMOTION);
	s.schedule(target_, p, 0);
      }
      highest_level_ = level;
    }

    if(!promo_timer_running_) {
      num_resched_ = pcl2->num_heard_ - 1;
      if(num_resched_) {
	promo_timer_running_ = 1;
	wait_state_ = 0;
	total_wait_time_ = 0;
	promo_timeout_ = random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * radius(level+1) + MAX_TIMEOUT/((num_resched_+1) * pow(2,highest_level_+1))), be_random_) + (MAX_ENERGY - node_->energy())*200/MAX_ENERGY;
	trace("Node %d: Promotion timeout after wait period in periodic_callback: %f", myaddr_,promo_timeout_);
	num_resched_ = 0;
	promo_start_time_ = s.clock();
	promo_timer_->resched(promo_timeout_);
      }
      else {
	double wait_time = PERIODIC_WAIT_TIME;
	promo_timer_running_ = 1;
	wait_state_ = 1;
	total_wait_time_ += wait_time;
	trace("Node %d: Entering wait period in periodic_callback at time %f", myaddr_,now);
	promo_timer_->resched(wait_time);
      }
    }
  }

  // Update ourself as potential child and parent for appropriate levels
  // in our hierarchy tables
  if(!demotion) {
    if(level) {
      pcl1->UpdatePotlParent(myaddr_, myaddr_, 0, level, energy, pcl1->seqnum_, FALSE);
      ++(pcl1->seqnum_);
    }
    pcl2->UpdatePotlChild(myaddr_, myaddr_, 0, level, energy, pcl2->seqnum_, IS_CHILD, FALSE,tag_list);
    ++(pcl2->seqnum_);
  }

  // Check if this is the root node. If so, set the unicast flag or suppress
  // flag when no changes occur for 2 times the update period
  if(!(cur_pcl->parent_) && (total_wait_time_ > (2*update_period_))) {
    if(adverts_type_ == UNICAST) {
      unicast_flag = TRUE;
    }
    else if(adverts_type_ == SUPPRESS) {
      suppress_flag = TRUE;
    }
  }
  else if(cur_pcl->parent_) {
    if(adverts_type_ == UNICAST) {
      unicast_flag = TRUE;
    }
    else if(adverts_type_ == SUPPRESS) {
      suppress_flag = TRUE;
    }
  } 
  
  if(!demotion && (now - cur_pcl->last_update_sent_ >= cur_pcl->update_period_) && !suppress_flag) {
    //    trace("Node %d: Sending update at time %f",myaddr_,now);
    Packet *p = makeUpdate(cur_pcl, HIER_ADVS, PERIODIC_ADVERTS);
    unsigned char *walk;
    if(unicast_flag) {
      if(level) {
	// Unicast updates to parent and children for level > 0
	lmnode = cur_pcl->pchildren_;
	while(lmnode) {
	  if(lmnode->id_ != myaddr_) {
	    newp = p->copy();
	    new_iph = (hdr_ip *) newp->access(off_ip_);
	    new_cmh = (hdr_cmn *) newp->access (off_cmn_);
	    walk = newp->accessdata();
	    //	    trace("Node %d: Generating unicast advert to child %d at time %f with next hop %d",myaddr_,lmnode->id_,now,lmnode->next_hop_);
	    
	    new_iph->dst() = (lmnode->id_ << 8) | ROUTER_PORT;
	    new_iph->dport() = ROUTER_PORT;
	    new_cmh->next_hop() = lmnode->next_hop_;
	    new_cmh->addr_type() = NS_AF_INET;
	    if(cur_pcl->level_)
	      new_cmh->size() = new_cmh->size() - 4 * (cur_pcl->num_potl_children_ - 1);
	    (*walk) = (UNICAST_ADVERT_CHILD) & 0xFF;
	    walk += 10;
	    (*walk++) = (cur_pcl->seqnum_ >> 24) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_ >> 16) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_ >> 8) & 0xFF;
	    (*walk++) = (cur_pcl->seqnum_) & 0xFF;
	    ++(cur_pcl->seqnum_);
	    
	    s.schedule(target_,newp,0);
	  }
	  lmnode = lmnode->next_;
	}
      }
      if(cur_pcl->parent_) {
	if((cur_pcl->parent_)->id_ != myaddr_) {
	  iph = (hdr_ip *) p->access(off_ip_);
	  cmh = (hdr_cmn *) p->access (off_cmn_);
	  walk = p->accessdata();
	  
	  //	  trace("Node %d: Generating unicast advert to parent %d at time %f with next hop %d",myaddr_,cur_pcl->parent_->id_,now,(cur_pcl->parent_)->next_hop_);
	  iph->dst() = ((cur_pcl->parent_)->id_ << 8) | ROUTER_PORT;
	  iph->dport() = ROUTER_PORT;
	  cmh->next_hop() = (cur_pcl->parent_)->next_hop_;
	  cmh->addr_type() = NS_AF_INET;
	  cmh->size() = cmh->size() - 4 * (cur_pcl->num_potl_children_);
	  
	  (*walk) = (UNICAST_ADVERT_PARENT) & 0xFF;
	  walk += 10;
	  (*walk++) = (cur_pcl->seqnum_ >> 24) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_ >> 16) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_ >> 8) & 0xFF;
	  (*walk++) = (cur_pcl->seqnum_) & 0xFF;
	  ++(cur_pcl->seqnum_);
	  
	  s.schedule(target_,p,0);
	}
	else 
	  Packet::free(p);
      }
      else
	Packet::free(p);
    }
    else {
      s.schedule(target_, p, 0);
    }
    cur_pcl->last_update_sent_ = now;
  }

  // Schedule next update 
  if(cur_pcl->last_update_sent_ == now || suppress_flag)
    next_update_delay = cur_pcl->update_period_ + jitter(LM_STARTUP_JITTER, be_random_);
  else
    next_update_delay = cur_pcl->update_period_ - (now - cur_pcl->last_update_sent_) + jitter(LM_STARTUP_JITTER, be_random_);
  

  //  if(now > 412.5) {
  //    purify_printf("periodic_callback2: %f,%d\n",now,myaddr_);
  //    purify_new_leaks();
  //  }

  //  if(myaddr_ == 12 && now > 402)
  //    purify_new_leaks();

  if(!demotion)
    s.schedule(cur_pcl->periodic_handler_, cur_pcl->periodic_update_event_, next_update_delay);
}



Packet *
LandmarkAgent::makeUpdate(ParentChildrenList *pcl, int pkt_type, int action)
{
  Packet *p = allocpkt();
  hdr_ip *iph = (hdr_ip *) p->access(off_ip_);
  hdr_cmn *hdrc = HDR_CMN(p);
  unsigned char *walk;
  compr_taglist *adv_tags = NULL;


  hdrc->next_hop_ = IP_BROADCAST; // need to broadcast packet
  hdrc->addr_type_ = NS_AF_INET;
  iph->dst() = IP_BROADCAST;  // packet needs to be broadcast
  iph->dport() = ROUTER_PORT;
  iph->ttl_ = radius(pcl->level_);

  iph->src() = myaddr_;
  iph->sport() = ROUTER_PORT;

  if(pkt_type == HIER_ADVS) {
    
    if(pcl->level_ == 0) {
      // A level 0 node cannot be demoted!
      assert(action != DEMOTION);

      // No children for level 0 LM
      // totally 12 bytes in packet for now; need to add our energy level later
      // each tag name is 4 bytes; 2 bytes for num_tags info
      p->allocdata(12 + (4 * pcl->num_tags_) + 2 + 4); 
      walk = p->accessdata();

      // Packet type; 1 byte
      (*walk++) = (action) & 0xFF;

      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // Our energy level; 4 bytes (just integer portion)
      int energy = (int)(node_->energy());
      (*walk++) = (energy >> 24) & 0xFF;
      (*walk++) = (energy >> 16) & 0xFF;
      (*walk++) = (energy >> 8) & 0xFF;
      (*walk++) = (energy) & 0xFF;

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
      (*walk++) = (pcl->seqnum_ >> 24) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 16) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 8) & 0xFF;
      (*walk++) = (pcl->seqnum_) & 0xFF;
      ++(pcl->seqnum_);

      // Parent ID; 2 bytes
      if(pcl->parent_ == NULL) {
	(*walk++) = (NO_PARENT >> 8) & 0xFF;
	(*walk++) = (NO_PARENT) & 0xFF;
      }
      else {
	(*walk++) = ((pcl->parent_)->id_ >> 8) & 0xFF;
	(*walk++) = ((pcl->parent_)->id_) & 0xFF;
      }

      (*walk++) = (pcl->num_tags_ >> 8) & 0xFF;
      (*walk++) = (pcl->num_tags_) & 0xFF;
      if(pcl->num_tags_) {
	adv_tags = pcl->tag_list_;
	while(adv_tags) {
	  (*walk++) = (adv_tags->obj_name_ >> 24) & 0xFF;
	  (*walk++) = (adv_tags->obj_name_ >> 16) & 0xFF;
      	  (*walk++) = (adv_tags->obj_name_ >> 8) & 0xFF;
	  (*walk++) = (adv_tags->obj_name_) & 0xFF;
	  adv_tags = adv_tags->next_;
	}
      }

      // In real life each of the above fields would be 
      // 4 byte integers; 20 bytes for IP addr
      // 4 byte for number of tags; 4 byte for each tag name; 4 byte for energy
      hdrc->size_ = 20 + 20 + 4 + (4 * pcl->num_tags_) + 4; 
    }
    else {
      // Need to list all the potential children LMs
      // Pkt size: 12 bytes (as before); 2 each for number of children 
      // and potl_children; 2 byte for each child's id
      // 4 bytes for each tag name; 2 bytes for num_tags info
      int pkt_size = 0;
      if(action == PERIODIC_ADVERTS)
	pkt_size = 12 + 4 + (2 * pcl->num_potl_children_) + (4 * pcl->num_tags_) + 2 + 4;
      else
	pkt_size = 16; // Demotion message

      p->allocdata(pkt_size);
      walk = p->accessdata();

      // Packet type; 1 byte
      (*walk++) = (action) & 0xFF;
      
      // level of LM advertisement; 1 byte
      (*walk++) = (pcl->level_) & 0xFF;

      // Our energy level; 4 bytes (just integer portion)
      int energy = (int)(node_->energy());
      (*walk++) = (energy >> 24) & 0xFF;
      (*walk++) = (energy >> 16) & 0xFF;
      (*walk++) = (energy >> 8) & 0xFF;
      (*walk++) = (energy) & 0xFF;

      // make ourselves as next hop; 2 bytes
      (*walk++) = (myaddr_ >> 8) & 0xFF;
      (*walk++) = (myaddr_) & 0xFF;

      // Vicinity size in number of hops; 2 bytes
      (*walk++) = (radius(pcl->level_) >> 8) & 0xFF;
      (*walk++) = (radius(pcl->level_)) & 0xFF;

      // Time at which packet was originated; 4 bytes (only integer portion)
      double now = Scheduler::instance().clock();
      int origin_time = (int)now;
      (*walk++) = (pcl->seqnum_ >> 24) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 16) & 0xFF;
      (*walk++) = (pcl->seqnum_ >> 8) & 0xFF;
      (*walk++) = (pcl->seqnum_) & 0xFF;
      ++(pcl->seqnum_);

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
      
      if(action == PERIODIC_ADVERTS) {
	// Number of children; 2 bytes
	(*walk++) = (pcl->num_children_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_children_) & 0xFF;
	
	// Number of potential children; 2 bytes
	(*walk++) = (pcl->num_potl_children_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_potl_children_) & 0xFF;
	
	LMNode *potl_ch = pcl->pchildren_;
	while(potl_ch) {
	  if(potl_ch->child_flag_ != NOT_POTL_CHILD) {
	    (*walk++) = (potl_ch->id_ >> 8) & 0xFF;
	    (*walk++) = (potl_ch->id_) & 0xFF;
	  }
	  potl_ch = potl_ch->next_;
	}

	(*walk++) = (pcl->num_tags_ >> 8) & 0xFF;
	(*walk++) = (pcl->num_tags_) & 0xFF;
	if(pcl->num_tags_) {
	  adv_tags = pcl->tag_list_;
	  while(adv_tags) {
	    (*walk++) = (adv_tags->obj_name_ >> 24) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_ >> 16) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_ >> 8) & 0xFF;
	    (*walk++) = (adv_tags->obj_name_) & 0xFF;
	    adv_tags = adv_tags->next_;
	  }
	}
	
	// 8 addl bytes for num_children and num_potl_children info
	hdrc->size_ = 20 + 8 + (4 * pcl->num_potl_children_) + 20 + 4 + (4 * pcl->num_tags_) + 4; 
	// In real life each of the above fields would be
	// 4 byte integers; 20 bytes for IP addr
	//	if(myaddr_ == 11) 
	//	  trace("Node 11: Packet size: %d",hdrc->size_);
      }
      else if(action == DEMOTION) {
	hdrc->size_ = 20 + 20;
      }      
    }
  }

  // Optimization for reducing energy consumption; Just advertise
  // sequence number in steady state
  //  if(pcl->parent_ == NULL && action != DEMOTION) 
  //    hdrc->size_ = 20 + 4;

  hdrc->direction() = -1;
  return p;
}



int
LandmarkAgent::radius(int level)
{
  // level i's radius >= (2 *level i-1's radius) + 1
  return((int(pow(2,level+1) + pow(2,level) - 1)));

  //  return((level + 1)*2 + 1);
  //  return(int(pow(2,level+1)) + 1);
}




ParentChildrenList::ParentChildrenList(int level, LandmarkAgent *a) : parent_(NULL), num_heard_(0), num_children_(0), num_potl_children_(0), num_pparent_(0), pchildren_(NULL), pparent_(NULL) , seqnum_(0) ,last_update_sent_(-(a->update_period_)), update_period_(a->update_period_), update_timeout_(a->update_timeout_), next_(NULL)
{
  level_ = level;
  periodic_update_event_ = new Event;
  periodic_handler_ = new LMPeriodicAdvtHandler(this);
  a_ = a;
  tag_list_ = NULL;
  num_tags_ = 0;
  adverts_type_ = FLOOD; // default is to flood adverts
}




void
PromotionTimer::expire(Event *e) {
  ParentChildrenList *pcl = a_->parent_children_list_;
  ParentChildrenList *new_pcl, *higher_level_pcl = NULL;
  ParentChildrenList *pcl1 = NULL; // Pointer to new highest_level_-1 object
  ParentChildrenList *pcl2 = NULL; // Pointer to new highest_level_+1 object
  ParentChildrenList *cur_pcl = NULL;
  int found = FALSE, has_parent = FALSE;
  int num_potl_ch = 0;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  compr_taglist *tag_list = NULL;

  //  if(now > 412.5) {
  //    purify_printf("expire1: %f,%d\n",now,a_->myaddr_);
  //    purify_new_leaks();
  //  }

  while(pcl != NULL) {
    if(pcl->level_ == (a_->highest_level_ + 1)) {
      // Exclude ourself from the count of the lower level nodes heard
      a_->num_resched_ = pcl->num_heard_ - 1;
      num_potl_ch = pcl->num_potl_children_;
    }
    else if(pcl->level_ == a_->highest_level_) {
      cur_pcl = pcl;
      if(pcl->parent_) {
	has_parent = TRUE;
      }
    }
    pcl = pcl->next_;
  }

  assert(cur_pcl);

  if(a_->wait_state_) {

    if(a_->num_resched_ && !has_parent) {
      a_->wait_state_ = 0;
      a_->total_wait_time_ = 0;
      
      // Promotion timeout is num_resched_ times the estimated time for 
      // a message to reach other nodes in its vicinity
      // PROM0_TIMEOUT_MULTIPLE is an estimate of time for adv to reach 
      // all nodes in vicinity
      a_->promo_timeout_ = a_->random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_) + MAX_TIMEOUT/((a_->num_resched_+1) * pow(2,a_->highest_level_))), a_->be_random_) + (MAX_ENERGY - (a_->node_)->energy())*200/MAX_ENERGY;
      a_->trace("Node %d: Promotion timeout after wait period in expire1: %f at time %f", a_->myaddr_,a_->promo_timeout_,s.clock());
      a_->num_resched_ = 0;
      a_->promo_start_time_ = s.clock();
      a_->promo_timer_->resched(a_->promo_timeout_);
    }
    else {
      double wait_time = PERIODIC_WAIT_TIME;
      a_->total_wait_time_ += wait_time;
      a_->trace("Node %d: Entering wait period in expire1 at time %f", a_->myaddr_,now);
      a_->promo_timer_->resched(wait_time);

      // Demote ourself we do not have any children (excluding ourself) after
      // waiting for 1.5 times the update period
      if(a_->highest_level_ && (a_->total_wait_time_ >= (a_->update_period_*1.5))) {
	//	a_->trace("Node %d: cur_pcl's number of children %d",a_->myaddr_,cur_pcl->num_children_);
	
	 if(cur_pcl->num_children_ <= 1) {
	   a_->trace("Node %d: Demoting from level %d because of no children",a_->myaddr_,a_->highest_level_);
	   --(a_->highest_level_);
	   Packet *p = a_->makeUpdate(cur_pcl, HIER_ADVS, DEMOTION);
	   s.schedule(a_->target_,p,0);
	 }
      }
    }      
    return;
  }    
  
  // Promotion timer is off
  a_->promo_timer_running_ = 0;

  // Only one promotion timer can be running at a node at a given instant. 
  // On expiry, the node will be promoted one level higher to highest_level_+1
  // Add a parentchildrenlist object for the higher level if one doesnt already
  // exist
  pcl = a_->parent_children_list_;
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
    a_->trace("Node %d: Promoted to level %d, num_potl_children %d at time %f", a_->myaddr_, a_->highest_level_, num_potl_ch, now);
    //    LMNode *lm = higher_level_pcl->pchildren_;
    //    a_->trace("Potential Children:");
    //    while(lm) {
    //      a_->trace("%d (level %d) Number of hops: %d", lm->id_,lm->level_,lm->num_hops_);
    //      lm = lm->next_;
    //    }
    
    //    lm = higher_level_pcl->pparent_;
    //    a_->trace("Potential Parent:");
    //    while(lm) {
    //      a_->trace("%d (level %d)", lm->id_,lm->level_);
    //      lm = lm->next_;
    //    }
  }
  
  // start off periodic advertisements for this higher level
  s.schedule(higher_level_pcl->periodic_handler_, higher_level_pcl->periodic_update_event_, 0);
  
  // add myself as potential parent for highest_level-1 and child for 
  // highest_level+1 
  int num_hops = 0;
  int energy = (int)((a_->node_)->energy());
  int child_flag = IS_CHILD;
  int delete_flag = FALSE;
  pcl1->UpdatePotlParent(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_, energy, pcl1->seqnum_, delete_flag);
  ++(pcl1->seqnum_);
  // tag_list == NULL doesnt matter because we're at a smaller level than
  // pcl2->level_ at this point. periodic_callback_ will update this field
  // correctly
  pcl2->UpdatePotlChild(a_->myaddr_, a_->myaddr_, num_hops, a_->highest_level_, energy, pcl2->seqnum_, child_flag, delete_flag, tag_list);
  ++(pcl2->seqnum_);
  
  // If we havent seen a LM that can be our parent at this higher level, start
  // promotion timer for promotion to next level
  if(higher_level_pcl->parent_ == NULL) {
    //    if (a_->debug_) printf("PromotionTimer's expire method: Scheduling timer for promo to level %d\n",a_->highest_level_);
    // Timer's status is TIMER_HANDLING in handle; so we just reschedule to
    // avoid "cant start timer" abort if sched is called
    a_->num_resched_ = pcl2->num_heard_ - 1;
    if(a_->num_resched_) {
      a_->promo_timer_running_ = 1;
      a_->wait_state_ = 0;
      a_->total_wait_time_ = 0;
      a_->promo_timeout_ = a_->random_timer(double(CONST_TIMEOUT + PROMO_TIMEOUT_MULTIPLES * a_->radius(a_->highest_level_+1) + MAX_TIMEOUT/((a_->num_resched_+1) * pow(2,a_->highest_level_+1))), a_->be_random_) + (MAX_ENERGY - (a_->node_)->energy())*200/MAX_ENERGY;
      a_->trace("Node %d: Promotion timeout after wait period in expire2: %f at time %f, num_resched_ %d, energy %f", a_->myaddr_,a_->promo_timeout_,s.clock(),a_->num_resched_,(a_->node_)->energy());
      a_->num_resched_ = 0;
      a_->promo_start_time_ = s.clock();
      a_->promo_timer_->resched(a_->promo_timeout_);
    }
    else {
      double wait_time = PERIODIC_WAIT_TIME;
      a_->promo_timer_running_ = 1;
      a_->wait_state_ = 1;
      a_->total_wait_time_ += wait_time;
      a_->trace("Node %d: Entering wait period in expire1 at time %f", a_->myaddr_,now);
      a_->promo_timer_->resched(wait_time);
    }
  }

  //  if(now > 412.5) {
  //    purify_printf("expire2: %f,%d\n",now,a_->myaddr_);
  //    purify_new_leaks();
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
      if (strcmp (argv[1], "print-nbrs") == 0)
	{
	  get_nbrinfo();
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "enable-caching") == 0)
	{
	  cache_ = 1;
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "unicast-adverts") == 0)
	{
	  adverts_type_ = UNICAST;
	  return (TCL_OK);
	}
      if (strcmp (argv[1], "hard-state-adverts") == 0)
	{
	  adverts_type_ = SUPPRESS;
	  // Entries should never timeout in a hard-state scheme
	  update_timeout_ = 1000000;
	  return (TCL_OK);
	}
      else if (strcmp (argv[1], "dumprtab") == 0)
	{
	  Packet *p2 = allocpkt ();
	  hdr_ip *iph2 = (hdr_ip *) p2->access (off_ip_);
	  //	  rtable_ent *prte;

          trace ("Table Dump %d[%d]\n----------------------------------\n",
	    myaddr_, iph2->sport());
	  trace ("VTD %.5f %d:%d\n", Scheduler::instance ().clock (),
		 myaddr_, iph2->sport_);
	  trace ("Remaining energy: %f", node_->energy());
	  trace ("Energy consumed by queries: %f", node_->qry_energy());

	  /*
	   * Freeing a routing layer packet --> don't need to
	   * call drop here.
	   */
	  trace("Highest Level: %d", highest_level_);
	  Packet::free (p2);
	  /*	  ParentChildrenList *pcl = parent_children_list_;
	  LMNode *pch;
	  while(pcl) {
	    trace("Level %d:", pcl->level_);
	    if(pcl->parent_)
	      trace("Parent: %d", (pcl->parent_)->id_);
	    else
	      trace("Parent: NULL");
	    trace("Number of children: %d\n", pcl->num_children_);
	    trace("Number of level %d nodes heard: %d\n", (pcl->level_)-1, pcl->num_heard_);
	    trace("Number of potl children: %d\n", pcl->num_potl_children_);
	    trace("Number of potl parent: %d\n", pcl->num_pparent_);
	    if(pcl->level_ >= 2 && highest_level_ >= 2) {
	      pch = pcl->pchildren_;
	      trace("Potential Children (radius %d):",radius(pcl->level_));
	      while(pch) {
		if(pch->child_flag_ != NOT_POTL_CHILD)
		  trace("Node %d (%d hops away)",pch->id_,pch->num_hops_);
		pch = pch->next_;
	      }
	    }
	    pcl = pcl->next_;
	  }
	  */

	  Packet::free(p2);
	  return (TCL_OK);
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
      else if (strcasecmp (argv[1], "set-update-period") == 0) {
	update_period_ = atof(argv[2]);
	if(adverts_type_ != SUPPRESS)
	  update_timeout_ = 2 * update_period_ + LM_STARTUP_JITTER;
	return TCL_OK;
      }
      else if (strcasecmp (argv[1], "set-update-timeout") == 0) {
	update_timeout_ = atof(argv[2]);
	return TCL_OK;
      }
      else if (strcasecmp (argv[1], "attach-tag-dbase") == 0)
	{
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  tag_dbase_ = (tags_database *) obj;
	  return TCL_OK;
	}
      else if (strcasecmp (argv[1], "node") == 0)
	{
	  assert(node_ == NULL);
	  TclObject *obj;
	if ((obj = TclObject::lookup (argv[2])) == 0)
	    {
	      fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1],
		       argv[2]);
	      return TCL_ERROR;
	    }
	  node_ = (MobileNode *) obj;
	  return TCL_OK;
	}
      else if (strcmp (argv[1], "query-debug") == 0)
	{
	  qry_debug_ = atoi(argv[2]);
	  return (TCL_OK);
	}
      else if (strcasecmp (argv[1], "ll-queue") == 0)
	{
	if (!(ll_queue = (PriQueue *) TclObject::lookup (argv[2])))
	    {
	      fprintf (stderr, "Landmark_Agent: ll-queue lookup of %s failed\n", argv[2]);
	      return TCL_ERROR;
	    }

	  return TCL_OK;
	}
    }

return (Agent::command (argc, argv));
}




void
LandmarkAgent::startUp()
{
  int i,ntags;
  Scheduler &s = Scheduler::instance();
  compr_taglist *local_tags0, *local_tags1, *local_tags2, *t_ptr;

  double x,y,z;
  node_->getLoc(&x,&y,&z);
  //  printf("Node %d position: (%f,%f,%f)\n",myaddr_,x,y,z);

  // Detection range smaller than transmission range. This is because, if 
  // the tags are passive, they may not have sufficient energy to re-radiate
  // information to the sensor
  double r = 60;

  local_tags0 = tag_dbase_->Gettags(x,y,r);
  //  trace("Node %d's at (%f,%f,%f) senses tags:",myaddr_,x,y,z);

  t_ptr = local_tags0;
  ntags = 0;
  while(t_ptr) {
    //    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    ++ntags;
    t_ptr = t_ptr->next_;
  }
  //  trace("Number of tags: %d",ntags);

  /* COMMENTING OUT FOLLOWING PORTIONS
  int agg_level = 1;
  int num_tags = 0;
  local_tags1 = aggregate_taginfo(local_tags0,agg_level,&num_tags);
  trace("Level 1 aggregates, num = %d",num_tags);
  t_ptr = local_tags1;
  while(t_ptr) {
    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    t_ptr = t_ptr->next_;
  }

  agg_level = 2;
  num_tags = 0;
  local_tags2 = aggregate_taginfo(local_tags1,agg_level,&num_tags);
  trace("Level 2 aggregates, num = %d",num_tags);
  t_ptr = local_tags2;
  while(t_ptr) {
    trace("tag name: %d.%d.%d",(t_ptr->obj_name_ >> 24) & 0xFF,(t_ptr->obj_name_ >> 16) & 0xFF,(t_ptr->obj_name_) & 0xFFFF);
    t_ptr = t_ptr->next_;
  }
  */

  assert(highest_level_ == 0);
  assert(parent_children_list_ == NULL);
  parent_children_list_ = new ParentChildrenList(highest_level_, this);
  ParentChildrenList **pcl = &parent_children_list_;

  // Start off periodic LM advertisements
  assert(highest_level_ == 0);
  s.schedule((*pcl)->periodic_handler_, (*pcl)->periodic_update_event_, INITIAL_WAIT_TIME + jitter(LM_STARTUP_JITTER, be_random_));
  (*pcl)->tag_list_ = local_tags0;
  (*pcl)->num_tags_ = ntags;

  // Start off promotion timer
  //  if (debug_) printf("startUp: Scheduling timer\n");
  promo_timer_running_ = 1;
  num_resched_ = 0;

  // Node enters "wait" state where it waits to receive other node's 
  // advertisements; Wait for 5 * radius(level) seconds; Should be
  // atleast the same as the period of LM advts (10s)
  wait_state_ = 1;
  double wait_time = WAIT_TIME * radius(highest_level_) + INITIAL_WAIT_TIME + LM_STARTUP_JITTER;
  total_wait_time_ += wait_time;
  //  trace("Node %d: Wait time at startUp: %f",myaddr_,wait_time);
  promo_timer_->sched(wait_time);

  //  promo_timer_->sched(promo_timeout_);
}




compr_taglist *
LandmarkAgent::aggregate_taginfo(compr_taglist *unagg_tags, int agg_level, int *num_tags)
{
  compr_taglist *agg_tags, *agg_ptr1, *agg_ptr2,  *last_agg_ptr;
  int found;
  
  *num_tags = 0;
  // agg_level is 1 implies ignore the last field
  // agg_level >= 2 implies ignore the last two fields
  agg_ptr1 = unagg_tags;
  agg_tags = NULL;
  
  while(agg_ptr1) {
    if(agg_level == 1) {
      found = FALSE;
      if(agg_tags) {
	agg_ptr2 = agg_tags;
	while(agg_ptr2) {
	  if((((agg_ptr2->obj_name_ >> 24) & 0xFF) == ((agg_ptr1->obj_name_ >> 24) & 0xFF)) && (((agg_ptr2->obj_name_ >> 16) & 0xFF) == ((agg_ptr1->obj_name_ >> 16) & 0xFF))) {
	    found = TRUE;
	    break;
	  }
	  last_agg_ptr = agg_ptr2;
	  agg_ptr2 = agg_ptr2->next_;
	}
      }

      if(!found) {
	++(*num_tags);
	if(!agg_tags) {
	  agg_tags = new compr_taglist;
	  last_agg_ptr = agg_tags;
	}
	else {
	  last_agg_ptr->next_ = new compr_taglist;
	  last_agg_ptr = last_agg_ptr->next_;
	}
	last_agg_ptr->obj_name_ = (agg_ptr1->obj_name_ & 0xFFFF0000);
      }
    }
    else if(agg_level >= 2) {
      found = FALSE;
      if(agg_tags) {
	agg_ptr2 = agg_tags;
	while(agg_ptr2) {
	  if(((agg_ptr2->obj_name_ >> 24) & 0xFF) == ((agg_ptr1->obj_name_ >> 24) & 0xFF)) {
	    found = TRUE;
	    break;
	  }
	  last_agg_ptr = agg_ptr2;
	  agg_ptr2 = agg_ptr2->next_;
	}
      }

      if(!found) {
	++(*num_tags);
	if(!agg_tags) {
	  agg_tags = new compr_taglist;
	  last_agg_ptr = agg_tags;
	}
	else {
	  last_agg_ptr->next_ = new compr_taglist;
	  last_agg_ptr = last_agg_ptr->next_;
	}
	last_agg_ptr->obj_name_ = (agg_ptr1->obj_name_ & 0xFF000000);
      }
    }
    agg_ptr1 = agg_ptr1->next_;	
  }
  return(agg_tags);
}



void
LandmarkAgent::recv(Packet *p, Handler *)
{
  hdr_ip *iph = (hdr_ip *) p->access (off_ip_);
  hdr_cmn *cmh = (hdr_cmn *) p->access (off_cmn_);

  /*
   *  Must be a packet being originated by the query agent at my node?
   */

  if(iph->src_ == myaddr_ && iph->sport_ == 0) {
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
    Packet::free(p);
    //   drop(p, DROP_RTR_ROUTE_LOOP);
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
  
  unsigned char *walk = p->accessdata();
  int action = *walk++;

  if ((iph->src_ != myaddr_) && (iph->dport_ == ROUTER_PORT) && (action != QUERY_PKT))
    {
      ProcessHierUpdate(p);
    }
  else
    {
      ForwardPacket(p);
    }
}




void
LandmarkAgent::ForwardPacket(Packet *p)
{
  hdr_ip *iph = (hdr_ip *) p->access (off_ip_);
  hdr_cmn *cmh = (hdr_cmn *) p->access (off_cmn_);
  Packet *newp;
  hdr_ip *new_iph;
  hdr_cmn *new_cmh;
  unsigned char *walk, *X_ptr, *Y_ptr, *level_ptr, *num_src_hops_ptr;
  unsigned char *last_hop_ptr, *pkt_end_ptr;
  int X, Y, next_hop_level, prev_hop_level, obj_name, num_src_hops;
  double local_x, local_y, local_z;
  int num_dst = 0, action, origin_time;
  NodeIDList *dst_nodes = NULL, *dst_ptr = NULL;
  int query_for_us = FALSE;
  Scheduler &s = Scheduler::instance();
  double now = s.clock();
  nsaddr_t last_hop_id;
  int cache_index = -1; // index into cache if object is found
  int found = FALSE; // whether object has been found in cache

  walk = p->accessdata();

  // Type of advertisement
  action = *walk++;

  X = 0;
  X_ptr = walk;
  X = *walk++;
  X = (X << 8) | *walk++;

  Y_ptr = walk;
  Y = *walk++;
  Y = (Y << 8) | *walk++;

  // level of our parent/child node that forwarded the query to us
  level_ptr = walk;
  next_hop_level = *walk++;

  obj_name = *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;
  obj_name = (obj_name << 8) | *walk++;

    // origin time of advertisement
  origin_time = *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  origin_time = (origin_time << 8) | *walk++;
  
  num_src_hops_ptr = walk;
  num_src_hops = *walk++;
  num_src_hops = (num_src_hops << 8) | *walk++;

  assert(num_src_hops <= 30);

  last_hop_ptr = NULL;
  for(int i = 0; i < num_src_hops; ++i) {
    last_hop_ptr = walk;
    walk += 3;
  }

  if(last_hop_ptr) {
    prev_hop_level = *(last_hop_ptr+2);
    last_hop_id = *last_hop_ptr;
    last_hop_id = (last_hop_id << 8) | *(last_hop_ptr+1);
  }
  else {
    prev_hop_level = 0;
    last_hop_id = NO_NEXT_HOP;
  }

  // Used to add source route to packet
  pkt_end_ptr = walk;
  
  // If this is a response pkt, cache this information
  if(cache_) {
    if(X != 65000 && Y != 65000) {
      if(num_cached_items_ < MAX_CACHE_ITEMS) {
	
	int replace_index = num_cached_items_;
	// If object already exists in cache, update info if necessary
	for(int i = 0; i < num_cached_items_; ++i) {
	  if(tag_cache_[i].obj_name_ == obj_name && tag_cache_[i].origin_time_ < origin_time) {
	  replace_index = i;
	  break;
	  }
	}
	
	tag_cache_[replace_index].obj_name_ = obj_name;
	tag_cache_[replace_index].origin_time_ = origin_time;
	tag_cache_[replace_index].X_ = X;
	tag_cache_[replace_index].Y_ = Y;
	++num_cached_items_;
      }
      else {
	// Use LRU cache replacement
	int replace_index = 0;
	int least_time = tag_cache_[replace_index].origin_time_;
	for(int i = 0; i < MAX_CACHE_ITEMS; ++i) {
	  if(tag_cache_[i].origin_time_ < least_time)
	    replace_index = i;
      }
	tag_cache_[replace_index].obj_name_ = obj_name;
	tag_cache_[replace_index].origin_time_ = origin_time;
	tag_cache_[replace_index].X_ = X;
	tag_cache_[replace_index].Y_ = Y;
      }
    }
    else {
      // If this is a query pkt; check if we have the relevant information 
      // cached. TTL = 600 seconds for the cache entries
      found = FALSE;
      for(int i = 0; i < num_cached_items_; ++i) {
	if(tag_cache_[i].obj_name_ == obj_name && tag_cache_[i].origin_time_ > origin_time - 600) {
	  found = TRUE;
	  cache_index = i;
	  break;
	}
      }
    }
  }


  // Loop check i.e., if response to our query agent has looped back
  // Following not the correct condition to detect a loop!
  //  assert(!((iph->dst_ >> 8) == myaddr_ && iph->dport_ == 0));

  // Reduce the source route to just parent-children (O(#levels))
  // This is possible since parent and child in each others vicinity
  cmh->direction() = -1;
  if((iph->dst_ >> 8) == myaddr_)
    query_for_us = TRUE;

  // Query pkt if X and Y are 65000
  if(X == 65000 && Y == 65000) {

    if(query_for_us || found) {
      if(qry_debug_)
	trace("Node %d: Rcved qry for us from node %d at time %f",myaddr_,last_hop_id,s.clock());
      if(!found)
	dst_nodes = search_tag(obj_name,prev_hop_level,next_hop_level,last_hop_id,&num_dst);

      if((num_dst == 0 && dst_nodes) || found) {
	delete dst_nodes;
	// if num_dst = 0 but dst_nodes is not NULL, we sense the
	// requested tag; add X,Y info and send response
	// if found is true, we have the cached information
	if(found) {
	  (*X_ptr++) = ((int)tag_cache_[cache_index].X_ >> 8) & 0xFF;
	  (*X_ptr) = ((int)tag_cache_[cache_index].X_) & 0xFF;
	  (*Y_ptr++) = ((int)tag_cache_[cache_index].Y_ >> 8) & 0xFF;
	  (*Y_ptr) = ((int)tag_cache_[cache_index].Y_) & 0xFF;
	}
	else {
	  node_->getLoc(&local_x, &local_y, &local_z);
	  (*X_ptr++) = ((int)local_x >> 8) & 0xFF;
	  (*X_ptr) = ((int)local_x) & 0xFF;
	  (*Y_ptr++) = ((int)local_y >> 8) & 0xFF;
	  (*Y_ptr) = ((int)local_y) & 0xFF;
	}

	// Send response
	iph->ttl_ = 1000;
	// Add 50 bytes to response 
	cmh->size() += 50;
	// Query from an agent at our node
	if(!num_src_hops) {
	  iph->dst_ = (myaddr_ << 8) | 0;
	  iph->dport_ = 0;
	  cmh->next_hop_ = myaddr_;
	}
	else {
	  --num_src_hops;
	  *num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	  *(num_src_hops_ptr + 1) = num_src_hops & 0xFF;
	  // Decr pkt size
	  cmh->size() -= 4;

	  iph->dst_ = *last_hop_ptr++;
	  iph->dst_ = (iph->dst_ << 8) | *last_hop_ptr++;
	  if(!num_src_hops)
	    iph->dst_ = (iph->dst_ << 8) | 0;
	  else
	    iph->dst_ = (iph->dst_ << 8) | ROUTER_PORT;
	  
	  int relevant_level = *last_hop_ptr;
	  cmh->next_hop_ = get_next_hop((iph->dst_ >> 8),relevant_level);
	  //	  assert(cmh->next_hop_ != NO_NEXT_HOP);	  
	  if(cmh->next_hop_ == NO_NEXT_HOP) {
	    Packet::free(p);
	    trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	    return;
	  }
	  *level_ptr = *last_hop_ptr;	  
	}

	//	if(found) 
	  //	  trace("Node %d: Gen response from cache at time %f to node %d",myaddr_,s.clock(),iph->dst_ >> 8);
	//	else
	  //	  trace("Node %d: Gen response at time %f to node %d",myaddr_,s.clock(),iph->dst_ >> 8);

	if(!num_src_hops && (iph->dst_ >> 8) == myaddr_) {
	  // TEMPORARY HACK! Cant forward from routing agent to some other
	  // agent on our node!
	  Packet::free(p);
	  if(now < 2000)
	    trace("Found object %d.%d.%d at (%d,%d) at time %f", (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,s.clock());
	  return;
	}
	else if((iph->dst_ >> 8) == myaddr_) {
	  ForwardPacket(p);
	}
	else {
	  s.schedule(target_,p,0);
	}
      }
      else if(num_dst >= 1) {

	if(--iph->ttl_ == 0) {
	  drop(p, DROP_RTR_TTL);
	  return;
	}
	
	// Add ourself to source route and increase the number of src hops
	//	if(last_hop_id != myaddr_) {
	++num_src_hops;
	*num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	*(num_src_hops_ptr+1) = num_src_hops & 0xFF;
	*pkt_end_ptr++ = (myaddr_ >> 8) & 0xFF;
	*pkt_end_ptr++ = myaddr_ & 0xFF;
	// Indicate the level of the pcl object that a node should look-up
	// to find the relevant routing table entry
	*pkt_end_ptr = (next_hop_level+1) & 0xFF;
	// Incr pkt size
	cmh->size() += 4;

	dst_ptr = dst_nodes;
	// Replicate pkt to each destination
	iph->dst_ = (dst_ptr->dst_node_ << 8) | ROUTER_PORT;
	iph->dport_ = ROUTER_PORT;
	cmh->next_hop_ = dst_ptr->dst_next_hop_;
	cmh->addr_type_ = NS_AF_INET;
	// Copy next hop variable to this variable temporarily
	// Copy it back into packet before sending the packet
	int tmp_next_hop_level = dst_ptr->next_hop_level_;

	if(qry_debug_)
	  trace("Node %d: Forwarding qry to node %d at time %f",myaddr_,dst_ptr->dst_node_,s.clock());

	dst_ptr = dst_ptr->next_;
	delete dst_nodes;
	dst_nodes = dst_ptr;

	for(int i = 1; i < num_dst; ++i) {
	  if(qry_debug_)
	    trace("Node %d: Forwarding qry to node %d at time %f",myaddr_,dst_ptr->dst_node_,s.clock());

	  // Change level and copy the packet
	  *level_ptr = dst_ptr->next_hop_level_;

	  newp = p->copy();

	  new_iph = (hdr_ip *) newp->access(off_ip_);
	  new_cmh = (hdr_cmn *) newp->access (off_cmn_);

	  new_iph->dst_ = (dst_ptr->dst_node_ << 8) | ROUTER_PORT;
	  new_iph->dport_ = ROUTER_PORT;
	  new_cmh->next_hop_ = dst_ptr->dst_next_hop_;
	  new_cmh->addr_type_ = NS_AF_INET;

	  if((new_iph->dst_ >> 8) == myaddr_)
	    ForwardPacket(newp);
	  else
	    s.schedule(target_,newp,0);

	  dst_ptr = dst_ptr->next_;
	  delete dst_nodes;
	  dst_nodes = dst_ptr;
	}
	
	*level_ptr = tmp_next_hop_level;

	if((iph->dst_ >> 8) == myaddr_) {
	  ForwardPacket(p);
	}
	else
	  s.schedule(target_,p,0);
      }
      else if(num_dst == 0) {
	// Free packet if we dont have any dst to forward packet
	if(qry_debug_)
	  trace("Node %d: Dropping query from %d at time %f,num_src_hops %d",myaddr_,iph->src_,s.clock(),num_src_hops);
	
	Packet::free(p);
	return;
      }
    }
    else {
      // simply forward to next hop
      if(qry_debug_)
	trace("Node %d: Forwarding query to node %d at time %f,num_src_hops %d",myaddr_,iph->dst_ >> 8,s.clock(),num_src_hops);

      if(--iph->ttl_ == 0) {
	drop(p, DROP_RTR_TTL);
        return;
      }

      cmh->next_hop_ = get_next_hop((iph->dst_ >> 8),next_hop_level+1);
      //      assert(cmh->next_hop_ != NO_NEXT_HOP);	  
      if(cmh->next_hop_ == NO_NEXT_HOP) {
	Packet::free(p);
	trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	return;
      }
      s.schedule(target_,p,0);
    }
  }
  else {
    // Forward the response packet
    if(qry_debug_)
      trace("Node %d: Forwarding response to node %d at time %f,num_src_hops %d",myaddr_,iph->dst_ >> 8,s.clock(),num_src_hops);
    if(--iph->ttl_ == 0) {
      drop(p, DROP_RTR_TTL);
      return;
    }

    if(query_for_us) {
	// Query from an agent at our node
      if(!num_src_hops) {
	iph->dst_ = (myaddr_ << 8) | 0;
	iph->dport_ = 0;
	cmh->next_hop_ = myaddr_;
      }
      else {
	--num_src_hops;
	*num_src_hops_ptr = (num_src_hops >> 8) & 0xFF;
	*(num_src_hops_ptr + 1) = num_src_hops & 0xFF;
	// Decr pkt size
	cmh->size() -= 4;
	
	iph->dst_ = *last_hop_ptr++;
	iph->dst_ = (iph->dst_ << 8) | *last_hop_ptr++;

	if(!num_src_hops)
	  iph->dst_ = (iph->dst_ << 8) | 0;
	else
	  iph->dst_ = (iph->dst_ << 8) | ROUTER_PORT;

	iph->dport_ = 0;

	int relevant_level = *last_hop_ptr;
	cmh->next_hop_ = get_next_hop((iph->dst_ >> 8),relevant_level);
	//	assert(cmh->next_hop_ != NO_NEXT_HOP);	  
	if(cmh->next_hop_ == NO_NEXT_HOP) {
	  Packet::free(p);
	  trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	  return;
	}
	
	*level_ptr = *last_hop_ptr;	  
      }
      if(!num_src_hops && (iph->dst_ >> 8) == myaddr_) {
	// TEMPORARY HACK! Cant forward from routing agent to some other
	// agent on our node!
	Packet::free(p);
	if(now < 2000)
	  trace("Found object %d.%d.%d at (%d,%d) at time %f", (obj_name >> 24) & 0xFF, (obj_name >> 16) & 0xFF, obj_name & 0xFFFF,X,Y,s.clock());
	return;
      }
      else if((iph->dst_ >> 8) == myaddr_)
	ForwardPacket(p);
      else
	s.schedule(target_,p,0);      
    }
    else {
      cmh->next_hop_ = get_next_hop((iph->dst_ >> 8),next_hop_level);
      //      assert(cmh->next_hop_ != NO_NEXT_HOP);	  
      if(cmh->next_hop_ == NO_NEXT_HOP) {
	Packet::free(p);
	trace("Node %d: Packet dropped because of no next hop info",myaddr_);
	return;
      }
      s.schedule(target_,p,0);
    }
  }
}




NodeIDList *
LandmarkAgent::search_tag(int obj_name, int prev_hop_level, int next_hop_level,nsaddr_t last_hop_id, int *num_dst)
{
  ParentChildrenList *pcl = parent_children_list_;
  LMNode *child;
  compr_taglist *tag_ptr;
  int found = FALSE, forward = FALSE;
  NodeIDList *nlist = NULL, *nlist_ptr = NULL;

  *num_dst = 0;

  // Check if our node senses the requested tag
  while(pcl->level_ != 0)
    pcl = pcl->next_;

  // if our node senses the tag, add the node to nlist but do not increase
  // num_dst
  tag_ptr = pcl->tag_list_;
  while(tag_ptr) {
    if(tag_ptr->obj_name_ == obj_name) {
      nlist = new NodeIDList;
      nlist->dst_node_ = myaddr_;
      nlist->dst_next_hop_ = myaddr_;
      return(nlist);
    }
    tag_ptr = tag_ptr->next_;
  }

  // If next_hop_level = 2, lookup would be done in the level 2 object
  // that would have level 1 tag aggregates
  if(next_hop_level == 2)
    obj_name = obj_name & 0xFFFF0000;
  else if(next_hop_level >= 3)
    obj_name = obj_name & 0xFF000000;

  pcl = parent_children_list_;
  while(pcl->level_ != next_hop_level)
    pcl = pcl->next_;
  
  assert(pcl);
  child = pcl->pchildren_;
  while(child) {
    // Dont forward back to child if child forwarded this query to us
    // We should forward to all children though if the message is going
    // down the hierarchy
    forward = FALSE;
    if(next_hop_level < prev_hop_level || (child->id_ != last_hop_id && next_hop_level >= prev_hop_level))
      forward = TRUE;
    if(child->child_flag_ == IS_CHILD && forward) {
      tag_ptr = child->tag_list_;
      while(tag_ptr) {
	if(tag_ptr->obj_name_ == obj_name) {
	  if(!nlist) {
	    nlist = new NodeIDList;
	    nlist_ptr = nlist;
	  }
	  else {
	    nlist_ptr->next_ = new NodeIDList;
	    nlist_ptr = nlist_ptr->next_;
	  }
	  nlist_ptr->dst_node_ = child->id_;
	  nlist_ptr->dst_next_hop_ = child->next_hop_;
	  nlist_ptr->next_hop_level_= next_hop_level - 1;
	  ++(*num_dst);
	  break;
	}
	tag_ptr = tag_ptr->next_;
      }
    }
    child = child->next_;
  }

  // Add parent if query is travelling up the hierarchy
  if(next_hop_level >= prev_hop_level && pcl->parent_) {
    if(!nlist) {
      nlist = new NodeIDList;
      nlist_ptr = nlist;
    }
    else {
      nlist_ptr->next_ = new NodeIDList;
      nlist_ptr = nlist_ptr->next_;
    }
    nlist_ptr->dst_node_ = (pcl->parent_)->id_;
    nlist_ptr->dst_next_hop_ = (pcl->parent_)->next_hop_;
    nlist_ptr->next_hop_level_= next_hop_level + 1;
    ++(*num_dst);
  }

  return(nlist);
}



nsaddr_t
LandmarkAgent::get_next_hop(nsaddr_t dst, int next_hop_level)
{
  ParentChildrenList *pcl = parent_children_list_;
  LMNode *pchild;

  while(pcl->level_ != next_hop_level) {
    pcl = pcl->next_;
  }
  
  assert(pcl);
  pchild = pcl->pchildren_;
  while(pchild) {
    if(pchild->id_ == dst)
      return(pchild->next_hop_);
    pchild = pchild->next_;
  }

  return(NO_NEXT_HOP);
}


void
LandmarkAgent::get_nbrinfo()
{
  ParentChildrenList *pcl;
  LMNode *pchild;
  int num_nbrs = 0;

  pcl = parent_children_list_;
  
  while(pcl->level_ != 1) 
    pcl = pcl->next_;

  assert(pcl);

  pchild = pcl->pchildren_;
  assert(pchild);

  while(pchild) {
    if(pchild->num_hops_ == 1)
      ++num_nbrs;
    pchild = pchild->next_;
  }
  
  trace("Node %d: Number of neighbours: %d",myaddr_,num_nbrs);
}

