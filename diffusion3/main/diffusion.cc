// 
// diffusion.cc  : Main Diffusion program
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: diffusion.cc,v 1.3 2001/11/29 23:25:31 haldar Exp $
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
// *******************************************************************
// Ported from SCADDS group's implementation of directed diffusion 
// into ns-2. Padma Haldar, nov 2001.
// ********************************************************************

#ifdef NS_DIFFUSION

#include "diffusion.hh"

#ifdef NS_DIFFUSION
#include "diffrtg.h"
#endif //ns


extern int global_debug_level;

#ifdef SCADDS
DiffusionCoreAgent *agent;

void signal_handler(int p)
{
  agent->TimetoStop();
  exit(0);
}
#endif //scadds


#ifdef NS_DIFFUSION

// ********************************************************************
// All major changes pertaining to NS go under here. 
// Note all changes specific for NS have been marked off by 
// ifdef NS_DIFFUSION statements.
// scadds specific code not applicable to ns has been marked off
// with ifdef SCADDS statements
// ********************************************************************


#endif //ns


void DiffusionCoreAgent::NeighborsTimeOut()
{
  struct timeval tmv;
  Neighbor_Entry *myNeighbor;
  NeighborList::iterator itr;

  diffPrint(DEBUG_MORE_DETAILS, "Neighbors Timeout !\n");

  getTime(&tmv);

  itr = my_neighbors.begin();

  while(itr != my_neighbors.end()){
    myNeighbor = *itr;
    if (tmv.tv_sec > myNeighbor->tmv.tv_sec + NEIGHBORS_TIMEOUT){
      // This neighbor expired
      itr = my_neighbors.erase(itr);
      delete myNeighbor;
    }
    else{
      itr++;
    }
  }

  // Re-schedule this timer
  eq->eq_addAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
}

void DiffusionCoreAgent::FilterTimeOut()
{
  char *debug_str;
  struct timeval tmv;
  Filter_Entry *myFilter;
  FilterList::iterator itr;

  diffPrint(DEBUG_MORE_DETAILS, "Filter Timeout !\n");

  getTime(&tmv);

  itr = filter_list.begin();

  while(itr != filter_list.end()){
    myFilter = *itr;
    if (tmv.tv_sec > myFilter->tmv.tv_sec + FILTER_TIMEOUT){

      // This filter expired
      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "Filter %d, %d, %d timed out !\n", myFilter->agent,
	      myFilter->handle, myFilter->priority);
      diffPrint(DEBUG_NO_DETAILS, debug_str);
      delete [] debug_str;

      itr = filter_list.erase(itr);
      delete myFilter;
    }
    else{
      itr++;
    }
  }

  // Re-Schedule this timer
  eq->eq_addAfter(FILTER_TIMER, NULL, FILTER_DELAY);
}

void DiffusionCoreAgent::SendMessage(Message *msg, u_int16_t port)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  DeviceList::iterator itr;
  Tcl_HashEntry *entryPtr;
  unsigned int key[2];
  int len;
  char *pos;
  
  out_pkt = AllocateBuffer(msg->msg_attr_vec);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  // Clear the buffer
  len = CalculateSize(msg->msg_attr_vec);
  bzero(out_pkt, (len + sizeof(struct hdr_diff)));
  len = PackAttrs(msg->msg_attr_vec, pos);

  // Adjust message size for logging and check hash
  msg->data_len = len;
  key[0] = msg->pkt_num;
  key[1] = msg->rdm_id;
  entryPtr = Tcl_FindHashEntry(&htable, (char *) key);
  if (entryPtr)
    msg->new_message = 0;
  else
    msg->new_message = 1;

  NEXT_HOP(dfh) = htonl(msg->next_hop);
  NUM_ATTR(dfh) = htons(msg->msg_attr_vec->size());
  SRC_PORT(dfh) = htons(diffusion_port);
  MSG_TYPE(dfh) = msg->msg_type;
  VERSION(dfh)  = DIFFUSION_VERSION;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh)  = htonl(msg->pkt_num);
  RDM_ID(dfh)   = htonl(msg->rdm_id);

  // Check if message goes to an agent or the network
  if (port){
    // Message goes to an agent
    LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);

    if (msg->next_hop != LOCALHOST_ADDR){
      diffPrint(DEBUG_ALWAYS, "Error: DiffusionCoreAgent::SendMessage: message destination is a local agent but next_hop != LOCALHOST_ADDR !\n");
      return;
    }

    for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
      (*itr)->SendPacket(out_pkt, sizeof(struct hdr_diff) + len, port);
    }
  }
  else{
    // Message goes to the network
    LAST_HOP(dfh) = htonl(myId);

#ifdef STATS
    stats->logOutgoingMessage(msg);
#endif

    // Add message to the hash table      
    if (entryPtr == NULL)
      PutHash(key[0], key[1]);
    else
      diffPrint(DEBUG_DETAILS, "Message being sent is an old message !\n");

    // Send Message
    dvi_send((char *) out_pkt, sizeof(struct hdr_diff) + len);
  }

  delete [] out_pkt;

}

