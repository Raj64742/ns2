//
// dr.cc           : Diffusion Routing Class
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: dr.cc,v 1.1 2001/11/08 17:43:39 haldar Exp $
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


#include <stdlib.h>
#include <stdio.h>

#include "dr.hh"


#ifdef SCADDS

NR *dr = NULL;

void * ReceiveThread(void *dr)
{
  // Never returns
  ((DiffusionRouting *)dr)->run();

  return NULL;
}

#endif //scadds

#ifdef NS_DIFFUSION

class DiffEventQueue;

// ********************************************************************
// All major changes pertaining to NS go under here. 
// Note all changes specific for NS have been marked off by 
// ifdef NS_DIFFUSION statements.
// scadds specific code not applicable to ns has been marked off
// with ifdef SCADDS statements
// ********************************************************************


void NsLocal::SendPacket(DiffPacket pkt, int len, int dst) {
  agent_->sendPacket(pkt, len, dst);
}

DiffPacket NsLocal::RecvPacket(int fd) {
  DiffPacket p;
  fprintf(stderr, "This function should not get called; call DiffAppAgent::recv(Packet *, Handler *) instead\n\n");
  exit(1);
  return (p);  // to keep the compiler happy
}


int DiffusionRouting::get_agentid(int id = -1) {
  if (id != -1)
    agent_id = id;
  return agent_id;
}

NR * NR::create_ns_NR(u_int16_t port, DiffAppAgent *da) {
  return(new DiffusionRouting(port, da));
}
#endif


#ifdef SCADDS
NR * NR::createNR(u_int16_t port = 0) {

  int retval;
  pthread_t thread;
  
  // Create Diffusion Routing Class
  if (dr)
    return dr;

  dr = new DiffusionRouting(port);
  // Fork a thread for receiving Messages
  retval = pthread_create(&thread, NULL, &ReceiveThread, (void *)dr);
  
  if (retval){
    diffPrint(DEBUG_ALWAYS, "Error creating receiving thread ! Aborting...\n");
    exit(-1);
  }
  
  return dr;
}
#endif //scadds
  

#ifdef NS_DIFFUSION
DiffusionRouting::DiffusionRouting(u_int16_t port, DiffAppAgent *da)
#else
DiffusionRouting::DiffusionRouting(u_int16_t port)
#endif
{
  char *debug_str;
  struct timeval tv;
  DiffusionIO *device;

  // Initialize basic stuff
  next_handle = 1;
  getTime(&tv);
  //srand(tv.tv_usec);
  getSeed(tv);
  pkt_count = rand();
  rdm_id = rand();
  agent_id = 0;
  
  if (port == 0)
    port = DEFAULT_DIFFUSION_PORT;

  diffusion_port = port;

  // Initialize event queue
#ifdef NS_DIFFUSION
  eq = new DiffEventQueue(da);
#else
  eq = new eventQueue;
#endif
  eq->eq_new();

  // Initialize input device

#ifdef NS_DIFFUSION
  device = new NsLocal(da);
  local_out_devices.push_back(device);
#endif //ns

#ifdef UDP
  device = new UDPLocal(&agent_id);
  in_devices.push_back(device);
  local_out_devices.push_back(device);
#endif

  // Print initialization message
  debug_str = new char[SMALL_DEBUG_MESSAGE];
  sprintf(debug_str, "Diffusion Routing Agent initializing... Agent Id = %d\n",
	  agent_id);
  diffPrint(DEBUG_ALWAYS, debug_str);

  sprintf(debug_str, "DR:pkt_count=%d, rdm_id=%d\n",pkt_count, rdm_id);
  diffPrint(DEBUG_ALWAYS, debug_str);
  
  delete [] debug_str;

  listening = true;

  // Initialize Semaphores
  drMtx = new pthread_mutex_t;
  queueMtx = new pthread_mutex_t;

  pthread_mutex_init(drMtx, NULL);
  pthread_mutex_init(queueMtx, NULL);
}

