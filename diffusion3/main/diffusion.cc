// 
// diffusion.cc  : Main Diffusion program
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: diffusion.cc,v 1.9 2002/03/21 19:30:55 haldar Exp $
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

#include "diffusion.hh"

#ifndef NS_DIFFUSION
DiffusionCoreAgent *agent;
#endif // !NS_DIFFUSION

class HashEntry {
public:
  bool dummy;

  HashEntry() { 
    dummy  = false;
  }
};

class NeighborEntry {
public:
  int32_t id;
  struct timeval tmv;

  NeighborEntry(int _id) : id(_id)
  {
    getTime(&tmv);
  }
};

#ifndef NS_DIFFUSION

void signal_handler(int p)
{
  agent->timeToStop();
  exit(0);
}

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

void DiffusionCoreAgent::timeToStop()
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
#     endif // USE_RPC
#  endif // WIRED
#endif // STATS

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

  // Main Select Loop
  while (1){

    // Wait for incoming packets
    FD_ZERO(&fds);
    max_sock = 0;

    for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
      (*itr)->addInFDS(&fds, &max_sock);
    }

    // Figure out how much to wait
    tv = eq->eqNextTimer();

    fflush(stdout);
    status = select(max_sock+1, &fds, NULL, NULL, tv);

    if ((status == 0) || (eq->eqTopInPast())){
      // We got a timeout
      event *e = eq->eqPop();

      // Timeouts
      switch (e->type){

      case NEIGHBORS_TIMER:

	neighborsTimeOut();
	free(e);

	break;

      case FILTER_TIMER:

	filterTimeOut();
	free(e);

	break;

      case STOP_TIMER:

	free(e);
	timeToStop();
	exit(0);

	break;
      }
    }

    // Check for new packets
    if (status > 0){
      do{
	flag = false;
	for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
	  fd = (*itr)->checkInFDS(&fds);
	  if (fd != 0){
	    // Message waiting
	    in_pkt = (*itr)->recvPacket(fd);

	    if (in_pkt)
	      recvPacket(in_pkt);

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
      diffPrint(DEBUG_IMPORTANT, "Select returned %d\n", status);
    }
  }
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::neighborsTimeOut()
{
  struct timeval tmv;
  NeighborEntry *myNeighbor;
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
  eq->eqAddAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
}

void DiffusionCoreAgent::filterTimeOut()
{
  struct timeval tmv;
  FilterEntry *myFilter;
  FilterList::iterator itr;

  diffPrint(DEBUG_MORE_DETAILS, "Filter Timeout !\n");

  getTime(&tmv);

  itr = filter_list.begin();

  while(itr != filter_list.end()){
    myFilter = *itr;
    if (tmv.tv_sec > myFilter->tmv.tv_sec + FILTER_TIMEOUT){

      // This filter expired
      diffPrint(DEBUG_NO_DETAILS, "Filter %d, %d, %d timed out !\n",
		myFilter->agent, myFilter->handle, myFilter->priority);
      itr = filter_list.erase(itr);
      delete myFilter;
    }
    else{
      itr++;
    }
  }

  // Re-Schedule this timer
  eq->eqAddAfter(FILTER_TIMER, NULL, FILTER_DELAY);
}

void DiffusionCoreAgent::sendMessage(Message *msg)
{
  Tcl_HashEntry *entryPtr;
  unsigned int key[2];
  Message *myMessage;
  
  myMessage = new Message(DIFFUSION_VERSION, msg->msg_type, diffusion_port, 0,
			  0, msg->pkt_num, msg->rdm_id, msg->next_hop, 0);

  myMessage->msg_attr_vec = CopyAttrs(msg->msg_attr_vec);
  myMessage->num_attr = myMessage->msg_attr_vec->size();
  myMessage->data_len = CalculateSize(myMessage->msg_attr_vec);

  // Adjust message size for logging and check hash
  key[0] = msg->pkt_num;
  key[1] = msg->rdm_id;
  entryPtr = Tcl_FindHashEntry(&htable, (char *) key);
  if (entryPtr)
    msg->new_message = 0;
  else
    msg->new_message = 1;

  myMessage->new_message = msg->new_message;

  // Check if message goes to an agent or the network
  if (msg->next_port){
    // Message goes to an agent
    myMessage->last_hop = LOCALHOST_ADDR;

    // If it's a local message, it has to go to a local agent
    if (myMessage->next_hop != (int)LOCALHOST_ADDR){
      diffPrint(DEBUG_ALWAYS, "Error: Message destination is a local agent but next_hop != LOCALHOST_ADDR !\n");
      delete myMessage;
      return;
    }

    // Send the message to the agent specified
    sendMessageToLibrary(myMessage, msg->next_port);
  }
  else{
    // Message goes to the network
    myMessage->last_hop = myId;

#ifdef STATS
    stats->logOutgoingMessage(myMessage);
#endif // STATS

    // Add message to the hash table      
    if (entryPtr == NULL)
      putHash(key[0], key[1]);
    else
      diffPrint(DEBUG_DETAILS, "Message being sent is an old message !\n");

    // Send Message
    sendMessageToNetwork(myMessage);
  }

  delete myMessage;
}

void DiffusionCoreAgent::forwardMessage(Message *msg, FilterEntry *dst)
{
  //DeviceList::iterator itr;
  RedirectMessage *originalHdr;
  NRAttribute *originalAttr;
  Message *myMessage;

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
  originalHdr->next_port = msg->next_port;
  originalHdr->handle = dst->handle;

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  myMessage = new Message(DIFFUSION_VERSION, REDIRECT, diffusion_port, 0,
			  0, pkt_count, rdm_id, LOCALHOST_ADDR, myId);

  // Increment pkt_counter
  pkt_count++;

  // Duplicate the message's attributes
  myMessage->msg_attr_vec = CopyAttrs(msg->msg_attr_vec);
  
  // Add the extra attribute
  myMessage->msg_attr_vec->push_back(originalAttr);
  myMessage->num_attr = myMessage->msg_attr_vec->size();
  myMessage->data_len = CalculateSize(myMessage->msg_attr_vec);

  sendMessageToLibrary(myMessage, dst->agent);

  delete myMessage;
  delete originalHdr;
}

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::sendMessageToLibrary(Message *msg, u_int16_t agent_id)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  int len;
  char *pos;

  out_pkt = AllocateBuffer(msg->msg_attr_vec);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec, pos);

  LAST_HOP(dfh) = htonl(msg->last_hop);
  NEXT_HOP(dfh) = htonl(msg->next_hop);
  VERSION(dfh) = msg->version;
  MSG_TYPE(dfh) = msg->msg_type;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh) = htonl(msg->pkt_num);
  RDM_ID(dfh) = htonl(msg->rdm_id);
  NUM_ATTR(dfh) = htons(msg->num_attr);
  SRC_PORT(dfh) = htons(msg->source_port);

  sendPacketToLibrary(out_pkt, sizeof(struct hdr_diff) + len, agent_id);

  delete [] out_pkt;
}
#else
void DiffusionCoreAgent::sendMessageToLibrary(Message *msg, u_int16_t agent_id)
{
  Message *myMsg;
  DeviceList::iterator itr;
  int len;

  myMsg = CopyMessage(msg);
  len = CalculateSize(myMsg->msg_attr_vec);
  len = len + sizeof(struct hdr_diff);

  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) myMsg, len, agent_id);
  }
}
#endif // !NS_DIFFUSION

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::sendMessageToNetwork(Message *msg)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  int len;
  char *pos;

  out_pkt = AllocateBuffer(msg->msg_attr_vec);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec, pos);

  LAST_HOP(dfh) = htonl(msg->last_hop);
  NEXT_HOP(dfh) = htonl(msg->next_hop);
  VERSION(dfh) = msg->version;
  MSG_TYPE(dfh) = msg->msg_type;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh) = htonl(msg->pkt_num);
  RDM_ID(dfh) = htonl(msg->rdm_id);
  NUM_ATTR(dfh) = htons(msg->num_attr);
  SRC_PORT(dfh) = htons(msg->source_port);

  sendPacketToNetwork(out_pkt, sizeof(struct hdr_diff) + len, msg->next_hop);

  delete [] out_pkt;
}
#else
void DiffusionCoreAgent::sendMessageToNetwork(Message *msg)
{
  Message *myMsg;
  int len;
  int32_t dst;
  DeviceList::iterator itr;

  myMsg = CopyMessage(msg);
  len = CalculateSize(myMsg->msg_attr_vec);
  len = len + sizeof(struct hdr_diff);
  dst = myMsg->next_hop;

  for (itr = out_devices.begin(); itr != out_devices.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) myMsg, len, dst);
  }
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::sendPacketToLibrary(DiffPacket pkt, int len,
					     u_int16_t dst)
{
  DeviceList::iterator itr;

  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->sendPacket(pkt, len, dst);
  }
}

