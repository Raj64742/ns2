//
// dr.cc           : Diffusion Routing Class
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: dr.cc,v 1.8 2002/03/21 19:30:55 haldar Exp $
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

#include <stdlib.h>
#include <stdio.h>

#include "dr.hh"

class CallbackEntry {
public:
  NR::Callback *cb;
  NR::handle subscription_handle;

  CallbackEntry(NR::Callback *_cb, NR::handle _subscription_handle) :
    cb(_cb), subscription_handle(_subscription_handle) {};
};

class TimerEntry {
public:
  handle         hdl;
  int            timeout;
  void           *p;
  TimerCallbacks *cb;

  TimerEntry(handle _hdl, int _timeout, void *_p, TimerCallbacks *_cb) : 
    hdl(_hdl), timeout(_timeout), p(_p), cb(_cb) {};
};

class HandleEntry {
public:
  handle hdl;
  bool valid;
  NRAttrVec *attrs;
  NR::Callback *cb;

  HandleEntry()
  {
    valid = true;
    cb = NULL;
  };

  ~HandleEntry(){

    ClearAttrs(attrs);
    delete attrs;
  };
};

#ifdef NS_DIFFUSION
class DiffEventQueue;

int DiffusionRouting::get_agentid(int id = -1) {
  if (id != -1)
    agent_id = id;
  return agent_id;
}

NR * NR::create_ns_NR(u_int16_t port, DiffAppAgent *da) {
  return(new DiffusionRouting(port, da));
}
#else
NR *dr = NULL;

#ifdef USE_THREADS
void * ReceiveThread(void *dr)
{
  // Never returns
  ((DiffusionRouting *)dr)->run(true, WAIT_FOREVER);

  return NULL;
}
#endif // USE_THREADS

NR * NR::createNR(u_int16_t port = 0)
{
  // Create Diffusion Routing Class
  if (dr)
    return dr;

  dr = new DiffusionRouting(port);

#ifdef USE_THREADS
  int retval;
  pthread_t thread;

  // Fork a thread for receiving Messages
  retval = pthread_create(&thread, NULL, &ReceiveThread, (void *)dr);

  if (retval){
    diffPrint(DEBUG_ALWAYS, "Error creating receiving thread ! Aborting...\n");
    exit(-1);
  }
#endif // USE_THREADS

  return dr;
}
#endif // NS_DIFFUSION

void getLock(pthread_mutex_t *mutex)
{
#ifdef USE_THREADS
  pthread_mutex_lock(mutex);
#endif // USE_THREADS
}

void releaseLock(pthread_mutex_t *mutex)
{
#ifdef USE_THREADS
  pthread_mutex_unlock(mutex);
#endif // USE_THREADS
}

#ifdef NS_DIFFUSION
DiffusionRouting::DiffusionRouting(u_int16_t port, DiffAppAgent *da)
#else
DiffusionRouting::DiffusionRouting(u_int16_t port)
#endif // NS_DIFFUSION
{
  struct timeval tv;
  DiffusionIO *device;

  // Initialize basic stuff
  next_handle = 1;
  getTime(&tv);
  setSeed(&tv);
  pkt_count = getRand();
  rdm_id = getRand();
  agent_id = 0;

  if (port == 0)
    port = DEFAULT_DIFFUSION_PORT;

  diffusion_port = port;

  // Initialize event queue
#ifdef NS_DIFFUSION
  eq = new DiffEventQueue(da);
#else
  eq = new EventQueue;
#endif // NS_DIFFUSION

  // Initialize input device
#ifdef NS_DIFFUSION
  device = new NsLocal(da);
  local_out_devices.push_back(device);
#endif // NS_DIFFUSION

#ifdef UDP
  device = new UDPLocal(&agent_id);
  in_devices.push_back(device);
  local_out_devices.push_back(device);
#endif // UDP

  // Print initialization message
  diffPrint(DEBUG_ALWAYS,
	    "Diffusion Routing Agent initializing... Agent Id = %d\n",
	    agent_id);

#ifdef USE_THREADS
  // Initialize Semaphores
  drMtx = new pthread_mutex_t;
  queueMtx = new pthread_mutex_t;

  pthread_mutex_init(drMtx, NULL);
  pthread_mutex_init(queueMtx, NULL);
#endif // USE_THREADS
}