handle DiffusionRouting::subscribe(NRAttrVec *subscribeAttrs, NR::Callback *cb)
{
  Handle_Entry *my_handle;
  NRAttribute *scopeAttr;

  // Get lock first
  pthread_mutex_lock(drMtx);

  // Check the published attributes
  if (!checkSubscription(subscribeAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the subscribe attributes !\n");
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new Handle_Entry;
  my_handle->hdl = next_handle;
  next_handle++;
  my_handle->cb = (NR::Callback *) cb;
  sub_list.push_back(my_handle);

  // Copy the attributes   
  my_handle->attrs = CopyAttrs(subscribeAttrs);

  // For subscriptions, scope is global if not specified
  if (!hasScope(subscribeAttrs)){
    scopeAttr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::GLOBAL_SCOPE);
    my_handle->attrs->push_back(scopeAttr);
  }

  // Send interest and add it to the queue
  InterestTimeout(my_handle);

  // Release lock
  pthread_mutex_unlock(drMtx);

  return my_handle->hdl;
}


int DiffusionRouting::unsubscribe(handle subscription_handle)
{
  Handle_Entry *my_handle = NULL;

  // Get the lock first
  pthread_mutex_lock(drMtx);

  my_handle = findHandle(subscription_handle, &sub_list);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Handle will be destroyed when next InterestTimeout happens
  my_handle->valid = false;

  // Release the lock
  pthread_mutex_unlock(drMtx);

  return OK;
}

handle DiffusionRouting::publish(NRAttrVec *publishAttrs)
{
  Handle_Entry *my_handle;
  NRAttribute *scopeAttr;

  // Get the lock first
  pthread_mutex_lock(drMtx);

  // Check the published attributes
  if (!checkPublication(publishAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the publish attributes !\n");
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new Handle_Entry;
  my_handle->hdl = next_handle;
  next_handle++;
  pub_list.push_back(my_handle);

  // Copy the attributes
  my_handle->attrs = CopyAttrs(publishAttrs);

  // For publications, scope is local if not specified
  if (!hasScope(publishAttrs)){
    scopeAttr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE);
    my_handle->attrs->push_back(scopeAttr);
  }
  
  // Release the lock
  pthread_mutex_unlock(drMtx);

  return my_handle->hdl;
}

int DiffusionRouting::unpublish(handle publication_handle)
{
  Handle_Entry *my_handle = NULL;

  // Get the lock first
  pthread_mutex_lock(drMtx);

  my_handle = removeHandle(publication_handle, &pub_list);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Free structures
  delete my_handle;

  // Release the lock
  pthread_mutex_unlock(drMtx);

  return OK;
}

int DiffusionRouting::send(handle publication_handle,
			   NRAttrVec *sendAttrs)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  Handle_Entry *my_handle;
  Packed_Attribute *aux;
  char *pos;
  int len;

  // Get the lock first
  pthread_mutex_lock(drMtx);

  // Get attributes associated with handle
  my_handle = findHandle(publication_handle, &pub_list);
  if (!my_handle){
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Check the send attributes
  if (!checkSend(sendAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the send attributes !\n");
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  // Special case for calculating packet size
  len = CalculateSize(my_handle->attrs);
  len = len + CalculateSize(sendAttrs);
  len = len + sizeof(struct hdr_diff);

  out_pkt = new int [1 + (len / sizeof(int))];

  if (out_pkt == NULL){
    diffPrint(DEBUG_ALWAYS, "Cannot allocate memory for outgoing message !\n");
    exit(-1);
  }

  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(my_handle->attrs, pos);

  // Move pointer ahead
  pos = pos + len;

  // Add the attributes passed in send
  len = len + PackAttrs(sendAttrs, pos);

  // Prepare packet header to send
  LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(sendAttrs->size() + my_handle->attrs->size());
  PKT_NUM(dfh) = htonl(pkt_count);
  RDM_ID(dfh) = htonl(rdm_id);
  pkt_count++;
  SRC_PORT(dfh) = htons(agent_id);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = DATA;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  snd(out_pkt, sizeof(struct hdr_diff) + len, diffusion_port);

  delete [] out_pkt;

  // Release the lock
  pthread_mutex_unlock(drMtx);

  return OK;
}

DiffPacket DiffusionRouting::AllocateBuffer(NRAttrVec *attrs)
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

handle DiffusionRouting::addFilter(NRAttrVec *filterAttrs, u_int16_t priority,
				   FilterCallback *cb)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  Filter_Entry *my_filter;
  NRAttrVec *attrs;
  NRAttribute *ctrlmsg;
  ControlMessage *controlblob;
  int len;
  char *pos;

  // Get lock first
  pthread_mutex_lock(drMtx);

  // Create and Initialize the handle_entry structute
  my_filter = new Filter_Entry(next_handle, priority, agent_id);
  next_handle++;
  my_filter->cb = (FilterCallback *) cb;
  filter_list.push_back(my_filter);

  // Copy attributes (keep them for matching later)
  my_filter->filterAttrs = CopyAttrs(filterAttrs);

  // Copy the attributes (and add the control attr)
  attrs = CopyAttrs(filterAttrs);
  controlblob = new ControlMessage;
  controlblob->command = ADD_FILTER;
  controlblob->param1 = priority;
  controlblob->param2 = my_filter->handle;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  attrs->push_back(ctrlmsg);

  out_pkt = AllocateBuffer(attrs);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(attrs, pos);

  // Prepare packet header to send
  LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(attrs->size());
  PKT_NUM(dfh) = htonl(pkt_count);
  pkt_count++;
  RDM_ID(dfh) = htonl(rdm_id);
  SRC_PORT(dfh) = htons(agent_id);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = CONTROL;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

  // Add keepalive to event queue
  pthread_mutex_lock(queueMtx);
  eq->eq_addAfter(FILTER_KEEPALIVE_TIMER, (void *) my_filter,
		  FILTER_KEEPALIVE_DELAY);
  pthread_mutex_unlock(queueMtx);

  delete [] out_pkt;

  ClearAttrs(attrs);
  delete attrs;
  delete controlblob;

  // Release the lock
  pthread_mutex_unlock(drMtx);

  return my_filter->handle;
}

int DiffusionRouting::removeFilter(handle filterHandle)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  Filter_Entry *entry = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec attrs;
  int len;
  char *pos;

  // Get lock first
  pthread_mutex_lock(drMtx);

  entry = findFilter(filterHandle);
  if (!entry){
    // Handle doesn't exist, return FAIL
    pthread_mutex_unlock(drMtx);
    return FAIL;
  }

  controlblob = new ControlMessage;
  controlblob->command = REMOVE_FILTER;
  controlblob->param1 = entry->handle;
  controlblob->param2 = 0;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  attrs.push_back(ctrlmsg);

  out_pkt = AllocateBuffer(&attrs);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(&attrs, pos);

  // Prepare packet header to send
  LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(attrs.size());
  PKT_NUM(dfh) = htonl(pkt_count);
  pkt_count++;
  RDM_ID(dfh) = htonl(rdm_id);
  SRC_PORT(dfh) = htons(agent_id);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = CONTROL;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

  // Handle will be destroyed when next keepalive timer happens
  entry->valid = false;

  delete [] out_pkt;

  ClearAttrs(&attrs);
  delete controlblob;

  // Release the lock
  pthread_mutex_unlock(drMtx);

  return OK;
}

handle DiffusionRouting::addTimer(int timeout, void *p, TimerCallbacks *cb)
{
  Timer_Entry *entry;

  entry = new Timer_Entry(next_handle, timeout, p, cb);

  pthread_mutex_lock(queueMtx);
  eq->eq_addAfter(APPLICATION_TIMER, (void *) entry, timeout);
  pthread_mutex_unlock(queueMtx);

  next_handle++;

  return entry->hdl;
}

#ifdef SCADDS
// Future work for NS: add support for removing events from event queue

int DiffusionRouting::removeTimer(handle hdl)
{
  event *e;
  Timer_Entry *entry;
  int found = -1;

  pthread_mutex_lock(queueMtx);

  // Find the timer in the queue
  e = eq->eq_findEvent(APPLICATION_TIMER);
  while (e){
    entry = (Timer_Entry *) e->payload;
    if (entry->hdl == hdl){
      found = 0;
      break;
    }

    e = eq->eq_findNextEvent(APPLICATION_TIMER, e->next);
  }

  // If timer found, remove it from the queue
  if (e){
    if (eq->eq_remove(e) != 0){
      diffPrint(DEBUG_ALWAYS, "Error: Can't remove event from queue !\n");
      abort();
    }

    // Call the application provided delete function
    entry->cb->del(entry->p);

    delete entry;
    free(e);
  }

  pthread_mutex_unlock(queueMtx);

  return found;
}
#endif //scadds

void DiffusionRouting::FilterKeepaliveTimeout(Filter_Entry *entry)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  Filter_Entry *my_handle = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec attrs;
  char *pos;
  int len;

  if (entry->valid){
    // Send keepalive

    controlblob = new ControlMessage;
    controlblob->command = KEEPALIVE_FILTER;
    controlblob->param1 = entry->handle;
    controlblob->param2 = 0;

    ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

    attrs.push_back(ctrlmsg);

    out_pkt = AllocateBuffer(&attrs);
    dfh = HDR_DIFF(out_pkt);

    pos = (char *) out_pkt;
    pos = pos + sizeof(struct hdr_diff);

    len = PackAttrs(&attrs, pos);

    // Prepare packet header to send
    LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
    NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
    NUM_ATTR(dfh) = htons(attrs.size());
    PKT_NUM(dfh) = htonl(pkt_count);
    pkt_count++;
    RDM_ID(dfh) = htonl(rdm_id);
    SRC_PORT(dfh) = htons(agent_id);
    VERSION(dfh) = DIFFUSION_VERSION;
    MSG_TYPE(dfh) = CONTROL;
    DATA_LEN(dfh) = htons(len);

    // Send Packet
    snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

    // Add another keepalive to event queue
    pthread_mutex_lock(queueMtx);
    eq->eq_addAfter(FILTER_KEEPALIVE_TIMER, (void *) entry,
		    FILTER_KEEPALIVE_DELAY);
    pthread_mutex_unlock(queueMtx);

    ClearAttrs(&attrs);
    delete controlblob;

    delete [] out_pkt;

  }
  else{
    // Filter was removed
    my_handle = deleteFilter(entry->handle);

    // We should have removed the correct handle
    if (my_handle != entry){
      diffPrint(DEBUG_ALWAYS, "DiffusionRouting::KeepaliveTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_handle;
  }
}

void DiffusionRouting::InterestTimeout(Handle_Entry *entry)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  Handle_Entry *my_handle = NULL;
  char *pos;
  int len;

  if (entry->valid){
    // Send the interest

    out_pkt = AllocateBuffer(entry->attrs);
    dfh = HDR_DIFF(out_pkt);

    pos = (char *) out_pkt;
    pos = pos + sizeof(struct hdr_diff);

    len = PackAttrs(entry->attrs, pos);

    // Prepare packet header to send
    LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
    NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
    NUM_ATTR(dfh) = htons(entry->attrs->size());
    PKT_NUM(dfh) = htonl(pkt_count);
    pkt_count++;
    RDM_ID(dfh) = htonl(rdm_id);
    SRC_PORT(dfh) = htons(agent_id);
    VERSION(dfh) = DIFFUSION_VERSION;
    MSG_TYPE(dfh) = INTEREST;
    DATA_LEN(dfh) = htons(len);

    // Send Packet
    snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

    // Add another InterestTimeout to the queue
    pthread_mutex_lock(queueMtx);
    eq->eq_addAfter(INTEREST_TIMER, (void *) entry,
		    INTEREST_DELAY +
		    (int) ((INTEREST_JITTER * (rand() * 1.0 / RAND_MAX)) -
			   (INTEREST_JITTER / 2)));
    pthread_mutex_unlock(queueMtx);

    delete [] out_pkt;

  }
  else{
    // Interest was canceled. Just delete it from the handle_list
    my_handle = removeHandle(entry->hdl, &sub_list);

    // We should have removed the correct handle
    if (my_handle != entry){
      diffPrint(DEBUG_ALWAYS, "DiffusionRouting::InterestTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_handle;
  }
}

void DiffusionRouting::ApplicationTimeout(Timer_Entry *entry)
{
  int new_timeout;

  new_timeout = entry->cb->recv(entry->hdl, entry->p);
  pthread_mutex_lock(queueMtx);
  if (new_timeout >= 0){
    if (new_timeout > 0){
      // Change the timer's timeout
      entry->timeout = new_timeout;
    }

    eq->eq_addAfter(APPLICATION_TIMER, (void *) entry, entry->timeout);
  }
  else{
    entry->cb->del(entry->p);
    delete entry;
  }

  pthread_mutex_unlock(queueMtx);

}

void DiffusionRouting::sendMessage(Message *msg, handle h,
				   u_int16_t dst_agent_id = 0)
{
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
  RedirectMessage *originalHdr;
  NRAttribute *originalAttr, *ctrlmsg;
  ControlMessage *controlblob;
  NRAttrVec *attrs;
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
  originalHdr->handle = 0;

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  // Create the attribute with the control message
  controlblob = new ControlMessage;
  controlblob->command = SEND_MESSAGE;
  controlblob->param1 = dst_agent_id;
  controlblob->param2 = 0;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  // Copy Attributes and add the originalAttr
  attrs = CopyAttrs(msg->msg_attr_vec);
  attrs->push_back(originalAttr);
  attrs->push_back(ctrlmsg);

  // Send the CONTROL message to diffusion
  out_pkt = AllocateBuffer(attrs);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(attrs, pos);

  // Prepare packet header to send
  LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(attrs->size());
  PKT_NUM(dfh) = htonl(pkt_count);
  pkt_count++;
  RDM_ID(dfh) = htonl(rdm_id);
  SRC_PORT(dfh) = htons(agent_id);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = CONTROL;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

  delete [] out_pkt;

  ClearAttrs(attrs);
  delete attrs;
  delete controlblob;
  delete originalHdr;
}

void DiffusionRouting::sendMessageToNext(Message *msg, handle h)
{
  RedirectMessage *originalHdr;
  NRAttribute *originalAttr, *ctrlmsg;
  ControlMessage *controlblob;
  NRAttrVec *attrs;
  DiffPacket out_pkt;
  struct hdr_diff *dfh;
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
  originalHdr->handle = 0;

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  // Create the attribute with the control message
  controlblob = new ControlMessage;
  controlblob->command = SEND_TO_NEXT;
  controlblob->param1 = h;
  controlblob->param2 = 0;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  // Copy Attributes and add originalAttr and controlAttr
  attrs = CopyAttrs(msg->msg_attr_vec);
  attrs->push_back(originalAttr);
  attrs->push_back(ctrlmsg);

  // Send the CONTROL message to diffusion
  out_pkt = AllocateBuffer(attrs);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(attrs, pos);

  // Prepare packet header to send
  LAST_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NEXT_HOP(dfh) = htonl(LOCALHOST_ADDR);
  NUM_ATTR(dfh) = htons(attrs->size());
  PKT_NUM(dfh) = htonl(pkt_count);
  pkt_count++;
  RDM_ID(dfh) = htonl(rdm_id);
  SRC_PORT(dfh) = htons(agent_id);
  VERSION(dfh) = DIFFUSION_VERSION;
  MSG_TYPE(dfh) = CONTROL;
  DATA_LEN(dfh) = htons(len);

  // Send Packet
  snd(out_pkt, sizeof(hdr_diff) + len, diffusion_port);

  delete [] out_pkt;

  ClearAttrs(attrs);
  delete attrs;
  delete controlblob;
  delete originalHdr;
}

#ifdef SCADDS
void DiffusionRouting::run()
{
  DeviceList::iterator itr;
  int status, max_sock, fd;
  bool flag;
  DiffPacket in_pkt;
  fd_set fds;
  struct timeval *tv;
  struct timeval tmv;
  char *debug_str;

  while (listening){
    FD_ZERO(&fds);
    max_sock = 0;

    for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
      (*itr)->AddInFDS(&fds, &max_sock);
    }

    // Get the next timeout
    pthread_mutex_lock(queueMtx);
    tv = eq->eq_nextTimer();
    pthread_mutex_unlock(queueMtx);

    // If we don't have any timeouts, we wait for POLLING_INTERVAL
    if (!tv){
      tv = &tmv;
      tmv.tv_sec = POLLING_INTERVAL;
      tmv.tv_usec = 0;
    }

    fflush(stdout);
    status = select(max_sock+1, &fds, NULL, NULL, tv);

    pthread_mutex_lock(queueMtx);
    if (eq->eq_topInPast()){
      // We got a timeout
      event *e = eq->eq_pop();
      pthread_mutex_unlock(queueMtx);

      // Timeouts
      switch (e->type){

      case INTEREST_TIMER:

	pthread_mutex_lock(drMtx);
	InterestTimeout((Handle_Entry *) e->payload);
	pthread_mutex_unlock(drMtx);

	free(e);

	break;

      case FILTER_KEEPALIVE_TIMER:

	pthread_mutex_lock(drMtx);
	FilterKeepaliveTimeout((Filter_Entry *) e->payload);
	pthread_mutex_unlock(drMtx);

	free(e);

	break;

      case APPLICATION_TIMER:

	ApplicationTimeout((Timer_Entry *) e->payload);

	free(e);

	break;

      default:

	break;

      }
    }
    else{
      // Don't forget to release the lock
      pthread_mutex_unlock(queueMtx);

      if (status > 0){
	do{
	  flag = false;
	  for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
	    fd = (*itr)->CheckInFDS(&fds);
	    if (fd != 0){
	      // Message waiting
	      in_pkt = (*itr)->RecvPacket(fd);
	      recv(in_pkt);

	      // Clear this fd
	      FD_CLR(fd, &fds);
	      status--;
	      flag = true;
	    }
	  }
	} while ((status > 0) && (flag == true));
      }
      else
	if (status < 0){
	  debug_str = new char[SMALL_DEBUG_MESSAGE];
	  sprintf(debug_str, "Select returned %d\n", status);
	  diffPrint(DEBUG_IMPORTANT, debug_str);
	  delete [] debug_str;
	}
    }
  }
}
#endif //scadds


void DiffusionRouting::snd(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator itr;

  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->SendPacket(pkt, len, dst);
  }
}

void DiffusionRouting::recv(DiffPacket pkt)
{
  struct hdr_diff *dfh = HDR_DIFF(pkt);
  Message *rcv_message = NULL;
  int8_t version, msg_type;
  u_int16_t data_len, num_attr, source_port;
  int32_t pkt_num, rdm_id, next_hop, last_hop;

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

  // Check destination
  if (next_hop != LOCALHOST_ADDR)
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

  // Process the incoming message
  if (rcv_message->msg_type == REDIRECT)
    ProcessControlMessage(rcv_message);
  else
    ProcessMessage(rcv_message);

  // We are dome

  delete rcv_message;
#ifdef SCADDS
  // diffPacket deleted when Packet::free() gets called
  // from recv(Packet*, handler*)
  delete [] pkt;
#endif

}

void DiffusionRouting::ProcessControlMessage(Message *msg)
{
  NRSimpleAttribute<void *> *originalHeader = NULL;
  NRAttrVec::iterator place = msg->msg_attr_vec->begin();
  RedirectMessage *originalHdr;
  Filter_Entry *entry;
  handle my_handle;

  // Find the attribute containing the original packet header
  originalHeader = OriginalHdrAttr.find_from(msg->msg_attr_vec, place, &place);
  if (!originalHeader){
    diffPrint(DEBUG_ALWAYS, "Error: DiffusionRouting::ProcessControlMessage: Received an invalid REDIRECT message !\n");
    return;
  }

  // Restore original message header
  originalHdr = (RedirectMessage *) originalHeader->getVal();
  my_handle = originalHdr->handle;
  msg->msg_type = originalHdr->msg_type;
  msg->source_port = originalHdr->source_port;
  msg->pkt_num = originalHdr->pkt_num;
  msg->rdm_id = originalHdr->rdm_id;
  msg->next_hop = originalHdr->next_hop;
  msg->last_hop = originalHdr->last_hop;
  msg->num_attr = originalHdr->num_attr;
  msg->new_message = originalHdr->new_message;

  // Delete attribute from the original set
  msg->msg_attr_vec->erase(place);
  delete originalHeader;

  // Find the right callback
  pthread_mutex_lock(drMtx);

  entry = findFilter(my_handle);
  if (entry && entry->valid){
    // Just to confirm
    if (OneWayMatch(entry->filterAttrs, msg->msg_attr_vec)){
      entry->cb->recv(msg, my_handle);
    }
    else{
      diffPrint(DEBUG_ALWAYS, "Warning: DiffusionRouting::ProcessControlMessage: Filter specified doesn't match incoming message's attributes !\n");
    }
  }
  else{
    diffPrint(DEBUG_IMPORTANT, "Report: DiffusionRouting::ProcessControlMessage: Filter in REDIRECT message was not found (possibly deleted ?)\n");
  }

  pthread_mutex_unlock(drMtx);
}

void DiffusionRouting::ProcessMessage(Message *msg)
{
  CallbackList cbl;
  Callback_Entry *aux;
  HandleList::iterator sub_itr;
  CallbackList::iterator cbl_itr;
  Handle_Entry *entry; 
  NRAttrVec *callbackAttrs;

  // Now, acquire the lock
  pthread_mutex_lock(drMtx);

  for (sub_itr = sub_list.begin(); sub_itr != sub_list.end(); ++sub_itr){
    entry = *sub_itr;
    if ((entry->valid) && (MatchAttrs(msg->msg_attr_vec, entry->attrs)))
      if (entry->cb){
	aux = new Callback_Entry;
	aux->cb = entry->cb;
	aux->subscription_handle = entry->hdl;
	cbl.push_back(aux);
      }
  }

  // We can release the lock now
  pthread_mutex_unlock(drMtx);

  // Now we just call all callback functions
  for (cbl_itr = cbl.begin(); cbl_itr != cbl.end(); ++cbl_itr){
    // Copy attributes
    callbackAttrs = CopyAttrs(msg->msg_attr_vec);

    // Call app-specific callback function
    aux = *cbl_itr;
    aux->cb->recv(msg->msg_attr_vec, aux->subscription_handle);
    delete aux;

    // Clean up callback attributes
    ClearAttrs(callbackAttrs);
    delete callbackAttrs;
  }

  // We are done
  cbl.clear();
}

Handle_Entry * DiffusionRouting::removeHandle(handle my_handle, HandleList *hl)
{
  HandleList::iterator itr;
  Handle_Entry *entry;

  for (itr = hl->begin(); itr != hl->end(); ++itr){
    entry = *itr;
    if (entry->hdl == my_handle){
      hl->erase(itr);
      return entry;
    }
  }
  return NULL;
}

Handle_Entry * DiffusionRouting::findHandle(handle my_handle, HandleList *hl)
{
  HandleList::iterator itr;
  Handle_Entry *entry;

  for (itr = hl->begin(); itr != hl->end(); ++itr){
    entry = *itr;
    if (entry->hdl == my_handle)
      return entry;
  }
  return NULL;
}

Filter_Entry * DiffusionRouting::deleteFilter(handle my_handle)
{
  FilterList::iterator itr;
  Filter_Entry *entry;

  for (itr = filter_list.begin(); itr != filter_list.end(); ++itr){
    entry = *itr;
    if (entry->handle == my_handle){
      filter_list.erase(itr);
      return entry;
    }
  }
  return NULL;
}

Filter_Entry * DiffusionRouting::findFilter(handle my_handle)
{
  FilterList::iterator itr;
  Filter_Entry *entry;

  for (itr = filter_list.begin(); itr != filter_list.end(); ++itr){
    entry = *itr;
    if (entry->handle == my_handle)
      return entry;
  }
  return NULL;
}

bool DiffusionRouting::hasScope(NRAttrVec *attrs)
{
  NRAttribute *temp = NULL;

  temp = NRScopeAttr.find(attrs);
  if (temp)
    return true;

  return false;
}

bool DiffusionRouting::checkSubscription(NRAttrVec *attrs)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;

  // Checks for class attribute and possible scope attribute
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  if (!nrclass)
    return false;

  if (nrscope){
    // Check for global interests
    if ((nrscope->getVal() == NRAttribute::GLOBAL_SCOPE) &&
	(nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
	(nrclass->getOp() == NRAttribute::IS))
      return true;

    // Check for local subscriptions
    if ((nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE) &&
	(nrclass->getOp() != NRAttribute::IS))
      return true;
  }

  if ((nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
      (nrclass->getOp() == NRAttribute::IS))
    return true;

  return false;
}

bool DiffusionRouting::checkPublication(NRAttrVec *attrs)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;

  // Checks for class attribute and possible scope attribute
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  if (!nrclass)
    return false;

  if ((nrclass->getVal() != NRAttribute::DATA_CLASS) ||
      (nrclass->getOp() != NRAttribute::IS))
    return false;

  // No scope attribute is OK
  if (!nrscope)
    return true;

  if ((nrscope->getOp() != NRAttribute::IS) ||
      (nrscope->getVal() != NRAttribute::NODE_LOCAL_SCOPE))
    return false;

  return true;
}

bool DiffusionRouting::checkSend(NRAttrVec *attrs)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;

  // Currently only checks for Class and Scope attributes
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  if (nrclass || nrscope)
    return false;

  return true;
}
