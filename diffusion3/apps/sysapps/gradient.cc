//
// gradient.cc    : Gradient Filter
// author         : Fabio Silva and Chalermek Intanagonwiwat
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: gradient.cc,v 1.8 2002/03/21 19:30:54 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#include "gradient.hh"

#ifdef NS_DIFFUSION
static class GradientFilterClass : public TclClass {
public:
  GradientFilterClass() : TclClass("Application/GradientFilter") {}
  TclObject* create(int argc, const char*const* argv) {
    if (argc == 5)
      return(new GradientFilter(argv[4]));
    else
      fprintf(stderr, "Insufficient number of args for creating GradientFilter");
    return (NULL);
  }
} class_gradient_filter;

int GradientFilter::command(int argc, const char*const* argv) {
  
  if (argc == 3) {
    if (strcasecmp(argv[1], "dr") == 0) {
      DiffAppAgent *agent;
      agent = (DiffAppAgent *) TclObject::lookup(argv[2]);
      dr = agent->dr();
      start();
      return TCL_OK;
    }
    if (strcasecmp(argv[1], "debug") == 0) {
      global_debug_level = atoi(argv[2]);
      if (global_debug_level < 1 || global_debug_level > 10) {
	global_debug_level = DEBUG_DEFAULT;
	printf("Error:Debug level outside range(1-10) or missing !\n");
      }
    }
  }
  return Application::command(argc, argv);
}

#endif // NS_DIFFUSION

void GradientFilterReceive::recv(Message *msg, handle h)
{
  app->recv(msg, h);
}

int GradientTimerReceive::expire(handle hdl, void *p)
{
  return app->ProcessTimers(hdl, p);
}

void GradientTimerReceive::del(void *p)
{
  TimerType *timer;
  NRAttrVec *attrs;
  Message *msg;

  timer = (TimerType *) p;

  switch (timer->which_timer){

  case SUBSCRIPTION_TIMER:

    attrs = ((NRAttrVec *) timer->param);

    if (attrs){
      ClearAttrs(attrs);
      delete attrs;
    }

    break;

  case INTEREST_TIMER:
  case MESSAGE_SEND_TIMER:

    msg = ((Message *) timer->param);

    if (msg)
      delete msg;

    break;

  }

  delete timer;
}

int GradientFilter::ProcessTimers(handle hdl, void *p)
{
  TimerType *timer;
  NRAttrVec *attrs;
  Message *msg;
  int timeout = 0;

  timer = (TimerType *) p;
 
  switch (timer->which_timer){

  case GRADIENT_TIMER:

    GradientTimeout();

    break;
    
  case REINFORCEMENT_TIMER:

    ReinforcementTimeout();

    break;

  case SUBSCRIPTION_TIMER:

    attrs = ((NRAttrVec *) timer->param);

    timeout = SubscriptionTimeout(attrs);

    break;

  case INTEREST_TIMER:

    msg = ((Message *) timer->param);
    
    InterestTimeout(msg);

    // Cancel Timer
    timeout = -1;

    break;

  case MESSAGE_SEND_TIMER:

    msg = ((Message *) timer->param);

    MessageTimeout(msg);

    // Cancel Timer
    timeout = -1;

    break;

  default:

    diffPrint(DEBUG_IMPORTANT,
	      "Error: ProcessTimers received unknown timer %d !\n",
	      timer->which_timer);

    break;
  }

  return timeout;
}

void GradientFilter::InterestTimeout(Message *msg)
{
  diffPrint(DEBUG_MORE_DETAILS, "Interest Timeout !\n");

  msg->last_hop = LOCALHOST_ADDR;
  msg->next_hop = BROADCAST_ADDR;
 
  ((DiffusionRouting *)dr)->sendMessage(msg, filterHandle);
}

void GradientFilter::MessageTimeout(Message *msg)
{
  diffPrint(DEBUG_MORE_DETAILS, "Message Timeout !\n");

  ((DiffusionRouting *)dr)->sendMessage(msg, filterHandle);
}

