//
// dr.cc           : Diffusion Routing Class
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: dr.cc,v 1.12 2002/09/16 17:57:28 haldar Exp $
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

int InterestCallback::expire()
{
  int retval;

  // Call the interestTimeout function
  retval = drt_->interestTimeout(handle_entry_);

  if (retval < 0)
    delete this;

  return retval;
}

int FilterKeepaliveCallback::expire()
{
  int retval;

  // Call the filterTimeout function
  retval = drt_->filterKeepaliveTimeout(filter_entry_);

  if (retval < 0)
    delete this;

  return retval;
}

int OldAPITimer::expire()
{
  int retval;

  // Call the callback function with the provided API
  retval = cb_->expire(0, p_);

  if (retval < 0)
    delete this;

  return retval;
}

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

NR * NR::createNR(u_int16_t port)
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

void GetLock(pthread_mutex_t *mutex)
{
#ifdef USE_THREADS
  pthread_mutex_lock(mutex);
#endif // USE_THREADS
}

void ReleaseLock(pthread_mutex_t *mutex)
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

  // Initialize timer manager
  timers_manager_ = new TimerManager;

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
  pthread_mutex_init(dr_mtx_, NULL);
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

handle DiffusionRouting::subscribe(NRAttrVec *subscribe_attrs, NR::Callback *cb)
{
  HandleEntry *my_handle;
  NRAttribute *scope_attr;
  TimerCallback *timer_callback;

  // Get lock first
  GetLock(dr_mtx_);

  // Check the published attributes
  if (!checkSubscription(subscribe_attrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the subscribe attributes !\n");
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
  my_handle->hdl_ = next_handle_;
  next_handle_++;
  my_handle->cb_ = (NR::Callback *) cb;
  sub_list_.push_back(my_handle);

  // Copy the attributes   
  my_handle->attrs_ = CopyAttrs(subscribe_attrs);

  // For subscriptions, scope is global if not specified
  if (!hasScope(subscribe_attrs)){
    scope_attr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::GLOBAL_SCOPE);
    my_handle->attrs_->push_back(scope_attr);
  }

  // Create Interest Timer and add it to the queue
  timer_callback = new InterestCallback(this, my_handle);
  timers_manager_->addTimer(SMALL_TIMEOUT, timer_callback);

  // Release lock
  ReleaseLock(dr_mtx_);

  return my_handle->hdl_;
}

int DiffusionRouting::unsubscribe(handle subscription_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  GetLock(dr_mtx_);

  my_handle = findHandle(subscription_handle, &sub_list_);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Handle will be destroyed when next interest timeout happens
  my_handle->valid_ = false;

  // Release the lock
  ReleaseLock(dr_mtx_);

  return OK;
}

handle DiffusionRouting::publish(NRAttrVec *publish_attrs)
{
  HandleEntry *my_handle;
  NRAttribute *scope_attr;

  // Get the lock first
  GetLock(dr_mtx_);

  // Check the published attributes
  if (!checkPublication(publish_attrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the publish attributes !\n");
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Create and Initialize the handle_entry structute
  my_handle = new HandleEntry;
  my_handle->hdl_ = next_handle_;
  next_handle_++;
  pub_list_.push_back(my_handle);

  // Copy the attributes
  my_handle->attrs_ = CopyAttrs(publish_attrs);

  // For publications, scope is local if not specified
  if (!hasScope(publish_attrs)){
    scope_attr = NRScopeAttr.make(NRAttribute::IS, NRAttribute::NODE_LOCAL_SCOPE);
    my_handle->attrs_->push_back(scope_attr);
  }

  // Release the lock
  ReleaseLock(dr_mtx_);

  return my_handle->hdl_;
}

int DiffusionRouting::unpublish(handle publication_handle)
{
  HandleEntry *my_handle = NULL;

  // Get the lock first
  GetLock(dr_mtx_);

  my_handle = removeHandle(publication_handle, &pub_list_);
  if (!my_handle){
    // Handle doesn't exist, return FAIL
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Free structures
  delete my_handle;

  // Release the lock
  ReleaseLock(dr_mtx_);

  return OK;
}

int DiffusionRouting::send(handle publication_handle,
			   NRAttrVec *send_attrs)
{
  Message *my_message;
  HandleEntry *my_handle;
  int8_t send_message_type = DATA;
  struct timeval current_time;

  // Get the lock first
  GetLock(dr_mtx_);

  // Get attributes associated with handle
  my_handle = findHandle(publication_handle, &pub_list_);
  if (!my_handle){
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Check the send attributes
  if (!checkSend(send_attrs)){
    DiffPrint(DEBUG_ALWAYS, "Error : Invalid class/scope attributes in the send attributes !\n");
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  // Check if it is time to send another exploratory data message
  GetTime(&current_time);

  if (TimevalCmp(&current_time, &(my_handle->exploratory_time_)) >= 0){

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
  my_message = new Message(DIFFUSION_VERSION, send_message_type, agent_id_,
			   0, 0, pkt_count_, random_id_, LOCALHOST_ADDR,
			   LOCALHOST_ADDR);
  // Increment pkt_counter
  pkt_count_++;

  // First, we duplicate the 'publish' attributes
  my_message->msg_attr_vec_ = CopyAttrs(my_handle->attrs_);

  // Now, we add the send attributes
  AddAttrs(my_message->msg_attr_vec_, send_attrs);

  // Compute the total number and size of the joined attribute sets
  my_message->num_attr_ = my_message->msg_attr_vec_->size();
  my_message->data_len_ = CalculateSize(my_message->msg_attr_vec_);

  // Release the lock
  ReleaseLock(dr_mtx_);

  // Send Packet
  sendMessageToDiffusion(my_message);

  delete my_message;

  return OK;
}

handle DiffusionRouting::addFilter(NRAttrVec *filter_attrs, u_int16_t priority,
				   FilterCallback *cb)
{
  FilterEntry *filter_entry;
  NRAttrVec *attrs;
  NRAttribute *ctrl_msg_attr;
  ControlMessage *control_blob;
  Message *my_message;
  TimerCallback *timer_callback;

  // Check parameters
  if (!filter_attrs || !cb || priority < FILTER_MIN_PRIORITY || priority > FILTER_MAX_PRIORITY){
    DiffPrint(DEBUG_ALWAYS, "Received invalid parameters when adding filter !\n");
    return FAIL;
  }

  // Get lock first
  GetLock(dr_mtx_);

  // Create and Initialize the handle_entry structute
  filter_entry = new FilterEntry(next_handle_, priority, agent_id_);
  next_handle_++;
  filter_entry->cb_ = (FilterCallback *) cb;
  filter_list_.push_back(filter_entry);

  // Copy attributes (keep them for matching later)
  filter_entry->filter_attrs_ = CopyAttrs(filter_attrs);

  // Copy the attributes (and add the control attr)
  attrs = CopyAttrs(filter_attrs);
  control_blob = new ControlMessage(ADD_UPDATE_FILTER,
				    priority, filter_entry->handle_);

  ctrl_msg_attr = ControlMsgAttr.make(NRAttribute::IS,
				      (void *)control_blob,
				      sizeof(ControlMessage));

  attrs->push_back(ctrl_msg_attr);

  // Initialize message structure
  my_message = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			   0, pkt_count_, random_id_, LOCALHOST_ADDR,
			   LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  my_message->msg_attr_vec_ = attrs;
  my_message->num_attr_ = attrs->size();
  my_message->data_len_ = CalculateSize(attrs);

  // Release the lock
  ReleaseLock(dr_mtx_);

  // Send Packet
  sendMessageToDiffusion(my_message);

  // Add keepalive timer to the event queue
  timer_callback = new FilterKeepaliveCallback(this, filter_entry);
  timers_manager_->addTimer(FILTER_KEEPALIVE_DELAY, timer_callback);

  // Delete message, attribute set and controlblob
  delete my_message;
  delete control_blob;

  return filter_entry->handle_;
}

int DiffusionRouting::removeFilter(handle filter_handle)
{
  FilterEntry *filter_entry = NULL;
  ControlMessage *control_blob;
  NRAttribute *ctrl_msg_attr;
  NRAttrVec *attrs;
  Message *my_message;

  // Get lock first
  GetLock(dr_mtx_);

  filter_entry = findFilter(filter_handle);
  if (!filter_entry){
    // Handle doesn't exist, return FAIL
    ReleaseLock(dr_mtx_);
    return FAIL;
  }

  control_blob = new ControlMessage(REMOVE_FILTER, filter_entry->handle_, 0);

  ctrl_msg_attr = ControlMsgAttr.make(NRAttribute::IS,
				      (void *)control_blob,
				      sizeof(ControlMessage));

  attrs = new NRAttrVec;
  attrs->push_back(ctrl_msg_attr);

  my_message = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			   0, pkt_count_, random_id_, LOCALHOST_ADDR,
			   LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  my_message->msg_attr_vec_ = attrs;
  my_message->num_attr_ = attrs->size();
  my_message->data_len_ = CalculateSize(attrs);

  // Handle will be destroyed when next keepalive timer happens
  filter_entry->valid_ = false;

  // Send Packet
  sendMessageToDiffusion(my_message);

  // Release the lock
  ReleaseLock(dr_mtx_);

  // Delete message
  delete my_message;
  delete control_blob;

  return OK;
}

handle DiffusionRouting::addTimer(int timeout, TimerCallback *callback)
{
  return (timers_manager_->addTimer(timeout, callback));
}

handle DiffusionRouting::addTimer(int timeout, void *p, TimerCallbacks *cb)
{
  TimerCallback *callback;

  callback = new OldAPITimer(cb, p);

  return (addTimer(timeout, callback));
}


bool DiffusionRouting::removeTimer(handle hdl)
{
  return (timers_manager_->removeTimer(hdl));
}

int DiffusionRouting::filterKeepaliveTimeout(FilterEntry *filter_entry)
{
  FilterEntry *my_entry = NULL;
  ControlMessage *control_blob;
  NRAttribute *ctrl_msg_attr;
  NRAttrVec *attrs;
  Message *my_message;

  // Acquire lock first
  GetLock(dr_mtx_);

  if (filter_entry->valid_){
    // Send keepalive
    control_blob = new ControlMessage(ADD_UPDATE_FILTER,
				      filter_entry->priority_,
				      filter_entry->handle_);

    ctrl_msg_attr = ControlMsgAttr.make(NRAttribute::IS,
					(void *)control_blob,
					sizeof(ControlMessage));

    attrs = CopyAttrs(filter_entry->filter_attrs_);
    attrs->push_back(ctrl_msg_attr);

    my_message = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			     0, pkt_count_, random_id_, LOCALHOST_ADDR,
			     LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count_++;

    // Add attributes to the message
    my_message->msg_attr_vec_ = attrs;
    my_message->num_attr_ = attrs->size();
    my_message->data_len_ = CalculateSize(attrs);

    // Send Message
    sendMessageToDiffusion(my_message);

    delete my_message;
    delete control_blob;

    // Release lock
    ReleaseLock(dr_mtx_);

    // Reschedule another filter keepalive timer in event queue
    return (FILTER_KEEPALIVE_DELAY);
  }
  else{
    // Filter was removed
    my_entry = deleteFilter(filter_entry->handle_);

    // We should have removed the correct handle
    if (my_entry != filter_entry){
      DiffPrint(DEBUG_ALWAYS, "DiffusionRouting::KeepaliveTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_entry;

    // Release lock
    ReleaseLock(dr_mtx_);

    return -1;
  }
}

int DiffusionRouting::interestTimeout(HandleEntry *handle_entry)
{
  HandleEntry *my_handle = NULL;
  Message *my_message;

  // Acquire lock first
  GetLock(dr_mtx_);

  if (handle_entry->valid_){
    // Send the interest message if entry is still valid
    my_message = new Message(DIFFUSION_VERSION, INTEREST, agent_id_, 0,
			     0, pkt_count_, random_id_, LOCALHOST_ADDR,
			     LOCALHOST_ADDR);

    // Increment pkt_counter
    pkt_count_++;

    // Add attributes to the message
    my_message->msg_attr_vec_ = CopyAttrs(handle_entry->attrs_);
    my_message->num_attr_ = handle_entry->attrs_->size();
    my_message->data_len_ = CalculateSize(handle_entry->attrs_);

    // Send Packet
    sendMessageToDiffusion(my_message);

    delete my_message;

    // Release lock
    ReleaseLock(dr_mtx_);

    // Reschedule this timer in the queue
    return (INTEREST_DELAY +
	    (int) ((INTEREST_JITTER * (GetRand() * 1.0 / RAND_MAX)) -
		   (INTEREST_JITTER / 2)));
  }
  else{
    // Interest was canceled. Just delete it from the handle_list
    my_handle = removeHandle(handle_entry->hdl_, &sub_list_);

    // We should have removed the correct handle
    if (my_handle != handle_entry){
      DiffPrint(DEBUG_ALWAYS,
		"Error: interestTimeout: Handles should match !\n");
      exit(-1);
    }

    delete my_handle;

    // Release lock
    ReleaseLock(dr_mtx_);

    // Delete timer from the queue
    return -1;
  }
}

int DiffusionRouting::sendMessage(Message *msg, handle h,
				  u_int16_t priority)
{
  RedirectMessage *original_hdr;
  NRAttribute *original_attr, *ctrl_msg_attr;
  ControlMessage *control_blob;
  NRAttrVec *attrs;
  Message *my_message;

  if ((priority < FILTER_MIN_PRIORITY) ||
      (priority > FILTER_KEEP_PRIORITY))
    return FAIL;

  // Create an attribute with the original header
  original_hdr = new RedirectMessage(msg->new_message_, msg->msg_type_,
				     msg->source_port_, msg->data_len_,
				     msg->num_attr_, msg->rdm_id_,
				     msg->pkt_num_, msg->next_hop_,
				     msg->last_hop_, 0,
				     msg->next_port_);

  original_attr = OriginalHdrAttr.make(NRAttribute::IS, (void *)original_hdr,
				       sizeof(RedirectMessage));

  // Create the attribute with the control message
  control_blob = new ControlMessage(SEND_MESSAGE, h, priority);

  ctrl_msg_attr = ControlMsgAttr.make(NRAttribute::IS, (void *)control_blob,
				      sizeof(ControlMessage));

  // Copy Attributes and add originalAttr and controlAttr
  attrs = CopyAttrs(msg->msg_attr_vec_);
  attrs->push_back(original_attr);
  attrs->push_back(ctrl_msg_attr);

  my_message = new Message(DIFFUSION_VERSION, CONTROL, agent_id_, 0,
			   0, pkt_count_, random_id_, LOCALHOST_ADDR,
			   LOCALHOST_ADDR);

  // Increment pkt_counter
  pkt_count_++;

  // Add attributes to the message
  my_message->msg_attr_vec_ = attrs;
  my_message->num_attr_ = attrs->size();
  my_message->data_len_ = CalculateSize(attrs);

  // Send Packet
  sendMessageToDiffusion(my_message);

  delete my_message;
  delete control_blob;
  delete original_hdr;

  return OK;
}

#ifndef NS_DIFFUSION
void DiffusionRouting::doIt()
{
  run(true, WAIT_FOREVER);
}

void DiffusionRouting::doOne(long timeout)
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
  struct timeval tv;
  struct timeval max_tv;

  do{
    FD_ZERO(&fds);
    max_sock = 0;

    // Set the maximum timeout value
    max_tv.tv_sec = (int) (max_timeout / 1000);
    max_tv.tv_usec = (int) ((max_timeout % 1000) * 1000);

    for (itr = in_devices_.begin(); itr != in_devices_.end(); ++itr){
      (*itr)->addInFDS(&fds, &max_sock);
    }

    // Check for the next timer
    timers_manager_->nextTimerTime(&tv);

    if (tv.tv_sec == MAXVALUE){
      // If we don't have any timers, we wait for POLLING_INTERVAL
      if (max_timeout == WAIT_FOREVER){
	tv.tv_sec = POLLING_INTERVAL;
	tv.tv_usec = 0;
      }
      else{
	tv = max_tv;
      }
    }
    else{
      if ((max_timeout != WAIT_FOREVER) && (TimevalCmp(&tv, &max_tv) > 0)){
	// max_timeout value is smaller than next timer's time, so we
	// use themax_timeout value instead
	tv = max_tv;
      }
    }

    status = select(max_sock+1, &fds, NULL, NULL, &tv);

    if (status == 0){
      // Process all timers that have expired
      timers_manager_->executeAllExpiredTimers();
    }

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
  Message *my_msg;
  DeviceList::iterator itr;
  int len;

  my_msg = CopyMessage(msg);
  len = CalculateSize(my_msg->msg_attr_vec_);
  len = len + sizeof(struct hdr_diff);

  for (itr = local_out_devices_.begin(); itr != local_out_devices_.end(); ++itr){
    (*itr)->sendPacket((DiffPacket) my_msg, len, diffusion_port_);
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
  GetLock(dr_mtx_);

  entry = findFilter(my_handle);
  if (entry && entry->valid_){
    // Just to confirm
    if (OneWayMatch(entry->filter_attrs_, msg->msg_attr_vec_)){
      ReleaseLock(dr_mtx_);
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

  ReleaseLock(dr_mtx_);
}

void DiffusionRouting::processMessage(Message *msg)
{
  CallbackList cbl;
  CallbackEntry *aux;
  HandleList::iterator sub_itr;
  CallbackList::iterator cbl_itr;
  HandleEntry *entry; 
  NRAttrVec *callback_attrs;

  // First, acquire the lock
  GetLock(dr_mtx_);

  for (sub_itr = sub_list_.begin(); sub_itr != sub_list_.end(); ++sub_itr){
    entry = *sub_itr;
    if ((entry->valid_) && (MatchAttrs(msg->msg_attr_vec_, entry->attrs_)))
      if (entry->cb_){
	aux = new CallbackEntry(entry->cb_, entry->hdl_);
	cbl.push_back(aux);
      }
  }

  // We can release the lock now
  ReleaseLock(dr_mtx_);

  // Now we just call all callback functions
  for (cbl_itr = cbl.begin(); cbl_itr != cbl.end(); ++cbl_itr){
    // Copy attributes
    callback_attrs = CopyAttrs(msg->msg_attr_vec_);

    // Call app-specific callback function
    aux = *cbl_itr;
    aux->cb_->recv(callback_attrs, aux->subscription_handle_);
    delete aux;

    // Clean up callback attributes
    ClearAttrs(callback_attrs);
    delete callback_attrs;
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