void DiffusionCoreAgent::ForwardMessage(Message *msg, Filter_Entry *dst)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  DeviceList::iterator itr;
  RedirectMessage *originalHdr;
  NRAttribute *originalAttr;
  int len;
  char *pos;

  // Create an attribute with the original header
  originalHdr = new RedirectMessage;
  originalHdr->msg_type = msg->msg_type;
  originalHdr->source_port = msg->source_port;
  originalHdr->data_len = msg->data_len;
  originalHdr->num_attr = msg->num_attr;
  originalHdr->pkt_num = msg->pkt_num;
  originalHdr->rdm_id = msg->rdm_id;
  originalHdr->next_hop = msg->next_hop;
  originalHdr->last_hop = msg->last_hop;
  originalHdr->new_message = msg->new_message;
  originalHdr->handle = dst->handle;

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  msg->msg_attr_vec->push_back(originalAttr);

  out_pkt = AllocateBuffer(msg->msg_attr_vec);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec, pos);

  LAST_HOP(dfh) = htonl(myId);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(msg->num_attr + 1);
  PKT_NUM(dfh) = htonl(pkt_count);
  RDM_ID(dfh) = htonl(rdm_id);
  SRC_PORT(dfh) = htons(diffusion_port);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = REDIRECT;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->SendPacket(out_pkt, sizeof(hdr_diff) + len, dst->agent);
  }

  pkt_count++;
  delete originalHdr;

  delete [] out_pkt;

}


void DiffusionCoreAgent::UpdateNeighbors(int id)
{
  NeighborList::iterator itr;
  Neighbor_Entry *newNeighbor;

  if (id == LOCALHOST_ADDR || id == myId)
    return;

  for (itr = my_neighbors.begin(); itr != my_neighbors.end(); ++itr){
    if ((*itr)->id == id)
      break;
  }

  if (itr == my_neighbors.end()){
    // This is a new neighbor
    newNeighbor = new Neighbor_Entry(id);
    my_neighbors.push_front(newNeighbor);
  }
  else{
    // Just update the neighbor timeout
    getTime(&((*itr)->tmv));
  }
}


Filter_Entry * DiffusionCoreAgent::FindFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator itr;
  Filter_Entry *current;

  for (itr = filter_list.begin(); itr != filter_list.end(); ++itr){
    current = *itr;
    if (handle != current->handle || agent != current->agent)
      continue;

    // Found
    return current;
  }
  return NULL;
}


Filter_Entry * DiffusionCoreAgent::DeleteFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator itr = filter_list.begin();
  Filter_Entry *current = NULL;

  while (itr != filter_list.end()){
    current = *itr;
    if (handle == current->handle && agent == current->agent){
      filter_list.erase(itr);
      break;
    }
    current = NULL;
    itr++;
  }
  return current;
}


void DiffusionCoreAgent::AddFilter(NRAttrVec *attrs, u_int16_t agent,
			       int16_t handle, u_int16_t priority)
{
  Filter_Entry *entry;

  entry = new Filter_Entry(handle, priority, agent);

  // Copy the Attribute Vector
  entry->filterAttrs = CopyAttrs(attrs);

  // Add this filter to the filter list
  filter_list.push_back(entry);
}