void GradientFilter::GradientTimeout()
{
  RoutingList::iterator Ritr;
  AgentsList::iterator Aitr;
  Routing_Entry *Rentry;
  Agents_Entry *Aentry;
  struct timeval tmv;

  diffPrint(DEBUG_MORE_DETAILS, "Gradient Timeout !\n");

  getTime(&tmv);

  Ritr = routing_list.begin();

  while (Ritr != routing_list.end()){
    Rentry = *Ritr;

    // Step 1: Delete expired gradients
    Aitr = Rentry->active.begin();
    while (Aitr != Rentry->active.end()){
      Aentry = *Aitr;
      if (tmv.tv_sec > (Aentry->tv.tv_sec + GRADIENT_TIMEOUT)){

	diffPrint(DEBUG_NO_DETAILS, "Deleting Gradient to node %d !\n",
		  Aentry->node_addr);

	Aitr = Rentry->active.erase(Aitr);
	delete Aentry;
      }
      else{
	Aitr++;
      }
    }

    Aitr = Rentry->agents.begin();
    while (Aitr != Rentry->agents.end()){
      Aentry = *Aitr;
      if (tmv.tv_sec > (Aentry->tv.tv_sec + GRADIENT_TIMEOUT)){

	diffPrint(DEBUG_NO_DETAILS,
		  "Deleting Gradient to agent %d !\n", Aentry->port);

	Aitr = Rentry->agents.erase(Aitr);
	delete Aentry;
      }
      else{
	Aitr++;
      }
    }

    // Remove the Routing Entry if no gradients and no agents
    if ((Rentry->active.size() == 0) &&
	(Rentry->agents.size() == 0)){
      // Deleting Routing Entry
      diffPrint(DEBUG_DETAILS,
		"Nothing left for this data type, cleaning up !\n");
      Ritr = routing_list.erase(Ritr);
      delete Rentry;
    }
    else{
      Ritr++;
    }
  }
}

void GradientFilter::ReinforcementTimeout()
{
  RoutingList::iterator Ritr;
  AgentsList::iterator Aitr;
  Routing_Entry *Rentry;
  Agents_Entry *Aentry;
  Message *neg_rmsg;

  diffPrint(DEBUG_MORE_DETAILS, "Reinforcement Timeout !\n");

  Ritr = routing_list.begin();

  while (Ritr != routing_list.end()){
    Rentry = *Ritr;

    // Step 1: Delete expired gradients
    Aitr = Rentry->data_neighbors.begin();
    while (Aitr != Rentry->data_neighbors.end()){
      Aentry = *Aitr;
      if (Aentry->neighbor_flag == OLD_MESSAGE){
	neg_rmsg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
			       0, 0, Rentry->attrs->size(), pkt_count,
			       rdm_id, Aentry->node_addr, LOCALHOST_ADDR);
	neg_rmsg->msg_attr_vec = CopyAttrs(Rentry->attrs);

	diffPrint(DEBUG_NO_DETAILS,
		  "Sending Negative Reinforcement to node %d !\n",
		  Aentry->node_addr);

	((DiffusionRouting *)dr)->sendMessage(neg_rmsg, filterHandle);

	pkt_count++;
	delete neg_rmsg;

	// Done. Delete entry
	Aitr = Rentry->data_neighbors.erase(Aitr);
	delete Aentry;
      }
      else{
	Aitr++;
      }
    }

    // Step 2: Delete data neighbors with no activity, zero flags
    Aitr = Rentry->data_neighbors.begin();
    while (Aitr != Rentry->data_neighbors.end()){
      Aentry = *Aitr;
      if (Aentry->neighbor_flag == NEW_MESSAGE){
	Aentry->neighbor_flag = 0;
	Aitr++;
      }
      else{
	// Delete entry
	Aitr = Rentry->data_neighbors.erase(Aitr);
	delete Aentry;
      }
    }

    // Advance to the next routing entry
    Ritr++;
  }
}

int GradientFilter::SubscriptionTimeout(NRAttrVec *attrs)
{
  Routing_Entry *entry;
  AttributeList::iterator itr;
  Attribute_Entry *ae;
  struct timeval tmv;

  diffPrint(DEBUG_MORE_DETAILS, "Subscription Timeout !\n");

  getTime(&tmv);

  // Find the correct Routing Entry
  entry = FindPerfectMatch(attrs);

  if (entry){
    // Step 1: Check Timeouts

    itr = entry->attr_list.begin();

    while (itr != entry->attr_list.end()){
      ae = *itr;
      if (tmv.tv_sec > (ae->tv.tv_sec + SUBSCRIPTION_TIMEOUT)){
	SendDisinterest(ae->attrs, entry);
	itr = entry->attr_list.erase(itr);
	delete ae;
      }
      else{
	itr++;
      }
    }
  }
  else{
    diffPrint(DEBUG_DETAILS, "Warning: SubscriptionTimeout could't find RE - maybe deleted by GradientTimeout ?\n");

    // Cancel Timer
    return -1;
  }

  // Keep Timer
  return 0;
}