void DiffusionCoreAgent::sendPacketToNetwork(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator itr;

  for (itr = out_devices.begin(); itr != out_devices.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) pkt, len, dst);
  }
}

void DiffusionCoreAgent::updateNeighbors(int id)
{
  NeighborList::iterator itr;
  NeighborEntry *newNeighbor;

  if (id == (int)LOCALHOST_ADDR || id == myId)
    return;

  for (itr = my_neighbors.begin(); itr != my_neighbors.end(); ++itr){
    if ((*itr)->id == id)
      break;
  }

  if (itr == my_neighbors.end()){
    // This is a new neighbor
    newNeighbor = new NeighborEntry(id);
    my_neighbors.push_front(newNeighbor);
  }
  else{
    // Just update the neighbor timeout
    getTime(&((*itr)->tmv));
  }
}

FilterEntry * DiffusionCoreAgent::findFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator itr;
  FilterEntry *current;

  for (itr = filter_list.begin(); itr != filter_list.end(); ++itr){
    current = *itr;
    if (handle != current->handle || agent != current->agent)
      continue;

    // Found
    return current;
  }
  return NULL;
}

FilterEntry * DiffusionCoreAgent::deleteFilter(int16_t handle, u_int16_t agent)
{
  FilterList::iterator itr = filter_list.begin();
  FilterEntry *current = NULL;

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

bool DiffusionCoreAgent::addFilter(NRAttrVec *attrs, u_int16_t agent,
				   int16_t handle, u_int16_t priority)
{
  FilterList::iterator itr;
  FilterEntry *entry;

  itr = filter_list.begin();
  while (itr != filter_list.end()){
    entry = *itr;
    if (entry->priority == priority)
      return false;
    itr++;
  }

  entry = new FilterEntry(handle, priority, agent);

  // Copy the Attribute Vector
  entry->filterAttrs = CopyAttrs(attrs);

  // Add this filter to the filter list
  filter_list.push_back(entry);

  return true;
}

FilterList::iterator DiffusionCoreAgent::findMatchingFilter(NRAttrVec *attrs,
							FilterList::iterator itr)
{
  FilterEntry *entry;

  for (;itr != filter_list.end(); ++itr){
    entry = *itr;

    if (OneWayMatch(entry->filterAttrs, attrs)){
      // That's a match !
      break;
    }
  }
  return itr;
}

bool DiffusionCoreAgent::restoreOriginalHeader(Message *msg)
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
  msg->next_port = originalHdr->next_port;

  // Delete attribute from original set
  msg->msg_attr_vec->erase(place);
  delete originalHeader;

  return true;
}