DiffusionRouting::~DiffusionRouting()
{
  HandleList::iterator itr;
  HandleEntry *current;

  // Delete all Handles
  for (itr = sub_list.begin(); itr != sub_list.end(); ++itr){
    current = *itr;
    delete current;
  }

  for (itr = pub_list.begin(); itr != pub_list.end(); ++itr){
    current = *itr;
    delete current;
  }
}

handle DiffusionRouting::subscribe(NRAttrVec *subscribeAttrs, NR::Callback *cb)
{
  HandleEntry *my_handle;
  NRAttribute *scopeAttr;

  // Get lock first
  getLock(drMtx);

  // Check the published attributes
  if (!checkSubscription(subscribeAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the subscribe attributes !\n");
    releaseLock(drMtx);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
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
  interestTimeout(my_handle);

  // Release lock
  releaseLock(drMtx);

  return my_handle->hdl;
}

int DiffusionRouting::unsubscribe(handle subscription_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  getLock(drMtx);

  my_handle = findHandle(subscription_handle, &sub_list);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    releaseLock(drMtx);
    return FAIL;
  }

  // Handle will be destroyed when next interest timeout happens
  my_handle->valid = false;

  // Release the lock
  releaseLock(drMtx);

  return OK;
}

handle DiffusionRouting::publish(NRAttrVec *publishAttrs)
{
  HandleEntry *my_handle;
  NRAttribute *scopeAttr;

  // Get the lock first
  getLock(drMtx);

  // Check the published attributes
  if (!checkPublication(publishAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the publish attributes !\n");
    releaseLock(drMtx);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
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
  releaseLock(drMtx);

  return my_handle->hdl;
}

int DiffusionRouting::unpublish(handle publication_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  getLock(drMtx);

  my_handle = removeHandle(publication_handle, &pub_list);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    releaseLock(drMtx);
    return FAIL;
  }

  // Free structures
  delete my_handle;

  // Release the lock
  releaseLock(drMtx);

  return OK;
}

int DiffusionRouting::send(handle publication_handle,
			   NRAttrVec *sendAttrs)
{
  Message *myMessage;
  HandleEntry *my_handle;

  // Get the lock first
  getLock(drMtx);

  // Get attributes associated with handle
  my_handle = findHandle(publication_handle, &pub_list);
  if (!my_handle){
    releaseLock(drMtx);
    return FAIL;
  }

  // Check the send attributes
  if (!checkSend(sendAttrs)){
    diffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the send attributes !\n");
    releaseLock(drMtx);
    return FAIL;
  }

  // Initialize message structure
  myMessage = new Message(DIFFUSION_VERSION, DATA, agent_id, 0,
			  0, pkt_count, rdm_id, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);
  // Increment pkt_counter
  pkt_count++;

  // First, we duplicate the 'publish' attributes
  myMessage->msg_attr_vec = CopyAttrs(my_handle->attrs);

  // Now, we add the send attributes
  AddAttrs(myMessage->msg_attr_vec, sendAttrs);

  // Compute the total number and size of the joined attribute sets
  myMessage->num_attr = myMessage->msg_attr_vec->size();
  myMessage->data_len = CalculateSize(myMessage->msg_attr_vec);

  // Release the lock
  releaseLock(drMtx);

  // Send Packet
  sendMessageToDiffusion(myMessage);

  delete myMessage;

  return OK;
}

handle DiffusionRouting::addFilter(NRAttrVec *filterAttrs, u_int16_t priority,
				   FilterCallback *cb)
{
  FilterEntry *my_filter;
  NRAttrVec *attrs;
  NRAttribute *ctrlmsg;
  ControlMessage *controlblob;
  Message *myMessage;

  // Check parameters
  if (!filterAttrs || !cb || priority < FILTER_MIN_PRIORITY || priority > FILTER_MAX_PRIORITY){
    diffPrint(DEBUG_ALWAYS, "Received invalid parameters when adding filter !\n");
    return FAIL;
  }

  // Get lock first
  getLock(drMtx);

  // Create and Initialize the handle_entry structute
  my_filter = new FilterEntry(next_handle, priority, agent_id);
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

  // Initialize message structure
  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id, 0,
			  0, pkt_count, rdm_id, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count++;

  // Add attributes to the message
  myMessage->msg_attr_vec = attrs;
  myMessage->num_attr = attrs->size();
  myMessage->data_len = CalculateSize(attrs);

  // Release the lock
  releaseLock(drMtx);

  // Send Packet
  sendMessageToDiffusion(myMessage);

  // Add keepalive to event queue
  getLock(queueMtx);
  eq->eqAddAfter(FILTER_KEEPALIVE_TIMER, (void *) my_filter,
		  FILTER_KEEPALIVE_DELAY);
  releaseLock(queueMtx);

  // Delete message, attribute set and controlblob
  delete myMessage;
  delete controlblob;

  return my_filter->handle;
}

int DiffusionRouting::removeFilter(handle filterHandle)
{
  FilterEntry *entry = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec *attrs;
  Message *myMessage;

  // Get lock first
  getLock(drMtx);

  entry = findFilter(filterHandle);
  if (!entry){
    // Handle doesn't exist, return FAIL
    releaseLock(drMtx);
    return FAIL;
  }

  controlblob = new ControlMessage;
  controlblob->command = REMOVE_FILTER;
  controlblob->param1 = entry->handle;
  controlblob->param2 = 0;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  attrs = new NRAttrVec;
  attrs->push_back(ctrlmsg);

  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id, 0,
			  0, pkt_count, rdm_id, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count++;

  // Add attributes to the message
  myMessage->msg_attr_vec = attrs;
  myMessage->num_attr = attrs->size();
  myMessage->data_len = CalculateSize(attrs);

  // Handle will be destroyed when next keepalive timer happens
  entry->valid = false;

  // Send Packet
  sendMessageToDiffusion(myMessage);

  // Release the lock
  releaseLock(drMtx);

  // Delete message
  delete myMessage;
  delete controlblob;

  return OK;
}

handle DiffusionRouting::addTimer(int timeout, void *p, TimerCallbacks *cb)
{
  TimerEntry *entry;

  entry = new TimerEntry(next_handle, timeout, p, cb);

  getLock(queueMtx);
  eq->eqAddAfter(APPLICATION_TIMER, (void *) entry, timeout);
  releaseLock(queueMtx);

  next_handle++;

  return entry->hdl;
}

#ifndef NS_DIFFUSION
// This function is currently unsupported in the NS implementation of diffusion
int DiffusionRouting::removeTimer(handle hdl)
{
  event *e;
  TimerEntry *entry;
  int found = -1;

  getLock(queueMtx);

  // Find the timer in the queue
  e = eq->eqFindEvent(APPLICATION_TIMER);
  while (e){
    entry = (TimerEntry *) e->payload;
    if (entry->hdl == hdl){
      found = 0;
      break;
    }

    e = eq->eqFindNextEvent(APPLICATION_TIMER, e->next);
  }

  // If timer found, remove it from the queue
  if (e){
    if (eq->eqRemove(e) != 0){
      diffPrint(DEBUG_ALWAYS, "Error: Can't remove event from queue !\n");
      exit(-1);
    }

    // Call the application provided delete function
    entry->cb->del(entry->p);

    delete entry;
    free(e);
  }

  releaseLock(queueMtx);

  return found;
}
#endif // NS_DIFFUSION

void DiffusionRouting::filterKeepaliveTimeout(FilterEntry *entry)
{
  FilterEntry *my_handle = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec *attrs;
  Message *myMessage;

  if (entry->valid){
    // Send keepalive
    controlblob = new ControlMessage;
    controlblob->command = KEEPALIVE_FILTER;
    controlblob->param1 = entry->handle;
    controlblob->param2 = 0;

    ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

    attrs = new NRAttrVec;
    attrs->push_back(ctrlmsg);

    myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id, 0,
			    0, pkt_count, rdm_id, LOCALHOST_ADDR,
			    LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count++;

    // Add attributes to the message
    myMessage->msg_attr_vec = attrs;
    myMessage->num_attr = attrs->size();
    myMessage->data_len = CalculateSize(attrs);

    // Send Message
    sendMessageToDiffusion(myMessage);

    // Add another keepalive to event queue
    getLock(queueMtx);
    eq->eqAddAfter(FILTER_KEEPALIVE_TIMER, (void *) entry,
		    FILTER_KEEPALIVE_DELAY);
    releaseLock(queueMtx);

    delete myMessage;
    delete controlblob;
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

void DiffusionRouting::interestTimeout(HandleEntry *entry)
{
  HandleEntry *my_handle = NULL;
  Message *myMessage;

  if (entry->valid){
    // Send the interest
    myMessage = new Message(DIFFUSION_VERSION, INTEREST, agent_id, 0,
			    0, pkt_count, rdm_id, LOCALHOST_ADDR,
			    LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count++;

    // Add attributes to the message
    myMessage->msg_attr_vec = CopyAttrs(entry->attrs);
    myMessage->num_attr = entry->attrs->size();
    myMessage->data_len = CalculateSize(entry->attrs);

    // Send Packet
    sendMessageToDiffusion(myMessage);

    // Add another interest timeout to the queue
    getLock(queueMtx);
    eq->eqAddAfter(INTEREST_TIMER, (void *) entry,
		    INTEREST_DELAY +
		    (int) ((INTEREST_JITTER * (getRand() * 1.0 / RAND_MAX)) -
			   (INTEREST_JITTER / 2)));
    releaseLock(queueMtx);

    delete myMessage;
  }
  else{
    // Interest was canceled. Just delete it from the handle_list
    my_handle = removeHandle(entry->hdl, &sub_list);

    // We should have removed the correct handle
    if (my_handle != entry){
      diffPrint(DEBUG_ALWAYS, "DiffusionRouting::interestTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_handle;
  }
}

void DiffusionRouting::applicationTimeout(TimerEntry *entry)
{
  int new_timeout;

  new_timeout = entry->cb->expire(entry->hdl, entry->p);
  getLock(queueMtx);
  if (new_timeout >= 0){
    if (new_timeout > 0){
      // Change the timer's timeout
      entry->timeout = new_timeout;
    }

    eq->eqAddAfter(APPLICATION_TIMER, (void *) entry, entry->timeout);
  }
  else{
    entry->cb->del(entry->p);
    delete entry;
  }

  releaseLock(queueMtx);
}

int DiffusionRouting::sendMessage(Message *msg, handle h,
				  u_int16_t priority = FILTER_KEEP_PRIORITY)
{
  RedirectMessage *originalHdr;
  NRAttribute *originalAttr, *ctrlmsg;
  ControlMessage *controlblob;
  NRAttrVec *attrs;
  Message *myMessage;

  if ((priority < FILTER_MIN_PRIORITY) ||
      (priority > FILTER_KEEP_PRIORITY))
    return FAIL;

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
  originalHdr->handle = 0;

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  // Create the attribute with the control message
  controlblob = new ControlMessage;
  controlblob->command = SEND_MESSAGE;
  controlblob->param1 = h;
  controlblob->param2 = priority;

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  // Copy Attributes and add originalAttr and controlAttr
  attrs = CopyAttrs(msg->msg_attr_vec);
  attrs->push_back(originalAttr);
  attrs->push_back(ctrlmsg);

  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id, 0,
			  0, pkt_count, rdm_id, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count++;

  // Add attributes to the message
  myMessage->msg_attr_vec = attrs;
  myMessage->num_attr = attrs->size();
  myMessage->data_len = CalculateSize(attrs);

  // Send Packet
  sendMessageToDiffusion(myMessage);

  delete myMessage;
  delete controlblob;
  delete originalHdr;

  return OK;
}

#ifndef NS_DIFFUSION
void DiffusionRouting::doIt()
{
  run(true, WAIT_FOREVER);
}

void DiffusionRouting::doOne(long timeout = WAIT_FOREVER)
{
  run(false, timeout);
}

void DiffusionRouting::run(bool wait_condition, long max_timeout)
{
  DeviceList::iterator itr;
  int status, max_sock, fd;
  bool flag;
  DiffPacket in_pkt;
  fd_set fds;
  struct timeval *tv;
  struct timeval tmv;

  do{
    FD_ZERO(&fds);
    max_sock = 0;

    for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
      (*itr)->addInFDS(&fds, &max_sock);
    }

    // Check for the next timer
    getLock(queueMtx);
    tv = eq->eqNextTimer();
    releaseLock(queueMtx);

    if (!tv){
      // If we don't have any timers, we wait for POLLING_INTERVAL
      tv = &tmv;
      if (max_timeout == WAIT_FOREVER){
	tmv.tv_sec = POLLING_INTERVAL;
	tmv.tv_usec = 0;
      }
      else{
	tmv.tv_sec = (int) (max_timeout / 1000);
	tmv.tv_usec = (int) ((max_timeout % 1000) * 1000);
      }
    }
    else{
      tmv.tv_sec = (int) (max_timeout / 1000);
      tmv.tv_usec = (int) ((max_timeout % 1000) * 1000);
      if ((max_timeout != WAIT_FOREVER) && ((tv->tv_sec > tmv.tv_sec)
					    || ((tv->tv_sec == tmv.tv_sec) &&
					(tv->tv_usec > tmv.tv_usec)))){
	// Timeout value is smaller than next timer's time
	// so, we use the max_timeout value instead
	tv = &tmv;
      }
    }

    status = select(max_sock+1, &fds, NULL, NULL, tv);

    getLock(queueMtx);
    if (eq->eqTopInPast()){
      // We got a timeout
      event *e = eq->eqPop();
      releaseLock(queueMtx);

      // Timeouts
      switch (e->type){

      case INTEREST_TIMER:

	getLock(drMtx);
	interestTimeout((HandleEntry *) e->payload);
	releaseLock(drMtx);

	free(e);

	break;

      case FILTER_KEEPALIVE_TIMER:

	getLock(drMtx);
	filterKeepaliveTimeout((FilterEntry *) e->payload);
	releaseLock(drMtx);

	free(e);

	break;

      case APPLICATION_TIMER:

	applicationTimeout((TimerEntry *) e->payload);

	free(e);

	break;

      default:

	break;

      }
    }
    else{
      // Don't forget to release the lock
      releaseLock(queueMtx);

      if (status > 0){
	do{
	  flag = false;
	  for (itr = in_devices.begin(); itr != in_devices.end(); ++itr){
	    fd = (*itr)->checkInFDS(&fds);
	    if (fd != 0){
	      // Message waiting
	      in_pkt = (*itr)->recvPacket(fd);
	      recvPacket(in_pkt);

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
	  diffPrint(DEBUG_IMPORTANT, "Select returned %d\n", status);
	}
    }
  } while (wait_condition);
}

#endif // NS_DIFFUSION

#ifndef NS_DIFFUSION
void DiffusionRouting::sendMessageToDiffusion(Message *msg)
{
  DiffPacket out_pkt = NULL;
  struct hdr_diff *dfh;
  char *pos;
  int len;

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

  sendPacketToDiffusion(out_pkt, sizeof(struct hdr_diff) + len, diffusion_port);

  delete [] out_pkt;
}
#else
void DiffusionRouting::sendMessageToDiffusion(Message *msg)
{
  Message *myMsg;
  DeviceList::iterator itr;
  int len;

  myMsg = CopyMessage(msg);
  len = CalculateSize(myMsg->msg_attr_vec);
  len = len + sizeof(struct hdr_diff);

  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) myMsg, len, diffusion_port);
  }
}
#endif // !NS_DIFFUSION

void DiffusionRouting::sendPacketToDiffusion(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator itr;

  for (itr = local_out_devices.begin(); itr != local_out_devices.end(); ++itr){
    (*itr)->sendPacket(pkt, len, dst);
  }
}

#ifndef NS_DIFFUSION
void DiffusionRouting::recvPacket(DiffPacket pkt)
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

  // Create a message structure from the incoming packet
  rcv_message = new Message(version, msg_type, source_port, data_len,
			    num_attr, pkt_num, rdm_id, next_hop, last_hop);

  // Read all attributes into the Message structure
  rcv_message->msg_attr_vec = UnpackAttrs(pkt, num_attr);

  // Process the incoming message
  recvMessage(rcv_message);

  // We are done
  delete rcv_message;
  delete [] pkt;
}
#endif // !NS_DIFFUSION

void DiffusionRouting::recvMessage(Message *msg)
{
  // Check version
  if (msg->version != DIFFUSION_VERSION)
    return;

  // Check destination
  if (msg->next_hop != (int)LOCALHOST_ADDR)
    return;

  // Process the incoming message
  if (msg->msg_type == REDIRECT)
    processControlMessage(msg);
  else
    processMessage(msg);
}

void DiffusionRouting::processControlMessage(Message *msg)
{
  NRSimpleAttribute<void *> *originalHeader = NULL;
  NRAttrVec::iterator place = msg->msg_attr_vec->begin();
  RedirectMessage *originalHdr;
  FilterEntry *entry;
  handle my_handle;

  // Find the attribute containing the original packet header
  originalHeader = OriginalHdrAttr.find_from(msg->msg_attr_vec, place, &place);
  if (!originalHeader){
    diffPrint(DEBUG_ALWAYS, "Error: Received an invalid REDIRECT message !\n");
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
  msg->next_port = originalHdr->next_port;

  // Delete attribute from the original set
  msg->msg_attr_vec->erase(place);
  delete originalHeader;

  // Find the right callback
  getLock(drMtx);

  entry = findFilter(my_handle);
  if (entry && entry->valid){
    // Just to confirm
    if (OneWayMatch(entry->filterAttrs, msg->msg_attr_vec)){
      releaseLock(drMtx);
      entry->cb->recv(msg, my_handle);
      return;
    }
    else{
      diffPrint(DEBUG_ALWAYS, "Warning: Filter specified doesn't match incoming message's attributes !\n");
    }
  }
  else{
    diffPrint(DEBUG_IMPORTANT, "Report: Filter in REDIRECT message was not found (possibly deleted ?)\n");
  }

  releaseLock(drMtx);
}

void DiffusionRouting::processMessage(Message *msg)
{
  CallbackList cbl;
  CallbackEntry *aux;
  HandleList::iterator sub_itr;
  CallbackList::iterator cbl_itr;
  HandleEntry *entry; 
  NRAttrVec *callbackAttrs;

  // First, acquire the lock
  getLock(drMtx);

  for (sub_itr = sub_list.begin(); sub_itr != sub_list.end(); ++sub_itr){
    entry = *sub_itr;
    if ((entry->valid) && (MatchAttrs(msg->msg_attr_vec, entry->attrs)))
      if (entry->cb){
	aux = new CallbackEntry(entry->cb, entry->hdl);
	cbl.push_back(aux);
      }
  }

  // We can release the lock now
  releaseLock(drMtx);

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

HandleEntry * DiffusionRouting::removeHandle(handle my_handle, HandleList *hl)
{
  HandleList::iterator itr;
  HandleEntry *entry;

  for (itr = hl->begin(); itr != hl->end(); ++itr){
    entry = *itr;
    if (entry->hdl == my_handle){
      hl->erase(itr);
      return entry;
    }
  }
  return NULL;
}

HandleEntry * DiffusionRouting::findHandle(handle my_handle, HandleList *hl)
{
  HandleList::iterator itr;
  HandleEntry *entry;

  for (itr = hl->begin(); itr != hl->end(); ++itr){
    entry = *itr;
    if (entry->hdl == my_handle)
      return entry;
  }
  return NULL;
}

FilterEntry * DiffusionRouting::deleteFilter(handle my_handle)
{
  FilterList::iterator itr;
  FilterEntry *entry;

  for (itr = filter_list.begin(); itr != filter_list.end(); ++itr){
    entry = *itr;
    if (entry->handle == my_handle){
      filter_list.erase(itr);
      return entry;
    }
  }
  return NULL;
}

FilterEntry * DiffusionRouting::findFilter(handle my_handle)
{
  FilterList::iterator itr;
  FilterEntry *entry;

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