void GradientFilter::DeleteRoutingEntry(Routing_Entry *entry)
{
  RoutingList::iterator itr;
  Routing_Entry *e;

  for (itr = routing_list.begin(); itr != routing_list.end(); ++itr){
    e = *itr;
    if (e == entry){
      itr = routing_list.erase(itr);
      delete entry;
      return;
    }
  }
  diffPrint(DEBUG_ALWAYS, "Error: DeleteRoutingEntry could not find entry to delete !\n");
}

Routing_Entry * GradientFilter::FindRoutingEntry(NRAttrVec *attrs, RoutingList::iterator start, RoutingList::iterator *place)
{
  RoutingList::iterator itr;
  Routing_Entry *entry;

  for (itr = start; itr != routing_list.end(); ++itr){
    entry = *itr;
    if (MatchAttrs(entry->attrs, attrs)){
      *place = itr;
      return entry;
    }
  }
  return NULL;
}

Routing_Entry * GradientFilter::FindPerfectMatch(NRAttrVec *attrs)
{
  RoutingList::iterator itr;
  Routing_Entry *entry;

  for (itr = routing_list.begin(); itr != routing_list.end(); ++itr){
    entry = *itr;
    if (PerfectMatch(entry->attrs, attrs))
      return entry;
  }
  return NULL;
}

Attribute_Entry * GradientFilter::FindMatchingSubscription(Routing_Entry *entry,
							NRAttrVec *attrs)
{
  AttributeList::iterator itr;
  Attribute_Entry *ae;

  for (itr = entry->attr_list.begin(); itr != entry->attr_list.end(); ++itr){
    ae = *itr;
    if (PerfectMatch(ae->attrs, attrs))
      return ae;
  }
  return NULL;
}

void GradientFilter::UpdateGradient(Routing_Entry *entry, int32_t last_hop, bool reinforced)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;

  for (itr = entry->active.begin(); itr != entry->active.end(); ++itr){
    ae = *itr;
    if (ae->node_addr == last_hop){
      getTime(&(ae->tv));
      if (reinforced)
	ae->reinforced = true;
      return;
    }
  }

  // We need to add a new gradient
  ae = new Agents_Entry;
  ae->node_addr = last_hop;
  ae->port = 0;
  if (reinforced)
    ae->reinforced = true;

  entry->active.push_back(ae);
}

void GradientFilter::UpdateAgent(Routing_Entry *entry, u_int16_t source_port)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;

  for (itr = entry->agents.begin(); itr != entry->agents.end(); ++itr){
    ae = *itr;
    if (ae->port == source_port){
      // We already have this guy
      getTime(&(ae->tv));
      return;
    }
  }

  // This is a new Agent
  ae = new Agents_Entry;
  ae->node_addr = LOCALHOST_ADDR;
  ae->port = source_port;

  entry->agents.push_back(ae);
}