FilterList * DiffusionCoreAgent::getFilterList(NRAttrVec *attrs)
{
  FilterList *my_list = new FilterList;
  FilterList::iterator itr, myItr;
  FilterEntry *entry, *listEntry;

  // We need to come up with a list of filters to call
  // F1 will be called before F2 if F1->priority > F2->priority
  // If F1 and F2 have the same priority, F1 will be called
  // before F2 is F1->handle < F2->handle. If both handles
  // are equal, the one with the lower port number will be
  // called first.

  itr = findMatchingFilter(attrs, filter_list.begin());

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

    itr = findMatchingFilter(attrs, itr);
  }
  return my_list;
}

u_int16_t DiffusionCoreAgent::getNextFilterPriority(int16_t handle,
						    u_int16_t priority,
						    u_int16_t agent)
{
  FilterList::iterator itr;
  FilterEntry *entry;

  if ((priority < FILTER_MIN_PRIORITY) ||
      (priority > FILTER_KEEP_PRIORITY))
    return FILTER_INVALID_PRIORITY;

  if (priority < FILTER_KEEP_PRIORITY)
    return (priority - 1);

  itr = filter_list.begin();

  while (itr != filter_list.end()){
    entry = *itr;

    if ((entry->handle == handle) && (entry->agent == agent)){
      // Found this filter
      return (entry->priority - 1);
    }

    itr++;
  }

  return FILTER_INVALID_PRIORITY;
}