FilterList::iterator DiffusionCoreAgent::FindMatchingFilter(NRAttrVec *attrs,
							FilterList::iterator itr)
{
  Filter_Entry *entry;

  for (;itr != filter_list.end(); ++itr){
    entry = *itr;

    if (OneWayMatch(entry->filterAttrs, attrs)){
      // That's a match !
      break;
    }
  }
  return itr;
}


bool DiffusionCoreAgent::RestoreOriginalHeader(Message *msg)
{
  NRAttrVec::iterator place = msg->msg_attr_vec->begin();
  NRSimpleAttribute<void *> *originalHeader = NULL;
  RedirectMessage *originalHdr;

  // Find original Header
  originalHeader = OriginalHdrAttr.find_from(msg->msg_attr_vec, place, &place);
  if (!originalHeader){
    diffPrint(DEBUG_ALWAYS, "Error: DiffusionCoreAgent::ProcessControlMessage couldn't find the OriginalHdrAttr !\n");
    return false;
  }

  // Restore original Header
  originalHdr = (RedirectMessage *) originalHeader->getVal();

  msg->msg_type = originalHdr->msg_type;
  msg->source_port = originalHdr->source_port;
  msg->pkt_num = originalHdr->pkt_num;
  msg->rdm_id = originalHdr->rdm_id;
  msg->next_hop = originalHdr->next_hop;
  msg->last_hop = originalHdr->last_hop;
  msg->new_message = originalHdr->new_message;
  msg->num_attr = originalHdr->num_attr;
  msg->data_len = originalHdr->data_len;

  // Delete attribute from original set
  msg->msg_attr_vec->erase(place);
  delete originalHeader;

  return true;
}


FilterList * DiffusionCoreAgent::GetFilterList(NRAttrVec *attrs)
{
  FilterList *my_list = new FilterList;
  FilterList::iterator itr, myItr;
  Filter_Entry *entry, *listEntry;

  // We need to come up with a list of filters to call
  // F1 will be called before F2 if F1->priority > F2->priority
  // If F1 and F2 have the same priority, F1 will be called
  // before F2 is F1->handle < F2->handle. If both handles
  // are equal, the one with the lower port number will be
  // called first.

  itr = FindMatchingFilter(attrs, filter_list.begin());

  while (itr != filter_list.end()){
    // We have a match
    entry = *itr;

    for (myItr = my_list->begin(); myItr != my_list->end(); ++myItr){
      listEntry = *myItr;

      // Figure out where to insert 
      if (entry->priority > listEntry->priority)
	break;

      if (entry->priority == listEntry->priority){
	if (entry->handle < listEntry->handle)
	  break;

	if (entry->handle == listEntry->handle)
	  if (entry->agent < listEntry->agent)
	    break;

      }
    }

    my_list->insert(myItr, entry);

    itr++;

    itr = FindMatchingFilter(attrs, itr);
  }
  return my_list;
}


void DiffusionCoreAgent::ProcessMessage(Message *msg)
{
  // First check if it's a control message
  if (msg->msg_type == CONTROL){
    ProcessControlMessage(msg);
    return;
  }

  // Now process the incoming message
  FilterList *my_list;
  FilterList::iterator itr;
  Filter_Entry *entry;

  my_list = GetFilterList(msg->msg_attr_vec);

  // Ok, we have a list of Filters to call. Send this message
  // to the first filter on this list
  if (my_list->size() > 0){
    itr = my_list->begin();
    entry = *itr;

    ForwardMessage(msg, entry);
    my_list->clear();
  }
  delete my_list;
}