void GradientFilter::ForwardData(Message *msg, Routing_Entry *entry,
			      bool &reinforced)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;
  struct timeval tmv;
  Message *mymsg, *neg_rmsg, *pos_rmsg;
  TimerType *dtimer;
  unsigned int key[2];
  Hash_Entry *hEntry;
  bool has_sink = false;
  ReinforcementMessage *reinforcementblob;
  NRAttribute *reinfmsg;
  NRAttrVec *attrs;

  mymsg = CopyMessage(msg);
  getTime(&tmv);

  // Step 1: Sink Processing
  for (itr = entry->agents.begin(); itr != entry->agents.end(); ++itr){
    ae = *itr;

    has_sink = true;

    // Forward DATA to local agents
    mymsg->next_hop = LOCALHOST_ADDR;
    mymsg->next_port = ae->port;

    ((DiffusionRouting *)dr)->sendMessage(mymsg, filterHandle);
  }

  // Reset next_port to zero so message goes to other nodes
  mymsg->next_port = 0;

  if (msg->msg_type == EXPLORATORY_DATA && !reinforced &&
      entry->agents.size() > 0){

    // Just send a single reinforcement per new exploratory data message
    reinforced = true;
     
    // Send reinforcement to 'last_hop'
    reinforcementblob = new ReinforcementMessage;
    reinforcementblob->rdm_id = msg->rdm_id;
    reinforcementblob->pkt_num = msg->pkt_num;

    reinfmsg = ReinforcementAttr.make(NRAttribute::IS, (void *)reinforcementblob, sizeof(ReinforcementMessage));
    
    attrs = CopyAttrs(msg->msg_attr_vec);
    attrs->push_back(reinfmsg);

    pos_rmsg = new Message(DIFFUSION_VERSION, POSITIVE_REINFORCEMENT, 0, 0,
			   attrs->size(), pkt_count, rdm_id, msg->last_hop,
			   LOCALHOST_ADDR);
    pos_rmsg->msg_attr_vec = CopyAttrs(attrs);

    diffPrint(DEBUG_NO_DETAILS,
	      "Sending Positive Reinforcement to node %d !\n",
	      msg->last_hop);

    // Create timer for sending this message later
    dtimer = new TimerType(MESSAGE_SEND_TIMER);
    dtimer->param = (void *) pos_rmsg;

    // Add timer for forwarding the data packet
    ((DiffusionRouting *)dr)->addTimer(POS_REINFORCEMENT_SEND_DELAY +
				       (int) ((POS_REINFORCEMENT_JITTER * (getRand() * 1.0 / RAND_MAX) - (POS_REINFORCEMENT_JITTER / 2))),
				       (void *) dtimer, tcb);

    pkt_count++;
    ClearAttrs(attrs);
    delete reinforcementblob;
  }

  // Step 2: Source Processing
  if (msg->last_hop == (int)LOCALHOST_ADDR){
    if (tmv.tv_sec >= entry->tv.tv_sec){
      getTime(&(entry->tv));
      entry->tv.tv_sec = entry->tv.tv_sec + EXPLORATORY_MESSAGE_DELAY;
      mymsg->msg_type = EXPLORATORY_DATA;
    }
  }

  // Step 3: Intermediate Processing
  SetReinforcementFlags(entry, msg->last_hop, NEW_MESSAGE);

  // Add EXPLORATORY_DATA messages to hash table
  if (msg->msg_type == EXPLORATORY_DATA){
    key[0] = msg->pkt_num;
    key[1] = msg->rdm_id;

    hEntry = new Hash_Entry;
    hEntry->last_hop = msg->last_hop;

    PutHash(hEntry, key[0], key[1]);
  }

  if (mymsg->msg_type == EXPLORATORY_DATA){
#ifdef USE_BROADCAST_MAC
    if (entry->active.size() > 0){
      // Broadcast DATA message
      dmsg = CopyMessage(mymsg);
      dmsg->next_hop = BROADCAST_ADDR;

      dtimer = new TimerType(MESSAGE_SEND_TIMER);
      dtimer->param = (void *) dmsg;

      // Add timer for forwarding the data packet
      ((DiffusionRouting *)dr)->addTimer(DATA_FORWARD_DELAY +
					 (int) ((DATA_FORWARD_JITTER * (getRand() * 1.0 / RAND_MAX) - (DATA_FORWARD_JITTER / 2))),
					 (void *) dtimer, tcb);
    }
#else
    // Forward DATA to all output gradients
    for (itr = entry->active.begin(); itr != entry->active.end(); ++itr){
      ae = *itr;

      mymsg->next_hop = ae->node_addr;

      ((DiffusionRouting *)dr)->sendMessage(mymsg, filterHandle);
    }
#endif //USE_BROADCAST_MAC
  }
  else{
    // Forward DATA only to reinforced gradients
    itr = entry->active.begin();
    ae = FindReinforcedGradients(&entry->active, itr, &itr);

    if (ae){
      while (ae){
	// Found reinforced path
	mymsg->next_hop = ae->node_addr;

	diffPrint(DEBUG_NO_DETAILS,
		  "Forwarding data through Reinforced Gradient to node %d !\n",
		  ae->node_addr);

	((DiffusionRouting *)dr)->sendMessage(mymsg, filterHandle);

	// Move to the next one
	itr++;

	ae = FindReinforcedGradients(&entry->active, itr, &itr);
      }
    }
    else{
      // No reinforced path found, send negative
      // reinforcement to last_hop
      if ((!has_sink) && (msg->last_hop != (int)LOCALHOST_ADDR)){
	neg_rmsg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
			       0, 0, entry->attrs->size(), pkt_count,
			       rdm_id, msg->last_hop, LOCALHOST_ADDR);
	neg_rmsg->msg_attr_vec = CopyAttrs(entry->attrs);

	diffPrint(DEBUG_NO_DETAILS,
		  "Sending Negative Reinforcement to node %d !\n",
		  msg->last_hop);

	((DiffusionRouting *)dr)->sendMessage(neg_rmsg, filterHandle);

	pkt_count++;
	delete neg_rmsg;
      }
    }
  }

  delete mymsg;
}