void DiffusionCoreAgent::processMessage(Message *msg)
{
  FilterList *my_list;
  FilterList::iterator itr;
  FilterEntry *entry;

  my_list = getFilterList(msg->msg_attr_vec);

  // Ok, we have a list of Filters to call. Send this message
  // to the first filter on this list
  if (my_list->size() > 0){
    itr = my_list->begin();
    entry = *itr;

    forwardMessage(msg, entry);
    my_list->clear();
  }
  delete my_list;
}

void DiffusionCoreAgent::processControlMessage(Message *msg)
{
  NRSimpleAttribute<void *> *ctrlmsg = NULL;
  NRAttrVec::iterator place;
  ControlMessage *controlblob = NULL;
  FilterList *flist;
  FilterList::iterator listitr;
  FilterEntry *entry;
  int command, param1, param2;
  u_int16_t priority, source_port, new_priority;
  int16_t handle;
  bool filter_is_last = false;

  // Control messages should not come from other nodes
  if (msg->last_hop != (int)LOCALHOST_ADDR){
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
  // Remarks: Send message from a local App to another App or
  //          a neighbor. If agent_id is zero, the packet goes
  //          out to the network. Otherwise, it goes to the
  //          agent_id located on this node.
  //
  //
  // command = SEND_TO_NEXT
  // param1  = handle
  // param2  = priority
  //
  // Remarks: Send this message to the next application in the
  //          priority list. We have to assemble the list again
  //          and figure out the current agent's position on the
  //          list. Then, we send to the next guy.

  logControlMessage(msg, command, param1, param2);

  // First we remove the control attribute from the message
  msg->msg_attr_vec->erase(place);
  delete ctrlmsg;

  switch(command){
  case ADD_FILTER:

    priority = param1;
    handle = param2;

    entry = findFilter(handle, msg->source_port);

    if (entry){
      // Filter already present

      if (PerfectMatch(entry->filterAttrs, msg->msg_attr_vec)){
	// Attrs also match
	if (priority == entry->priority){
	  // Nothing to do !
	  diffPrint(DEBUG_NO_DETAILS,
		    "Attempt to add filter %d, %d, %d twice !\n",
		    msg->source_port, handle, priority);
	}
	else{
	  // Update the priority
	  diffPrint(DEBUG_NO_DETAILS,
		    "Updated priority of filter %d, %d, %d to %d\n",
		    msg->source_port, handle, entry->priority, priority);
	  entry->priority = priority;
	}

	break;
      }
    }

    // This is a new filter
    if (!addFilter(msg->msg_attr_vec, msg->source_port, handle, priority)){
      diffPrint(DEBUG_ALWAYS, "Failed to add filter %d, %d, %d\n",
		msg->source_port, handle, priority);
    }
    else{
      diffPrint(DEBUG_NO_DETAILS, "Adding filter %d, %d, %d\n",
		msg->source_port, handle, priority);
    }

    break;

  case REMOVE_FILTER:

    handle = param1;
    entry = deleteFilter(handle, msg->source_port);
    if (entry){
      // Filter deleted
      diffPrint(DEBUG_NO_DETAILS, "Filter %d, %d, %d deleted.\n",
		entry->agent, entry->handle, entry->priority);

      delete entry;
    }
    else{
      diffPrint(DEBUG_ALWAYS, "Couldn't find filter to delete !\n");
    }

    break;

  case KEEPALIVE_FILTER:

    handle = param1;
    entry = findFilter(handle, msg->source_port);
    if (entry){
      // Update filter timeout
      getTime(&(entry->tmv));
      diffPrint(DEBUG_SOME_DETAILS, "Filter %d, %d, %d refreshed.\n",
		entry->agent, entry->handle, entry->priority);
    }
    else{
      diffPrint(DEBUG_ALWAYS, "Couldn't find filter to update !\n");
    }

    break;

  case SEND_MESSAGE:

    handle = param1;
    priority = param2;
    source_port = msg->source_port;

    if (!restoreOriginalHeader(msg))
      break;

    new_priority = getNextFilterPriority(handle, priority, source_port);

    if (new_priority == FILTER_INVALID_PRIORITY)
      break;

    // Now process the incoming message
    flist = getFilterList(msg->msg_attr_vec);

    // Find the filter after the 'current' filter on the list
    if (flist->size() > 0){
      for (listitr = flist->begin(); listitr != flist->end(); ++listitr){
	entry = *listitr;
	if (entry->priority <= new_priority){
	  forwardMessage(msg, entry);
	  break;
	}
      }

      if (listitr == flist->end())
	filter_is_last = true;

    }
    else{
      filter_is_last = true;
    }

    if (filter_is_last){
      // Forward message to the network or the destination application
      sendMessage(msg);
    }

    flist->clear();

    delete flist;

    break;

  default:

    diffPrint(DEBUG_ALWAYS, "Error: Unknown control message received !\n");

    break;
  }
}

void DiffusionCoreAgent::logControlMessage(Message *msg, int command,
					   int param1, int param2)
{
  // Logs the incoming message
}

#ifdef NS_DIFFUSION
DiffusionCoreAgent::DiffusionCoreAgent(DiffRoutingAgent *diffrtg)
#else
DiffusionCoreAgent::DiffusionCoreAgent(int argc, char **argv)
#endif // NS_DIFFUSION
{
  DiffusionIO *device;
  char *scadds_env;
  long stop_time;
  struct timeval tv;

  opterr = 0;
  config_file = NULL;
  stop_time = 0;

#ifdef NS_DIFFUSION
  application_id = strdup("DIFFUSION_NS");
#else
  application_id = strdup(argv[0]);
#endif // NS_DIFFUSION
  scadds_env = getenv("scadds_addr");
  diffusion_port = DEFAULT_DIFFUSION_PORT;

#ifdef BBN_LOGGER
  InitMainLogger();
#endif // BBN_LOGGER

#ifndef NS_DIFFUSION
  // Parse command line options
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
	diffPrint(DEBUG_ALWAYS, "%s will stop after %ld seconds\n",
		  PROGRAM, stop_time);
      }

      break;

    case 'h':

      usage();

      break;

    case 'v':

      diffPrint(DEBUG_ALWAYS, "\n%s %s\n", PROGRAM, RELEASE);
      exit(0);

      break;

    case 'd':

      debug_level = atoi(optarg);

      if (debug_level < 1 || debug_level > 10){
	diffPrint(DEBUG_ALWAYS, "Error: Debug level outside range or missing !\n");
	usage();
      }

      global_debug_level = debug_level;

      break;

    case 'f':

      if (!strncasecmp(optarg, "-", 1)){
	diffPrint(DEBUG_ALWAYS, "Error: Parameter is missing !\n");
	usage();
      }

      config_file = strdup(optarg);

      break;

    case '?':

      diffPrint(DEBUG_ALWAYS, "Error: %c isn't a valid option or its parameter is missing !\n", optopt);
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
#endif // !NS_DIFFUSION

  // Get diffusion ID
  if (scadds_env != NULL){
    myId = atoi(scadds_env);
  }
  else{
    diffPrint(DEBUG_ALWAYS, "Diffusion : scadds_addr not set. Using random id.\n");

    // Generate random ID
    do{
      getTime(&tv);
      setSeed(&tv);
      myId = getRand();
    }
    while(myId == (int)LOCALHOST_ADDR || myId == (int)BROADCAST_ADDR);
  }

  // Initialize variables
  lon = 0.0;
  lat = 0.0;

#ifdef STATS
  stats = new DiffusionStats(myId);
#  ifndef WIRED
#     ifdef USE_RPC
  rpcstats = new RPCStats(myId);
#     endif // USE_RPC
#  endif // !WIRED
#endif // STATS

  getTime(&tv);
  setSeed(&tv);
  pkt_count = getRand();
  rdm_id = getRand();

  Tcl_InitHashTable(&htable, 2);

  // Initialize EventQueue
#ifdef NS_DIFFUSION
  eq = new DiffusionCoreEQ(diffrtg);
#else
  eq = new EventQueue;
#endif // NS_DIFFUSION

  // Add timers to the EventQueue
  eq->eqAddAfter(NEIGHBORS_TIMER, NULL, NEIGHBORS_DELAY);
  eq->eqAddAfter(FILTER_TIMER, NULL, FILTER_DELAY);

  if (stop_time > 0) 
    eq->eqAddAfter(STOP_TIMER, NULL, stop_time * 1000);

  getTime(&tv);

  // Print Initialization message
  diffPrint(DEBUG_ALWAYS, "Diffusion : starting at time %ld:%ld\n",
	    tv.tv_sec, tv.tv_usec);
  diffPrint(DEBUG_ALWAYS, "Diffusion : Node id = %d\n", myId);

  // Initialize diffusion io devices
#ifdef NS_DIFFUSION
  device = new LocalApp(diffrtg);
  local_out_devices.push_back(device);
  device = new LinkLayerAbs(diffrtg);
  out_devices.push_back(device);
#endif // NS_DIFFUSION

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

HashEntry * DiffusionCoreAgent::getHash(unsigned int pkt_num,
					 unsigned int rdm_id)
{
  unsigned int key[2];

  key[0] = pkt_num;
  key[1] = rdm_id;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL)
    return NULL;

  return (HashEntry *)Tcl_GetHashValue(entryPtr);
}