void DiffusionCoreAgent::ProcessControlMessage(Message *msg)
{
  char *debug_str;
  NRSimpleAttribute<void *> *ctrlmsg = NULL;
  NRAttrVec::iterator place;
  ControlMessage *controlblob = NULL;
  FilterList *flist;
  FilterList::iterator listitr;
  Filter_Entry *entry;
  int command, param1, param2;
  u_int16_t priority, source_port;
  int16_t handle;
  bool filter_is_last = false;

  // Control messages should not come from other nodes
  if (msg->last_hop != LOCALHOST_ADDR){
    diffPrint(DEBUG_ALWAYS, "Error: Received control message from another node !\n");
    return;
  }

  // Find the control attribute
  place = msg->msg_attr_vec->begin();
  ctrlmsg = ControlMsgAttr.find_from(msg->msg_attr_vec, place, &place);

  if (!ctrlmsg){
    // Control message is invalid
    diffPrint(DEBUG_ALWAYS, "Error: Control message received is invalid !\n");
    return;
  }

  // Extract the control message info
  controlblob = (ControlMessage *) ctrlmsg->getVal();
  command = controlblob->command;
  param1 = controlblob->param1;
  param2 = controlblob->param2;

  // Filter API definitions
  //
  // command = ADD_FILTER
  // param1  = priority
  // param2  = handle
  // attrs   = other attrs specify the filter
  // 
  // Remarks: If this filter is already present for this module,
  //          we don't create a new one. A filter is identified
  //          by the handle and the originating agent. We output
  //          a warning message if the same handle is added twice
  //          and do nothing. However, if attrs and handle are
  //          the same, we just update priority.
  //
  //
  // command = REMOVE_FILTER
  // param1  = handle
  //
  // Remarks: Remove the filter identified by (agent, handle)
  //          If it's not found, a warning message is generated.
  //
  //
  // command = KEEPALIVE_FILTER
  // param1  = handle
  //
  // Remarks: Update the filter's timeout so it doesn't expire.
  //          If filter is not found, output a warning message.
  //
  //
  // command = SEND_MESSAGE
  // param1  = agent_id
  //
  // Remarks: Send message from a local App to another App ot
  //          a neighbor. If agent_id is zero, the packet goes
  //          out to the network. Otherwise, it goes to the
  //          agent_id located on this node.
  //
  //
  // command = SEND_TO_NEXT
  // param1  = handle
  //
  // Remarks: Send this message to the next application in the
  //          priority list. We have to assemble the list again
  //          and figure out the current agent's position on the
  //          list. Then, we send to the next guy.

  LogControlMessage(msg, command, param1, param2);

  // First we remove the control attribute from the message
  msg->msg_attr_vec->erase(place);
  delete ctrlmsg;

  switch(command){
  case ADD_FILTER:

    priority = param1;
    handle = param2;

    entry = FindFilter(handle, msg->source_port);

    if (entry){
      // Filter already present

      if (PerfectMatch(entry->filterAttrs, msg->msg_attr_vec)){
	// Attrs also match
	if (priority == entry->priority){

	  // Nothing to do !
	  debug_str = new char[SMALL_DEBUG_MESSAGE];
	  sprintf(debug_str, "Attempt to add filter %d, %d, %d twice !\n",
		  msg->source_port, handle, priority);
	  diffPrint(DEBUG_NO_DETAILS, debug_str);
	  delete [] debug_str;

	}
	else{
	  // Update the priority
	  debug_str = new char[SMALL_DEBUG_MESSAGE];
	  sprintf(debug_str, "Updated priority of filter %d, %d, %d to %d\n",
		  msg->source_port, handle, entry->priority, priority);
	  diffPrint(DEBUG_NO_DETAILS, debug_str);
	  delete [] debug_str;

	  entry->priority = priority;
	}

	break;
      }
    }

    // This is a new filter
    debug_str = new char[SMALL_DEBUG_MESSAGE];
    sprintf(debug_str, "Adding filter %d, %d, %d\n",
	    msg->source_port, handle, priority);
    diffPrint(DEBUG_NO_DETAILS, debug_str);
    delete [] debug_str;

    AddFilter(msg->msg_attr_vec, msg->source_port, handle, priority);

    break;

  case REMOVE_FILTER:

    handle = param1;
    entry = DeleteFilter(handle, msg->source_port);
    if (entry){
      // Filter deleted
      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "Filter %d, %d, %d deleted.\n",
	      entry->agent, entry->handle, entry->priority);
      diffPrint(DEBUG_NO_DETAILS, debug_str);
      delete [] debug_str;

      delete entry;
    }
    else{
      diffPrint(DEBUG_ALWAYS, "Couldn't find filter to delete !\n");
    }

    break;

  case KEEPALIVE_FILTER:

    handle = param1;
    entry = FindFilter(handle, msg->source_port);
    if (entry){
      // Update filter timeout
      getTime(&(entry->tmv));

      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "Filter %d, %d, %d refreshed.\n",
	      entry->agent, entry->handle, entry->priority);
      diffPrint(DEBUG_SOME_DETAILS, debug_str);
      delete [] debug_str;

    }
    else{
      diffPrint(DEBUG_ALWAYS, "Couldn't find filter to update !\n");
    }

    break;

  case SEND_MESSAGE:

    if (!RestoreOriginalHeader(msg))
      break;

    // Send Message
    SendMessage(msg, param1);

    break;

  case SEND_TO_NEXT:

    handle = param1;
    source_port = msg->source_port;

    if (!RestoreOriginalHeader(msg))
      break;

    // Now process the incoming message
    flist = GetFilterList(msg->msg_attr_vec);

    // Find the filter after the 'current' filter on the list
    if (flist->size() > 0){
      for (listitr = flist->begin(); listitr != flist->end(); ++listitr){
	entry = *listitr;
	if (entry->agent == source_port &&
	    entry->handle == handle){
	  // This is the filter who sent us the info
	  listitr++;
	  if (listitr == flist->end())
	    filter_is_last = true;
	  break;
	}
      }

      if (!filter_is_last){
	if (listitr == flist->end())
	  listitr = flist->begin();
	entry = *listitr;

	ForwardMessage(msg, entry);

      }

      flist->clear();
    }

    delete flist;

    break;

  default:

    diffPrint(DEBUG_ALWAYS, "Error: Unknown control message received !\n");
    break;
  }
}