Agents_Entry * GradientFilter::FindReinforcedGradients(AgentsList *agents, AgentsList::iterator start, AgentsList::iterator *place)
{
  AgentsList::iterator itr;
  Agents_Entry *entry;

  for (itr = start; itr != agents->end(); ++itr){
    entry = *itr;
    if (entry->reinforced){
      *place = itr;
      return entry;
    }
  }
  return NULL;
}

Agents_Entry * GradientFilter::FindReinforcedGradient(int32_t node_addr, Routing_Entry *entry)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;

  itr = entry->active.begin();
  ae = FindReinforcedGradients(&entry->active, itr, &itr);

  if (ae){
    while(ae){
      if (ae->node_addr == node_addr)
	return ae;

      // Not the one we're looking for
      itr++;
      ae = FindReinforcedGradients(&entry->active, itr, &itr);
    }
  }

  return NULL;
}

void GradientFilter::DeleteGradient(Routing_Entry *entry, Agents_Entry *ae)
{
  AgentsList::iterator itr;
  Agents_Entry *e;

  for (itr = entry->active.begin(); itr != entry->active.end(); ++itr){
    e = *itr;
    if (e == ae){
      itr = entry->active.erase(itr);
      delete ae;
      return;
    }
  }
  diffPrint(DEBUG_ALWAYS,
	    "Error: DeleteGradient could not find gradient to delete !\n");
}

void GradientFilter::SetReinforcementFlags(Routing_Entry *entry, int32_t last_hop, int new_message)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;

  // Do nothing if message is coming from a local source
  if (last_hop == (int)LOCALHOST_ADDR)
    return;

  for (itr = entry->data_neighbors.begin();
       itr != entry->data_neighbors.end(); ++itr){
    ae = *itr;
    if (ae->node_addr == last_hop){
      if (ae->neighbor_flag > 0)
	return;
      ae->neighbor_flag = new_message;
      return;
    }
  }

  // We need to add a new data neighbor
  ae = new Agents_Entry;
  ae->node_addr = last_hop;
  ae->neighbor_flag = new_message;

  entry->data_neighbors.push_back(ae);
}

void GradientFilter::SendInterest(NRAttrVec *attrs, Routing_Entry *entry)
{
  AgentsList::iterator itr;
  Agents_Entry *ae;

  Message *msg = new Message(DIFFUSION_VERSION, INTEREST, 0, 0,
			     attrs->size(), 0, 0, LOCALHOST_ADDR,
			     LOCALHOST_ADDR);

  msg->msg_attr_vec = CopyAttrs(attrs);

  for (itr = entry->agents.begin(); itr != entry->agents.end(); ++itr){
    ae = *itr;

    msg->next_port = ae->port;

    ((DiffusionRouting *)dr)->sendMessage(msg, filterHandle);
  }

  delete msg;
}

void GradientFilter::SendDisinterest(NRAttrVec *attrs, Routing_Entry *entry)
{
  NRAttrVec *newAttrs;
  NRSimpleAttribute<int> *nrclass = NULL;

  newAttrs = CopyAttrs(attrs);

  nrclass = NRClassAttr.find(newAttrs);
  if (!nrclass){
    diffPrint(DEBUG_ALWAYS,
	      "Error: SendDisinterest couldn't find the class attribute !\n");
    ClearAttrs(newAttrs);
    delete newAttrs;
    return;
  }

  // Change the class_key value
  nrclass->setVal(NRAttribute::DISINTEREST_CLASS);

  SendInterest(newAttrs, entry);
   
  ClearAttrs(newAttrs);
  delete newAttrs;
}

