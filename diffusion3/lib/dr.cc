//
// dr.cc           : Diffusion Routing Class
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: dr.cc,v 1.10 2002/05/13 22:33:45 haldar Exp $
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
  NR::Callback *cb_;
  NR::handle subscription_handle_;

  CallbackEntry(NR::Callback *cb, NR::handle subscription_handle) :
    cb_(cb), subscription_handle_(subscription_handle) {};
};

class TimerEntry {
public:
  handle         hdl_;
  int            timeout_;
  void           *p_;
  TimerCallbacks *cb_;

  TimerEntry(handle hdl, int timeout, void *p, TimerCallbacks *cb) : 
    hdl_(hdl), timeout_(timeout), p_(p), cb_(cb) {};
};

class HandleEntry {
public:
  handle hdl_;
  bool valid_;
  NRAttrVec *attrs_;
  NR::Callback *cb_;
  struct timeval exploratory_time_;

  HandleEntry()
  {
    GetTime(&exploratory_time_);
    valid_ = true;
    cb_ = NULL;
  };

  ~HandleEntry(){

    ClearAttrs(attrs_);
    delete attrs_;
  };
};

#ifdef NS_DIFFUSION
class DiffEventQueue;

int DiffusionRouting::getAgentId(int id = -1) {
  if (id != -1)
    agent_id_ = id;
  return agent_id_;
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
    DiffPrint(DEBUG_ALWAYS, "Error creating receiving thread ! Aborting...\n");
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
  next_handle_ = 1;
  GetTime(&tv);
  SetSeed(&tv);
  pkt_count_ = GetRand();
  random_id_ = GetRand();
  agent_id_ = 0;

  if (port == 0)
    port = DEFAULT_DIFFUSION_PORT;

  diffusion_port_ = port;

  // Initialize event queue
#ifdef NS_DIFFUSION
  eq_ = new DiffEventQueue(da);
#else
  eq_ = new EventQueue;
#endif // NS_DIFFUSION

  // Initialize input device
#ifdef NS_DIFFUSION
  device = new NsLocal(da);
  local_out_devices_.push_back(device);
#endif // NS_DIFFUSION

#ifdef UDP
  device = new UDPLocal(&agent_id_);
  in_devices_.push_back(device);
  local_out_devices_.push_back(device);
#endif // UDP

  // Print initialization message
  DiffPrint(DEBUG_ALWAYS,
	    "Diffusion Routing Agent initializing... Agent Id = %d\n",
	    agent_id_);

#ifdef USE_THREADS
  // Initialize Semaphores
  dr_mtx_ = new pthread_mutex_t;
  queue_mtx_ = new pthread_mutex_t;

  pthread_mutex_init(dr_mtx_, NULL);
  pthread_mutex_init(queue_mtx_, NULL);
#endif // USE_THREADS
}

DiffusionRouting::~DiffusionRouting()
{
  HandleList::iterator itr;
  HandleEntry *current;

  // Delete all Handles
  for (itr = sub_list_.begin(); itr != sub_list_.end(); ++itr){
    current = *itr;
    delete current;
  }

  for (itr = pub_list_.begin(); itr != pub_list_.end(); ++itr){
    current = *itr;
    delete current;
  }
}