void DiffusionCoreAgent::LogControlMessage(Message *msg, int command,
				       int param1, int param2)
{
  // Logs the incoming message
}



#ifdef NS_DIFFUSION
DiffusionCoreAgent::DiffusionCoreAgent(DiffRoutingAgent *diffrtg)
#else
DiffusionCoreAgent::DiffusionCoreAgent(int argc, char **argv)
#endif
{
  int opt;
  DiffusionIO *device;
  char *scadds_env;
  long stop_time;
  struct timeval tv;
  char *debug_str;

  stop_time=0;
  global_debug_level = DEFAULT_DEBUG_LEVEL;
  diffusion_port = DEFAULT_DIFFUSION_PORT;

  scadds_env = getenv("scadds_addr");
  if (scadds_env != NULL){
    myId = atoi(scadds_env);
  }
  else{
    diffPrint(DEBUG_ALWAYS, "Diffusion : scadds_addr not set. Using random id.\n");

    // Generate random ID
    do{
      getTime(&tv);
      //srand(tv.tv_usec);
      getSeed(tv);
      myId = rand();
    }
    while(myId == LOCALHOST_ADDR || myId == BROADCAST_ADDR);
  }

#ifdef SCADDS
  // Parse command line options
  opterr = 0;
  config_file = NULL;
  stop_time = 0;

  while (1){
    opt = getopt(argc, argv, "f:hd:vt:p:");

    switch(opt){

    case 'p': 

      diffusion_port = (u_int16_t) atoi(optarg);
      if ((diffusion_port < 1024) || (diffusion_port >= 65535)){
	diffPrint(DEBUG_ALWAYS, "Diffusion Error: Port must be between 1024 and 65535 !\n");
	exit(-1);
      }

      break;

    case 't':

      stop_time = atol(optarg);
      if (stop_time <= 0){
	diffPrint(DEBUG_ALWAYS, "Diffusion Error: stop time must be > 0\n");
	exit(-1);
      }
      else{
	debug_str = new char[SMALL_DEBUG_MESSAGE];
	sprintf(debug_str, "%s will stop after %ld seconds\n",
		PROGRAM, stop_time);
	diffPrint(DEBUG_ALWAYS, debug_str);
	delete [] debug_str;
      }

      break;

    case 'h':

      usage();

      break;

    case 'v':

      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "\n%s %s\n", PROGRAM, RELEASE);
      diffPrint(DEBUG_ALWAYS, debug_str);
      delete [] debug_str;
      exit(0);

      break;

    case 'd':

      global_debug_level = atoi(optarg);

      if (global_debug_level < 1 || global_debug_level > 10){
	global_debug_level = DEFAULT_DEBUG_LEVEL;
	diffPrint(DEBUG_ALWAYS, "Error: Debug level outside range or missing !\n");
	usage();
      }

      break;

    case 'f':

      if (!strncasecmp(optarg, "-", 1)){
	diffPrint(DEBUG_ALWAYS, "Error: Parameter is missing !\n");
	usage();
      }

      config_file = strdup(optarg);

      break;

    case '?':

      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "Error: %c isn't a valid option or its parameter is missing !\n", optopt);
      diffPrint(DEBUG_ALWAYS, debug_str);
      delete [] debug_str;
      usage();

      break;

    case ':':

      diffPrint(DEBUG_ALWAYS, "Parameter missing !\n");
      usage();

      break;
    }

    if (opt == -1)
      break;
  }

  if (!config_file)
    config_file = strdup(DEFAULT_CONFIG_FILE);