void GradientFilter::recv(Message *msg, handle h)
{
  if (h != filterHandle){
    diffPrint(DEBUG_ALWAYS,
	      "Error: received msg for handle %d, subscribed to handle %d !\n",
	      h, filterHandle);
    return;
  }

  if (msg->new_message == 1)
    ProcessNewMessage(msg);
  else
    ProcessOldMessage(msg);
}

void GradientFilter::ProcessOldMessage(Message *msg)
{
  Routing_Entry *entry;
  RoutingList::iterator place;

  switch (msg->msg_type){

  case INTEREST:

    diffPrint(DEBUG_NO_DETAILS, "Received Old Interest !\n");

    if (msg->last_hop == (int)LOCALHOST_ADDR){
      // Old interest should not come from local agent
      diffPrint(DEBUG_ALWAYS, "Warning: Old Interest from local agent !\n");
      break;
    }

    // Get the routing entry for these attrs      
    entry = FindPerfectMatch(msg->msg_attr_vec);
    if (entry)
      UpdateGradient(entry, msg->last_hop, false);

    break;

  case DATA: 

    diffPrint(DEBUG_NO_DETAILS, "Received an old Data message !\n");

    // Find the correct routing entry
    place = routing_list.begin();
    entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);

    while (entry){
      diffPrint(DEBUG_NO_DETAILS,
		"Set flags to %d to OLD_MESSAGE !\n", msg->last_hop);

      SetReinforcementFlags(entry, msg->last_hop, OLD_MESSAGE);
      place++;
      entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);
    }

    break;

  case EXPLORATORY_DATA:

    // Just drop it
    diffPrint(DEBUG_NO_DETAILS,
	      "Received an old Exploratory Data. Loop detected !\n");

    break;

  case POSITIVE_REINFORCEMENT:

    diffPrint(DEBUG_IMPORTANT, "Received an old Positive Reinforcement !\n");

    break;

  case NEGATIVE_REINFORCEMENT:

    diffPrint(DEBUG_IMPORTANT, "Received an old Negative Reinforcement !\n");

    diffPrint(DEBUG_IMPORTANT, "pkt_num = %d, rdm_id = %d !\n",
	      msg->pkt_num, msg->rdm_id);

    break;

  default: 

    break;
  }
}