handle DiffusionRouting::subscribe(NRAttrVec *subscribeAttrs, NR::Callback *cb)
{
  HandleEntry *my_handle;
  NRAttribute *scopeAttr;

  // Get lock first
  getLock(dr_mtx_);

  // Check the published attributes
  if (!checkSubscription(subscribeAttrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the subscribe attributes !\n");
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
  my_handle->hdl_ = next_handle_;
  next_handle_++;
  my_handle->cb_ = (NR::Callback *) cb;
  sub_list_.push_back(my_handle);

  // Copy the attributes   
  my_handle->attrs_ = CopyAttrs(subscribeAttrs);

  // For subscriptions, scope is global if not specified
  if (!hasScope(subscribeAttrs)){
    scopeAttr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::GLOBAL_SCOPE);
    my_handle->attrs_->push_back(scopeAttr);
  }

  // Send interest and add it to the queue
  interestTimeout(my_handle);

  // Release lock
  releaseLock(dr_mtx_);

  return my_handle->hdl_;
}

int DiffusionRouting::unsubscribe(handle subscription_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  getLock(dr_mtx_);

  my_handle = findHandle(subscription_handle, &sub_list_);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Handle will be destroyed when next interest timeout happens
  my_handle->valid_ = false;

  // Release the lock
  releaseLock(dr_mtx_);

  return OK;
}

handle DiffusionRouting::publish(NRAttrVec *publishAttrs)
{
  HandleEntry *my_handle;
  NRAttribute *scopeAttr;

  // Get the lock first
  getLock(dr_mtx_);

  // Check the published attributes
  if (!checkPublication(publishAttrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the publish attributes !\n");
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
  my_handle->hdl_ = next_handle_;
  next_handle_++;
  pub_list_.push_back(my_handle);

  // Copy the attributes
  my_handle->attrs_ = CopyAttrs(publishAttrs);

  // For publications, scope is local if not specified
  if (!hasScope(publishAttrs)){
    scopeAttr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE);
    my_handle->attrs_->push_back(scopeAttr);
  }

  // Release the lock
  releaseLock(dr_mtx_);

  return my_handle->hdl_;
}

int DiffusionRouting::unpublish(handle publication_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  getLock(dr_mtx_);

  my_handle = removeHandle(publication_handle, &pub_list_);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Free structures
  delete my_handle;

  // Release the lock
  releaseLock(dr_mtx_);

  return OK;
}

int DiffusionRouting::send(handle publication_handle,
			   NRAttrVec *sendAttrs)
{
  Message *myMessage;
  HandleEntry *my_handle;
  int8_t send_message_type = DATA;
  struct timeval current_time;

  // Get the lock first
  getLock(dr_mtx_);

  // Get attributes associated with handle
  my_handle = findHandle(publication_handle, &pub_list_);
  if (!my_handle){
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Check the send attributes
  if (!checkSend(sendAttrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the send attributes !\n");
    releaseLock(dr_mtx_);
    return FAIL;
  }

  // Check if it is time to send another exploratory data message
  GetTime(&current_time);

  if (TimevalCmp(&current_time, &(my_handle->exploratory_time_)) > 0){

    // Check if it is a push data message or a regular data message
    if (isPushData(my_handle->attrs_)){
      // Push data message

      // Update time for the next push exploratory message
      GetTime(&(my_handle->exploratory_time_));
      my_handle->exploratory_time_.tv_sec += PUSH_EXPLORATORY_DELAY;

      send_message_type = PUSH_EXPLORATORY_DATA;
    }
    else{
      // Regular data message

      // Update time for the next exploratory message
      GetTime(&(my_handle->exploratory_time_));
      my_handle->exploratory_time_.tv_sec += EXPLORATORY_DATA_DELAY;
    
      send_message_type = EXPLORATORY_DATA;
    }
  }

  // Initialize message structure
  myMessage = new Message(DIFFUSION_VERSION, send_message_type, agent_id_,
			  0, 0, pkt_count_, random_id_, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);
  // Increment pkt_counter
  pkt_count_++;

  // First, we duplicate the 'publish' attributes
  myMessage->msg_attr_vec_ = CopyAttrs(my_handle->attrs_);

  // Now, we add the send attributes
  AddAttrs(myMessage->msg_attr_vec_, sendAttrs);

  // Compute the total number and size of the joined attribute sets
  myMessage->num_attr_ = myMessage->msg_attr_vec_->size();
  myMessage->data_len_ = CalculateSize(myMessage->msg_attr_vec_);

  // Release the lock
  releaseLock(dr_mtx_);

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
    DiffPrint(DEBUG_ALWAYS, "Received invalid parameters when adding filter !\n");
    return FAIL;
  }

  // Get lock first
  getLock(dr_mtx_);

  // Create and Initialize the handle_entry structute
  my_filter = new FilterEntry(next_handle_, priority, agent_id_);
  next_handle_++;
  my_filter->cb_ = (FilterCallback *) cb;
  filter_list_.push_back(my_filter);

  // Copy attributes (keep them for matching later)
  my_filter->filterAttrs_ = CopyAttrs(filterAttrs);

  // Copy the attributes (and add the control attr)
  attrs = CopyAttrs(filterAttrs);
  controlblob = new ControlMessage(ADD_FILTER, priority, my_filter->handle_);

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  attrs->push_back(ctrlmsg);

  // Initialize message structure
  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			  0, pkt_count_, random_id_, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  myMessage->msg_attr_vec_ = attrs;
  myMessage->num_attr_ = attrs->size();
  myMessage->data_len_ = CalculateSize(attrs);

  // Release the lock
  releaseLock(dr_mtx_);

  // Send Packet
  sendMessageToDiffusion(myMessage);

  // Add keepalive to event queue
  getLock(queue_mtx_);
  eq_->eqAddAfter(FILTER_KEEPALIVE_TIMER, (void *) my_filter,
		  FILTER_KEEPALIVE_DELAY);
  releaseLock(queue_mtx_);

  // Delete message, attribute set and controlblob
  delete myMessage;
  delete controlblob;

  return my_filter->handle_;
}

int DiffusionRouting::removeFilter(handle filterHandle)
{
  FilterEntry *entry = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec *attrs;
  Message *myMessage;

  // Get lock first
  getLock(dr_mtx_);

  entry = findFilter(filterHandle);
  if (!entry){
    // Handle doesn't exist, return FAIL
    releaseLock(dr_mtx_);
    return FAIL;
  }

  controlblob = new ControlMessage(REMOVE_FILTER, entry->handle_, 0);

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  attrs = new NRAttrVec;
  attrs->push_back(ctrlmsg);

  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			  0, pkt_count_, random_id_, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  myMessage->msg_attr_vec_ = attrs;
  myMessage->num_attr_ = attrs->size();
  myMessage->data_len_ = CalculateSize(attrs);

  // Handle will be destroyed when next keepalive timer happens
  entry->valid_ = false;

  // Send Packet
  sendMessageToDiffusion(myMessage);

  // Release the lock
  releaseLock(dr_mtx_);

  // Delete message
  delete myMessage;
  delete controlblob;

  return OK;
}

handle DiffusionRouting::addTimer(int timeout, void *p, TimerCallbacks *cb)
{
  TimerEntry *entry;

  entry = new TimerEntry(next_handle_, timeout, p, cb);

  getLock(queue_mtx_);
  eq_->eqAddAfter(APPLICATION_TIMER, (void *) entry, timeout);
  releaseLock(queue_mtx_);

  next_handle_++;

  return entry->hdl_;
}

#ifndef NS_DIFFUSION
// This function is currently unsupported in the NS implementation of diffusion
int DiffusionRouting::removeTimer(handle hdl)
{
  DiffusionEvent *e;
  TimerEntry *entry;
  int found = -1;

  getLock(queue_mtx_);

  // Find the timer in the queue
  e = eq_->eqFindEvent(APPLICATION_TIMER);
  while (e){
    entry = (TimerEntry *) e->payload_;
    if (entry->hdl_ == hdl){
      found = 0;
      break;
    }

    e = eq_->eqFindNextEvent(APPLICATION_TIMER, e->next_);
  }

  // If timer found, remove it from the queue
  if (e){
    if (eq_->eqRemove(e) != 0){
      DiffPrint(DEBUG_ALWAYS, "Error: Can't remove event from queue !\n");
      exit(-1);
    }

    // Call the application provided delete function
    entry->cb_->del(entry->p_);

    delete entry;
    delete e;
  }

  releaseLock(queue_mtx_);

  return found;
}
#endif // NS_DIFFUSION

void DiffusionRouting::filterKeepaliveTimeout(FilterEntry *entry)
{
  FilterEntry *my_entry = NULL;
  ControlMessage *controlblob;
  NRAttribute *ctrlmsg;
  NRAttrVec *attrs;
  Message *myMessage;

  if (entry->valid_){
    // Send keepalive
    controlblob = new ControlMessage(KEEPALIVE_FILTER, entry->handle_, 0);

    ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

    attrs = new NRAttrVec;
    attrs->push_back(ctrlmsg);

    myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			    0, pkt_count_, random_id_, LOCALHOST_ADDR,
			    LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count_++;

    // Add attributes to the message
    myMessage->msg_attr_vec_ = attrs;
    myMessage->num_attr_ = attrs->size();
    myMessage->data_len_ = CalculateSize(attrs);

    // Send Message
    sendMessageToDiffusion(myMessage);

    // Add another keepalive to event queue
    getLock(queue_mtx_);
    eq_->eqAddAfter(FILTER_KEEPALIVE_TIMER, (void *) entry,
		    FILTER_KEEPALIVE_DELAY);
    releaseLock(queue_mtx_);

    delete myMessage;
    delete controlblob;
  }
  else{
    // Filter was removed
    my_entry = deleteFilter(entry->handle_);

    // We should have removed the correct handle
    if (my_entry != entry){
      DiffPrint(DEBUG_ALWAYS, "DiffusionRouting::KeepaliveTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_entry;
  }
}

void DiffusionRouting::interestTimeout(HandleEntry *entry)
{
  HandleEntry *my_handle = NULL;
  Message *myMessage;

  if (entry->valid_){
    // Send the interest
    myMessage = new Message(DIFFUSION_VERSION, INTEREST, agent_id_, 0,
			    0, pkt_count_, random_id_, LOCALHOST_ADDR,
			    LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count_++;

    // Add attributes to the message
    myMessage->msg_attr_vec_ = CopyAttrs(entry->attrs_);
    myMessage->num_attr_ = entry->attrs_->size();
    myMessage->data_len_ = CalculateSize(entry->attrs_);

    // Send Packet
    sendMessageToDiffusion(myMessage);

    // Add another interest timeout to the queue
    getLock(queue_mtx_);
    eq_->eqAddAfter(INTEREST_TIMER, (void *) entry,
		    INTEREST_DELAY +
		    (int) ((INTEREST_JITTER * (GetRand() * 1.0 / RAND_MAX)) -
			   (INTEREST_JITTER / 2)));
    releaseLock(queue_mtx_);

    delete myMessage;
  }
  else{
    // Interest was canceled. Just delete it from the handle_list
    my_handle = removeHandle(entry->hdl_, &sub_list_);

    // We should have removed the correct handle
    if (my_handle != entry){
      DiffPrint(DEBUG_ALWAYS, "DiffusionRouting::interestTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_handle;
  }
}

void DiffusionRouting::applicationTimeout(TimerEntry *entry)
{
  int new_timeout;

  new_timeout = entry->cb_->expire(entry->hdl_, entry->p_);
  getLock(queue_mtx_);
  if (new_timeout >= 0){
    if (new_timeout > 0){
      // Change the timer's timeout
      entry->timeout_ = new_timeout;
    }

    eq_->eqAddAfter(APPLICATION_TIMER, (void *) entry, entry->timeout_);
  }
  else{
    entry->cb_->del(entry->p_);
    delete entry;
  }

  releaseLock(queue_mtx_);
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
  originalHdr = new RedirectMessage(msg->new_message_, msg->msg_type_,
				    msg->source_port_, msg->data_len_,
				    msg->num_attr_, msg->rdm_id_,
				    msg->pkt_num_, msg->next_hop_,
				    msg->last_hop_, 0,
				    msg->next_port_);

  originalAttr = OriginalHdrAttr.make(NRAttribute::IS, (void *)originalHdr, sizeof(RedirectMessage));

  // Create the attribute with the control message
  controlblob = new ControlMessage(SEND_MESSAGE, h, priority);

  ctrlmsg = ControlMsgAttr.make(NRAttribute::IS, (void *)controlblob, sizeof(ControlMessage));

  // Copy Attributes and add originalAttr and controlAttr
  attrs = CopyAttrs(msg->msg_attr_vec_);
  attrs->push_back(originalAttr);
  attrs->push_back(ctrlmsg);

  myMessage = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			  0, pkt_count_, random_id_, LOCALHOST_ADDR,
			  LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  myMessage->msg_attr_vec_ = attrs;
  myMessage->num_attr_ = attrs->size();
  myMessage->data_len_ = CalculateSize(attrs);

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
  DiffusionEvent *e;
  struct timeval *tv;
  struct timeval tmv;

  do{
    FD_ZERO(&fds);
    tv = NULL;
    max_sock = 0;

    for (itr = in_devices_.begin(); itr != in_devices_.end(); ++itr){
      (*itr)->addInFDS(&fds, &max_sock);
    }

    // Check for the next timer
    getLock(queue_mtx_);
    tv = eq_->eqNextTimer();
    releaseLock(queue_mtx_);

    if (!tv){
      // If we don't have any timers, we wait for POLLING_INTERVAL
      tv = new struct timeval;
      *tv = tmv;
      if (max_timeout == WAIT_FOREVER){
	tv->tv_sec = POLLING_INTERVAL;
	tv->tv_usec = 0;
      }
      else{
	tv->tv_sec = (int) (max_timeout / 1000);
	tv->tv_usec = (int) ((max_timeout % 1000) * 1000);
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
	*tv = tmv;
      }
    }

    status = select(max_sock+1, &fds, NULL, NULL, tv);

    // Let's delete tv before we forget
    delete tv;

    getLock(queue_mtx_);
    if (eq_->eqTopInPast()){
      // We got a timeout
      e = eq_->eqPop();
      releaseLock(queue_mtx_);

      // Timeouts
      switch (e->type_){

      case INTEREST_TIMER:

	getLock(dr_mtx_);
	interestTimeout((HandleEntry *) e->payload_);
	releaseLock(dr_mtx_);

	delete e;

	break;

      case FILTER_KEEPALIVE_TIMER:

	getLock(dr_mtx_);
	filterKeepaliveTimeout((FilterEntry *) e->payload_);
	releaseLock(dr_mtx_);

	delete e;

	break;

      case APPLICATION_TIMER:

	applicationTimeout((TimerEntry *) e->payload_);

	delete e;

	break;

      default:

	delete e;

	break;

      }
    }
    else{
      // Don't forget to release the lock
      releaseLock(queue_mtx_);

      if (status > 0){
	do{
	  flag = false;
	  for (itr = in_devices_.begin(); itr != in_devices_.end(); ++itr){
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
	  DiffPrint(DEBUG_IMPORTANT, "Select returned %d\n", status);
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

  out_pkt = AllocateBuffer(msg->msg_attr_vec_);
  dfh = HDR_DIFF(out_pkt);

  pos = (char *) out_pkt;
  pos = pos + sizeof(struct hdr_diff);

  len = PackAttrs(msg->msg_attr_vec_, pos);

  LAST_HOP(dfh) = htonl(msg->last_hop_);
  NEXT_HOP(dfh) = htonl(msg->next_hop_);
  DIFF_VER(dfh) = msg->version_;
  MSG_TYPE(dfh) = msg->msg_type_;
  DATA_LEN(dfh) = htons(len);
  PKT_NUM(dfh) = htonl(msg->pkt_num_);
  RDM_ID(dfh) = htonl(msg->rdm_id_);
  NUM_ATTR(dfh) = htons(msg->num_attr_);
  SRC_PORT(dfh) = htons(msg->source_port_);

  sendPacketToDiffusion(out_pkt, sizeof(struct hdr_diff) + len, diffusion_port_);

  delete [] out_pkt;
}
#else
void DiffusionRouting::sendMessageToDiffusion(Message *msg)
{
  Message *myMsg;
  DeviceList::iterator itr;
  int len;

  myMsg = CopyMessage(msg);
  len = CalculateSize(myMsg->msg_attr_vec_);
  len = len + sizeof(struct hdr_diff);

  for (itr = local_out_devices_.begin(); itr != local_out_devices_.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) myMsg, len, diffusion_port_);
  }
}
#endif // !NS_DIFFUSION

void DiffusionRouting::sendPacketToDiffusion(DiffPacket pkt, int len, int dst)
{
  DeviceList::iterator itr;

  for (itr = local_out_devices_.begin(); itr != local_out_devices_.end(); ++itr){
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
  version = DIFF_VER(dfh);
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
  rcv_message->msg_attr_vec_ = UnpackAttrs(pkt, num_attr);

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
  if (msg->version_ != DIFFUSION_VERSION)
    return;

  // Check destination
  if (msg->next_hop_ != LOCALHOST_ADDR)
    return;

  // Process the incoming message
  if (msg->msg_type_ == REDIRECT)
    processControlMessage(msg);
  else
    processMessage(msg);
}

void DiffusionRouting::processControlMessage(Message *msg)
{
  NRSimpleAttribute<void *> *original_header_attr = NULL;
  NRAttrVec::iterator place = msg->msg_attr_vec_->begin();
  RedirectMessage *original_header;
  FilterEntry *entry;
  handle my_handle;

  // Find the attribute containing the original packet header
  original_header_attr = OriginalHdrAttr.find_from(msg->msg_attr_vec_,
						   place, &place);
  if (!original_header_attr){
    DiffPrint(DEBUG_ALWAYS, "Error: Received an invalid REDIRECT message !\n");
    return;
  }

  // Restore original message header
  original_header = (RedirectMessage *) original_header_attr->getVal();
  my_handle = original_header->handle_;
  msg->msg_type_ = original_header->msg_type_;
  msg->source_port_ = original_header->source_port_;
  msg->pkt_num_ = original_header->pkt_num_;
  msg->rdm_id_ = original_header->rdm_id_;
  msg->next_hop_ = original_header->next_hop_;
  msg->last_hop_ = original_header->last_hop_;
  msg->num_attr_ = original_header->num_attr_;
  msg->new_message_ = original_header->new_message_;
  msg->next_port_ = original_header->next_port_;

  // Delete attribute from the original set
  msg->msg_attr_vec_->erase(place);
  delete original_header_attr;

  // Find the right callback
  getLock(dr_mtx_);

  entry = findFilter(my_handle);
  if (entry && entry->valid_){
    // Just to confirm
    if (OneWayMatch(entry->filterAttrs_, msg->msg_attr_vec_)){
      releaseLock(dr_mtx_);
      entry->cb_->recv(msg, my_handle);
      return;
    }
    else{
      DiffPrint(DEBUG_ALWAYS,
		"Warning: Filter doesn't match incoming message's attributes !\n");
    }
  }
  else{
    DiffPrint(DEBUG_IMPORTANT,
	      "Report: Cannot find filter (possibly deleted ?)\n");
  }

  releaseLock(dr_mtx_);
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
  getLock(dr_mtx_);

  for (sub_itr = sub_list_.begin(); sub_itr != sub_list_.end(); ++sub_itr){
    entry = *sub_itr;
    if ((entry->valid_) && (MatchAttrs(msg->msg_attr_vec_, entry->attrs_)))
      if (entry->cb_){
	aux = new CallbackEntry(entry->cb_, entry->hdl_);
	cbl.push_back(aux);
      }
  }

  // We can release the lock now
  releaseLock(dr_mtx_);

  // Now we just call all callback functions
  for (cbl_itr = cbl.begin(); cbl_itr != cbl.end(); ++cbl_itr){
    // Copy attributes
    callbackAttrs = CopyAttrs(msg->msg_attr_vec_);

    // Call app-specific callback function
    aux = *cbl_itr;
    aux->cb_->recv(msg->msg_attr_vec_, aux->subscription_handle_);
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
    if (entry->hdl_ == my_handle){
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
    if (entry->hdl_ == my_handle)
      return entry;
  }
  return NULL;
}

FilterEntry * DiffusionRouting::deleteFilter(handle my_handle)
{
  FilterList::iterator itr;
  FilterEntry *entry;

  for (itr = filter_list_.begin(); itr != filter_list_.end(); ++itr){
    entry = *itr;
    if (entry->handle_ == my_handle){
      filter_list_.erase(itr);
      return entry;
    }
  }
  return NULL;
}

FilterEntry * DiffusionRouting::findFilter(handle my_handle)
{
  FilterList::iterator itr;
  FilterEntry *entry;

  for (itr = filter_list_.begin(); itr != filter_list_.end(); ++itr){
    entry = *itr;
    if (entry->handle_ == my_handle)
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

  // We first try to locate both class and scope attributes
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  // There must be a class attribute in subscriptions
  if (!nrclass)
    return false;

  if (nrscope){
    // This subcription has both class and scope attribute. So, we
    // check if class/scope attributes comply with the Diffusion
    // Routing API

    // Must check scope's operator. The API requires it to be "IS"
    if (nrscope->getOp() != NRAttribute::IS)
      return false;

    // Ok, so first check if this is a global subscription
    if ((nrscope->getVal() == NRAttribute::GLOBAL_SCOPE) &&
	(nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
	(nrclass->getOp() == NRAttribute::IS))
      return true;

    // Check for local subscriptions
    if (nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE)
      return true;

    // Just to be sure we did not miss any case
    return false;
  }

  // If there is no scope attribute, we will insert one later if this
  // subscription looks like a global subscription
  if ((nrclass->getVal() == NRAttribute::INTEREST_CLASS) &&
      (nrclass->getOp() == NRAttribute::IS))
    return true;

  return false;
}

bool DiffusionRouting::checkPublication(NRAttrVec *attrs)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;

  // We first try to locate both class and scope attributes
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  // There must be a class attribute in the publication
  if (!nrclass)
    return false;

  // In addition, the Diffusion Routing API requires the class
  // attribute to be set to "IS DATA_CLASS"
  if ((nrclass->getVal() != NRAttribute::DATA_CLASS) ||
      (nrclass->getOp() != NRAttribute::IS))
    return false;

  if (nrscope){
    // Ok, so this publication has both class and scope attributes. We
    // now have to check if they comply to the Diffusion Routing API
    // semantics for publish

    // Must check scope's operator. The API requires it to be "IS"
    if (nrscope->getOp() != NRAttribute::IS)
      return false;

    // We accept both global and local scope data messages
    if ((nrscope->getVal() == NRAttribute::GLOBAL_SCOPE) ||
	(nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE))
      return true;

    // Just not to miss any case
    return false;
  }

  // A publish without a scope attribute is fine, we will include a
  // default NODE_LOCAL_SCOPE attribute later
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

bool DiffusionRouting::isPushData(NRAttrVec *attrs)
{
  NRSimpleAttribute<int> *nrclass = NULL;
  NRSimpleAttribute<int> *nrscope = NULL;

  // Currently only checks for Class and Scope attributes
  nrclass = NRClassAttr.find(attrs);
  nrscope = NRScopeAttr.find(attrs);

  // We should have both class and scope
  if (nrclass && nrscope){
    if (nrscope->getVal() == NRAttribute::NODE_LOCAL_SCOPE)
      return false;
    return true;
  }
  else{
    DiffPrint(DEBUG_ALWAYS, "Error: Cannot find class/scope attributes !\n");
    return false;
  }
}