#endif //scadds

  // Initialize variables
  lon = 0.0;
  lat = 0.0;

#ifdef STATS
  stats = new DiffusionStats(myId);
#  ifndef WIRED
#     ifdef USE_RPC
  rpcstats = new RPCStats(myId);
#     endif
#  endif
#endif

  getTime(&tv);
  //srand(tv.tv_usec);
  getSeed(tv);
  pkt_count = rand();
  rdm_id = rand();
  
  Tcl_InitHashTable(&htable, 2);

  // Initialize eventQueue
#ifdef NS_DIFFUSION
  eq = new DiffusionCoreEQ(diffrtg);
#else
  eq = new eventQueue;
#endif

  eq->eq_new();

  // Add timers to the eventQueue
  eq->eq_addAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
  eq->eq_addAfter(FILTER_TIMER, NULL, FILTER_DELAY);

  if (stop_time > 0) 
    eq->eq_addAfter(STOP_TIMER, NULL, stop_time * 1000);
  
  getTime(&tv);

  // Print Initialization message
  debug_str = new char[SMALL_DEBUG_MESSAGE];
  sprintf(debug_str, "CORE:pkt_count=%d, rdm_id=%d\n",pkt_count,rdm_id);
  diffPrint(DEBUG_ALWAYS, debug_str);
  sprintf(debug_str, "Diffusion : starting at time %ld:%ld\n",
	  tv.tv_sec, tv.tv_usec);
  diffPrint(DEBUG_ALWAYS, debug_str);
  sprintf(debug_str, "Diffusion : Node id = %d\n", myId);
  diffPrint(DEBUG_ALWAYS, debug_str);
  delete [] debug_str;

  // Initialize diffusion io devices

#ifdef NS_DIFFUSION
  device = new LocalApp(diffrtg);
  local_out_devices.push_back(device);
  device = new LinkLayerAbs(diffrtg);
  out_devices.push_back(device);
#endif

#ifdef UDP
  device = new UDPLocal(&diffusion_port);
  in_devices.push_back(device);
  local_out_devices.push_back(device);

#ifdef WIRED
  device = new UDPWired(config_file);
  out_devices.push_back(device);
#endif // WIRED
#endif // UDP

#ifdef USE_RPC
  device = new RPCIO();
  in_devices.push_back(device);
  out_devices.push_back(device);
#endif // USE_RPC

#ifdef USE_MOTE_NIC
  device = new MOTEIO();
  in_devices.push_back(device);
  out_devices.push_back(device);
#endif // USE_MOTE_NIC

#ifdef USE_WINSNG2
  device = new WINSNG2();
  in_devices.push_back(device);
  out_devices.push_back(device);
#endif // USE_WINSNG2
}