void GradientFilter::ProcessNewMessage(Message *msg)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;
  Routing_Entry *entry;
  RoutingList::iterator place;
  Attribute_Entry *ae;
  Agents_Entry *aentry;
  AgentsList::iterator al_itr;
  Message *neg_rmsg;
  bool new_data_type = false;
  bool reinforced = false;
  Hash_Entry *hEntry;
  unsigned int key[2];
  ReinforcementMessage *reinforcementMsg;
  NRSimpleAttribute<void *> *reinforcementAttr = NULL;

  switch (msg->msg_type){

  case INTEREST:

    diffPrint(DEBUG_NO_DETAILS, "Received Interest !\n");

    // Step 1: Look for the same data type
    entry = FindPerfectMatch(msg->msg_attr_vec);

    if (!entry){
      // Create a new routing entry for this data type
      entry = new Routing_Entry;
      entry->attrs = CopyAttrs(msg->msg_attr_vec);
      routing_list.push_back(entry);
      new_data_type = true;
    }

    if (msg->last_hop == (int)LOCALHOST_ADDR){
      // From local agent
      UpdateAgent(entry, msg->source_port);
    }
    else{
      // From outside, we just add the new gradient
      UpdateGradient(entry, msg->last_hop, false);
    }

    nrclass = NRClassAttr.find(msg->msg_attr_vec);
    nrscope = NRScopeAttr.find(msg->msg_attr_vec);

    if (!nrclass || !nrscope){
      diffPrint(DEBUG_ALWAYS, "Warning: Can't find CLASS and/or SCOPE attributes in Interest message !\n");
      return;
    }

    if ((nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
	(nrclass->getOp() == NRAttribute::IS) &&
	(nrscope->getVal() == NRAttribute::GLOBAL_SCOPE)){
      // Forward this interest

      TimerType *timer;

      timer = new TimerType(INTEREST_TIMER);
      timer->param = (void *) CopyMessage(msg);

      ((DiffusionRouting *)dr)->addTimer(INTEREST_FORWARD_DELAY +
					 (int) ((INTEREST_FORWARD_JITTER * (getRand() * 1.0 / RAND_MAX) - (INTEREST_FORWARD_JITTER / 2))),
					 (void *) timer, tcb);

    }
    else{
      if ((nrclass->getOp() != NRAttribute::IS) &&
	  (nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE) &&
	  (new_data_type)){

	TimerType *timer;

	timer = new TimerType(SUBSCRIPTION_TIMER);
	timer->param = (void *) CopyAttrs(msg->msg_attr_vec);

	((DiffusionRouting *)dr)->addTimer(SUBSCRIPTION_DELAY +
					   (int) (SUBSCRIPTION_DELAY * (getRand() * 1.0 / RAND_MAX)),
					   (void *) timer, tcb);
      }

      // Subscriptions don't have to match other subscriptions
      break;
    }

    // Step 2: Match other routing tables
    place = routing_list.begin();
    entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);

    while (entry){
      // Got a match
      ae = FindMatchingSubscription(entry, msg->msg_attr_vec);

      // Do we already have this subscription
      if (ae){
	getTime(&(ae->tv));
      }
      else{
	ae = new Attribute_Entry;
	ae->attrs = CopyAttrs(msg->msg_attr_vec);
	entry->attr_list.push_back(ae);
	SendInterest(ae->attrs, entry);
      }
      // Move to the next Routing_Entry
      place++;

      entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);
    }

      break;

  case DATA:
  case EXPLORATORY_DATA:

    if (msg->msg_type == DATA)
      diffPrint(DEBUG_NO_DETAILS, "Received Data !\n");
    else
      diffPrint(DEBUG_NO_DETAILS, "Received Exploratory Data !\n");

    // Find the correct routing entry
    place = routing_list.begin();
    entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);

    while (entry){
      ForwardData(msg, entry, reinforced);
      place++;
      entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);
    }

    break;

  case POSITIVE_REINFORCEMENT:

    diffPrint(DEBUG_NO_DETAILS, "Received a Positive Reinforcement !\n");

    reinforcementAttr = ReinforcementAttr.find(msg->msg_attr_vec);
    if (!reinforcementAttr){
      diffPrint(DEBUG_ALWAYS, "Error: Received an invalid Positive Reinforcement message !\n");
      return;
    }

    reinforcementMsg = (ReinforcementMessage *) reinforcementAttr->getVal();

    key[0] = reinforcementMsg->pkt_num;
    key[1] = reinforcementMsg->rdm_id;

    hEntry = GetHash(key[0], key[1]);

    // Find routing entries that match this data type
    place = routing_list.begin();
    entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);

    while (entry){
      // Reinforce gradients to last_hop
      UpdateGradient(entry, msg->last_hop, true);

      place++;
      entry = FindRoutingEntry(msg->msg_attr_vec, place, &place);
    }

    // If we have no record of this message that either because
    // we originated the message (in which case, no further action
    // is required) or because we dropped it a long time ago because
    // of our hashing configuration parameters (in this case, we can't
    // do anything).
    if (hEntry){
      msg->next_hop = hEntry->last_hop;

      diffPrint(DEBUG_NO_DETAILS,
		"Forwarding Positive Reinforcement to node %d !\n",
		hEntry->last_hop);

      ((DiffusionRouting *)dr)->sendMessage(msg, filterHandle);
    }

    break;

  case NEGATIVE_REINFORCEMENT:

    diffPrint(DEBUG_NO_DETAILS, "Received a Negative Reinforcement !\n");

    entry = FindPerfectMatch(msg->msg_attr_vec);
    if (entry){
      aentry = FindReinforcedGradient(msg->last_hop, entry);
      if (aentry){
	// Remove reinforced gradient to last_hop
	DeleteGradient(entry, aentry);

	aentry = FindReinforcedGradients(&entry->active, entry->active.begin(),
					 &al_itr);

	// If there are no other reinforced outgoing gradients
	// we need to send our own negative reinforcement
	if (!aentry){
	  neg_rmsg = new Message(DIFFUSION_VERSION, NEGATIVE_REINFORCEMENT,
				 0, 0, entry->attrs->size(), pkt_count,
				 rdm_id, BROADCAST_ADDR, LOCALHOST_ADDR);
	  neg_rmsg->msg_attr_vec = CopyAttrs(entry->attrs);

	  diffPrint(DEBUG_NO_DETAILS,
		    "Forwarding Negative Reinforcement to ALL !\n");

	  ((DiffusionRouting *)dr)->sendMessage(neg_rmsg, filterHandle);

	  pkt_count++;
	  delete neg_rmsg;
	}
      }
    }

    break;

  default:

    break;
  }
}