void DiffusionCoreAgent::putHash(unsigned int pkt_num,
				 unsigned int rdm_id)
{
  Tcl_HashEntry *entryPtr;
  HashEntry *hashPtr;
  HashList::iterator itr;
  unsigned int key[2];
  int newPtr;

  if (hash_list.size() == HASH_TABLE_MAX_SIZE){
    // Hash table reached maximum size

    for (int i = 0; ((i < HASH_TABLE_REMOVE_AT_ONCE)
		     && (hash_list.size() > 0)); i++){
      itr = hash_list.begin();
      entryPtr = *itr;
      hashPtr = (HashEntry *) Tcl_GetHashValue(entryPtr);
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

  hashPtr = new HashEntry;

  Tcl_SetHashValue(entryPtr, hashPtr);

  hash_list.push_back(entryPtr);
}

#ifndef NS_DIFFUSION
void DiffusionCoreAgent::recvPacket(DiffPacket pkt)
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

  // Packet is good, create a message
  rcv_message = new Message(version, msg_type, source_port, data_len,
			    num_attr, pkt_num, rdm_id, next_hop, last_hop);

  // Read all attributes into the Message structure
  rcv_message->msg_attr_vec = UnpackAttrs(pkt, num_attr);

  // Process the incoming message
  recvMessage(rcv_message);

  // Don't forget to message when we're done
  delete rcv_message;

  delete [] pkt;
}
#endif // !NS_DIFFUSION

void DiffusionCoreAgent::recvMessage(Message *msg)
{
  Tcl_HashEntry *entryPtr;
  unsigned int key[2];

  // Check version
  if (msg->version != DIFFUSION_VERSION)
    return;

  // Check for ID conflict
  if (msg->last_hop == myId){
    diffPrint(DEBUG_ALWAYS, "Error: A diffusion ID conflict has been detected !\n");
    exit(-1);
  }

  // Address filtering
  if ((msg->next_hop != (int)BROADCAST_ADDR) &&
      (msg->next_hop != (int)LOCALHOST_ADDR) &&
      (msg->next_hop != myId))
    return;

  // Control Messages are unique and don't go to the hash
  if (msg->msg_type != CONTROL){
    // Hash table keeps info about packets
  
    key[0] = msg->pkt_num;
    key[1] = msg->rdm_id;
    entryPtr = Tcl_FindHashEntry(&htable, (char *) key);

    if (entryPtr != NULL){
      diffPrint(DEBUG_DETAILS, "Received old message !\n");
      msg->new_message = 0;
    }
    else{
      // Add message to the hash table
      putHash(key[0], key[1]);
      msg->new_message = 1;
    }
  }

#ifdef STATS
  stats->logIncomingMessage(msg);
#endif // STATS

  // Check if it's a control of a regular message
  if (msg->msg_type == CONTROL)
    processControlMessage(msg);
  else
    processMessage(msg);
}

#ifndef NS_DIFFUSION
int main(int argc, char **argv)
{
  agent = new DiffusionCoreAgent(argc, argv);

  signal(SIGINT, signal_handler);

  agent->run();

  return 0;
}
#endif // !NS_DIFFUSION