Hash_Entry * DiffusionCoreAgent::GetHash(unsigned int pkt_num,
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


void DiffusionCoreAgent::PutHash(unsigned int pkt_num,
			     unsigned int rdm_id)
{
  Tcl_HashEntry *entryPtr;
  Hash_Entry    *hashPtr;
  HashList::iterator itr;
  unsigned int key[2];
  int newPtr;

  if (hash_list.size() == HASH_TABLE_MAX_SIZE){
    // Hash table reached maximum size

    for (int i = 0; ((i < HASH_TABLE_REMOVE_AT_ONCE)
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

  hashPtr = new Hash_Entry;

  Tcl_SetHashValue(entryPtr, hashPtr);

  hash_list.push_back(entryPtr);
}


void DiffusionCoreAgent::dvi_send(char *pkt, int length)
{
  DeviceList::iterator itr;
  struct hdr_diff *dfh = HDR_DIFF(pkt);
  int32_t dst;

  dst = ntohl(NEXT_HOP(dfh));

  for (itr = out_devices.begin(); itr != out_devices.end(); ++itr){
    (*itr)->SendPacket((DiffPacket) pkt, length, dst);
  }
}


DiffPacket DiffusionCoreAgent::AllocateBuffer(NRAttrVec *attrs)
{
  DiffPacket pkt;
  int len;

  len = CalculateSize(attrs);
  len = len + sizeof(struct hdr_diff);
  pkt = new int [1 + (len / sizeof(int))];

  if (pkt == NULL){
    diffPrint(DEBUG_ALWAYS, "Cannot allocate memory for outgoing message !\n");
    exit(-1);
  }

  return pkt;
}


void DiffusionCoreAgent::recv(DiffPacket pkt)
{
  struct hdr_diff *dfh = HDR_DIFF(pkt);
  Message *rcv_message = NULL;
  int8_t version, msg_type;
  u_int16_t data_len, num_attr, source_port;
  int32_t rdm_id, pkt_num, next_hop, last_hop;   

  // Read header
  version = VERSION(dfh);
  msg_type = MSG_TYPE(dfh);
  source_port = ntohs(SRC_PORT(dfh));
  pkt_num = ntohl(PKT_NUM(dfh));
  rdm_id = ntohl(RDM_ID(dfh));
  num_attr = ntohs(NUM_ATTR(dfh));
  next_hop = ntohl(NEXT_HOP(dfh));
  last_hop = ntohl(LAST_HOP(dfh));
  data_len = ntohs(DATA_LEN(dfh));

  // Check version
  if (version != DIFFUSION_VERSION)
    return;

  // Check for ID conflict
  if (last_hop == myId){
    diffPrint(DEBUG_ALWAYS, "Error: A diffusion ID conflict has been detected !\n");
    exit(-1);
  }

  // Address filtering
  if ((next_hop != BROADCAST_ADDR) &&
      (next_hop != LOCALHOST_ADDR) &&
      (next_hop != myId))
    return;

  // Packet is good, create a message
  rcv_message = new Message(version, msg_type, source_port, data_len,
			    num_attr, pkt_num, rdm_id, next_hop, last_hop);

  int total_size = sizeof(struct hdr_diff) + data_len;

  //rcv_message->msg = pkt;
  rcv_message->msg_size = total_size;

  // Read all attributes into the Message structure
  //rcv_message->msg_attr_vec = UnpackAttrs(rcv_message->msg, num_attr);
  rcv_message->msg_attr_vec = UnpackAttrs(pkt, num_attr);

  // Control Messages are unique and don't go to the hash
  if (rcv_message->msg_type != CONTROL){
    // Hash table keeps info about packets
    Tcl_HashEntry *entryPtr;
    unsigned int key[2];

    key[0] = pkt_num;
    key[1] = rdm_id;
    entryPtr = Tcl_FindHashEntry(&htable, (char *) key);

    if (entryPtr != NULL){
      diffPrint(DEBUG_DETAILS, "Received old message !\n");
      rcv_message->new_message = 0;
    }
    else{
      // Add message to the hash table
      PutHash(key[0], key[1]);
      rcv_message->new_message = 1;
    }
  }

#ifdef STATS
  stats->logIncomingMessage(rcv_message);
#endif

  // Now, we look to the current filters to find a match
  ProcessMessage(rcv_message);

  // Don't forget to delete message when we're done

  delete rcv_message;
#ifdef SCADDS
  // diffPacket deleted when Packet::free() gets called
  // in diffRoutingAgent::recv(Packet*, handler*)
  delete [] pkt;
#endif

}

DiffPacket DupPacket(DiffPacket pkt)
{
  int p_size;
  struct hdr_diff *dfh = HDR_DIFF(pkt);

  p_size = sizeof(struct hdr_diff) + ntohs(DATA_LEN(dfh));

  DiffPacket dup = new int [1 + (p_size / sizeof(int))];

  if (dup == NULL){
    diffPrint(DEBUG_ALWAYS, "Can't allocate memory. Duplicate Packet fails.\n");
    exit(-1);
  }

  //memcpy(dup, pkt, sizeof(struct hdr_diff) + ntohs(DATA_LEN(dfh)));
  memcpy(dup, pkt, p_size);

  return dup;
}


#ifdef SCADDS
void DiffusionCoreAgent::usage()
{
  diffPrint(DEBUG_ALWAYS, "Usage: diffusion [-d debug] [-f filename] [-t stoptime] [-v] [-h] [-p port]\n\n");
  diffPrint(DEBUG_ALWAYS, "\t-d - Sets debug level (0-10)\n");
  diffPrint(DEBUG_ALWAYS, "\t-t - Schedule diffusion to exit after stoptime seconds\n");
  diffPrint(DEBUG_ALWAYS, "\t-f - Uses filename as the config file\n");
  diffPrint(DEBUG_ALWAYS, "\t-v - Prints diffusion version\n");
  diffPrint(DEBUG_ALWAYS, "\t-h - Prints this information\n");
  diffPrint(DEBUG_ALWAYS, "\t-p - Sets diffusion port to port\n");

  diffPrint(DEBUG_ALWAYS, "\n");

  exit(0);
}

void DiffusionCoreAgent::TimetoStop()
{
  FILE *outfile = NULL;

  if (global_debug_level > DEBUG_SOME_DETAILS){
    outfile = fopen("/tmp/diffusion.out", "w");
    if (outfile == NULL){
      diffPrint(DEBUG_ALWAYS ,"Diffusion Error: Can't create /tmp/diffusion.out\n");
      return;
    }
  }

#ifdef STATS
  stats->printStats(stdout);
  if (outfile)
    stats->printStats(outfile);
#  ifndef WIRED
#     ifdef USE_RPC
  rpcstats->printStats(stdout);
  if (outfile)
    rpcstats->printStats(outfile);
#     endif
#  endif
#endif

  if (outfile)
    fclose(outfile);
}

void DiffusionCoreAgent::run()
{
  DeviceList::iterator itr;
  DiffPacket in_pkt;
  fd_set fds;
  bool flag;
  int status, max_sock, fd;
  struct timeval *tv;
  char *debug_str;

  // Main Select Loop
  while (1){

    // Wait for incoming packets
    FD_ZERO(&fds);
    max_sock = 0;

    for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
      (*itr)->AddInFDS(&fds, &max_sock);
    }

    // Figure out how much to wait
    tv = eq->eq_nextTimer();

    fflush(stdout);
    status = select(max_sock+1, &fds, NULL, NULL, tv);

    if ((status == 0) || (eq->eq_topInPast())){
      // We got a timeout
      event *e = eq->eq_pop();

      // Timeouts
      switch (e->type){

      case NEIGHBORS_TIMER:

	NeighborsTimeOut();
	free(e);

	break;

      case FILTER_TIMER:

	FilterTimeOut();
	free(e);

	break;

      case STOP_TIMER:

	free(e);
	TimetoStop();
	exit(0);

	break;
      }
    }

    // Check for new packets
    if (status > 0){
      do{
	flag = false;
	for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
	  fd = (*itr)->CheckInFDS(&fds);
	  if (fd != 0){
	    // Message waiting
	    in_pkt = (*itr)->RecvPacket(fd);

	    if (in_pkt)
	      recv(in_pkt);

	    // Clear this fd
	    FD_CLR(fd, &fds);
	    status--;
	    flag = true;
	  }
	}
      } while ((status > 0) && (flag == true));
    }

    // This should not happen
    if (status < 0){
      debug_str = new char[SMALL_DEBUG_MESSAGE];
      sprintf(debug_str, "Select returned %d\n", status);
      diffPrint(DEBUG_IMPORTANT, debug_str);
      delete [] debug_str;
    }
  }
}


int main(int argc, char **argv)
{
  agent = new DiffusionCoreAgent(argc, argv);

  signal(SIGINT, signal_handler);

  agent->run();

  return 0;
}

#endif //scadds

#endif // NS