Hash_Entry * GradientFilter::GetHash(unsigned int pkt_num,
				  unsigned int rdm_id)
{
   unsigned int key[2];
   
   key[0] = pkt_num;
   key[1] = rdm_id;
   
   Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);
   
   if (entryPtr == NULL)
      return NULL;
   
   return (Hash_Entry *)Tcl_GetHashValue(entryPtr);
}

void GradientFilter::PutHash(Hash_Entry *newHashPtr,
			  unsigned int pkt_num,
			  unsigned int rdm_id)
{
   Tcl_HashEntry *entryPtr;
   Hash_Entry    *hashPtr;
   HashList::iterator itr;
   unsigned int key[2];
   int newPtr;
 
   if (hash_list.size() == HASH_TABLE_DATA_MAX_SIZE){
      // Hash table reached maximum size
      
      for (int i = 0; ((i < HASH_TABLE_DATA_REMOVE_AT_ONCE)
		       && (hash_list.size() > 0)); i++){
	 itr = hash_list.begin();
	 entryPtr = *itr;
	 hashPtr = (Hash_Entry *) Tcl_GetHashValue(entryPtr);
	 delete hashPtr;
	 hash_list.erase(itr);
	 Tcl_DeleteHashEntry(entryPtr);
      }
   }
  
   key[0] = pkt_num;
   key[1] = rdm_id;
   
   entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);

   if (newPtr == 0){
      diffPrint(DEBUG_IMPORTANT, "Key already exists in hash !\n");
      return;
   }

   Tcl_SetHashValue(entryPtr, newHashPtr);

   hash_list.push_back(entryPtr);
}

handle GradientFilter::setupFilter()
{
  NRAttrVec attrs;
  handle h;

  // This is a dummy attribute for filtering
  // It matches everything
  attrs.push_back(NRClassAttr.make(NRAttribute::IS, NRAttribute::INTEREST_CLASS));

  h = ((DiffusionRouting *)dr)->addFilter(&attrs, GRADIENT_FILTER_PRIORITY, fcb);

  ClearAttrs(&attrs);
  return h;
}

#ifndef NS_DIFFUSION
void GradientFilter::run()
{
  // Doesn't do anything
  while (1){
    sleep(1000);
  }
}
#endif // NS_DIFFUSION

#ifdef NS_DIFFUSION
GradientFilter::GradientFilter(const char *diffrtg)
{
  DiffAppAgent *agent;
#else
GradientFilter::GradientFilter(int argc, char **argv)
{
#endif // NS_DIFFUSION
  struct timeval tv;

  getTime(&tv);
  setSeed(&tv);
  pkt_count = getRand();
  rdm_id = getRand();

  // Create Diffusion Routing class
#ifdef NS_DIFFUSION
  agent = (DiffAppAgent *)TclObject::lookup(diffrtg);
  dr = agent->dr();
#else
  ParseCommandLine(argc, argv);
  dr = NR::createNR(diffusion_port);
#endif // NS_DIFFUSION

  // Create callback classes and set up pointers
  fcb = new GradientFilterReceive(this);
  tcb = new GradientTimerReceive(this);

  // Initialize Hashing structures
  Tcl_InitHashTable(&htable, 2);

  // Set up the filter
  filterHandle = setupFilter();

  // Print filter information
  diffPrint(DEBUG_IMPORTANT,
	    "Gradient filter subscribed to *, received handle %d\n",
	    filterHandle);

  // Add the Gradient Timer
  TimerType *timer;

  timer = new TimerType(GRADIENT_TIMER);
  ((DiffusionRouting *)dr)->addTimer(GRADIENT_DELAY, (void *) timer, tcb);

  timer = new TimerType(REINFORCEMENT_TIMER);
  ((DiffusionRouting *)dr)->addTimer(REINFORCEMENT_DELAY, (void *) timer, tcb);

  getTime(&tv);

  diffPrint(DEBUG_ALWAYS, "Gradient filter initialized at time %ld:%ld!\n",
	    tv.tv_sec, tv.tv_usec);
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  GradientFilter *app;

  // Initialize and run the Gradient Filter
  app = new GradientFilter(argc, argv);
  app->run();

  return 0;
}
#endif // NS_DIFFUSION
