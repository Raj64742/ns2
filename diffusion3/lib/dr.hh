//
// dr.hh           : Diffusion Routing Class Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: dr.hh,v 1.12 2002/09/16 17:57:28 haldar Exp $
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

// This file defines Diffusion Routing's Publish/Subscribe, Filter and
// Timer APIs. It is included from diffapp.hh (and therefore
// applications and filters should include diffapp.hh instead). For a
// detailed description of the API, please look at the Diffusion
// Routing API document (available at the SCADDS website:
// http://www.isi.edu/scadds).

#ifndef _DR_HH_
#define _DR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <pthread.h>
#include <string.h>

#include "main/timers.hh"
#include "main/filter.hh"
#include "main/config.hh"
#include "main/iodev.hh"
#include "main/tools.hh"

#ifdef NS_DIFFUSION
#include "diffagent.h"
#endif // NS_DIFFUSION

#ifdef UDP
#include "drivers/UDPlocal.hh"
#endif // UDP

#define WAIT_FOREVER       -1
#define POLLING_INTERVAL   10 // seconds
#define SMALL_TIMEOUT      10 // milliseconds

typedef long handle;

class HandleEntry;
class CallbackEntry;
class DiffusionRouting;

typedef list<HandleEntry *> HandleList;
typedef list<CallbackEntry *> CallbackList;

class TimerCallbacks {
public:
  virtual int expire(handle hdl, void *p) = 0;
  virtual void del(void *p) = 0;
};

class InterestCallback : public TimerCallback {
public:
  InterestCallback(DiffusionRouting *drt, HandleEntry *handle_entry) :
    drt_(drt), handle_entry_(handle_entry) {};
  ~InterestCallback() {};
  int expire();

  DiffusionRouting *drt_;
  HandleEntry *handle_entry_;
};

class FilterKeepaliveCallback : public TimerCallback {
public:
  FilterKeepaliveCallback(DiffusionRouting *drt, FilterEntry *filter_entry) :
    drt_(drt), filter_entry_(filter_entry) {};
  ~FilterKeepaliveCallback() {};
  int expire();

  DiffusionRouting *drt_;
  FilterEntry *filter_entry_;
};

class OldAPITimer : public TimerCallback {
public:
  OldAPITimer(TimerCallbacks *cb, void *p) :
    cb_(cb), p_(p) {};
  ~OldAPITimer() {};
  int expire();

  TimerCallbacks *cb_;
  void *p_;
};

class DiffusionRouting : public NR {
public:

#ifdef NS_DIFFUSION
  DiffusionRouting(u_int16_t port, DiffAppAgent *da);
  int getAgentId(int id = -1);
  MobileNode *getNode(MobileNode *mn = 0)
  {
    if (mn != 0)
      node_ = mn;
    return node_;
  };
#else
  DiffusionRouting(u_int16_t port);
  void run(bool wait_condition, long max_timeout);
#endif // NS_DIFFUSION

  virtual ~DiffusionRouting();

  // NR Publish/Subscribe API functions

  handle subscribe(NRAttrVec *subscribe_attrs, NR::Callback *cb);

  int unsubscribe(handle subscription_handle);

  handle publish(NRAttrVec *publish_attrs);

  int unpublish(handle publication_handle);

  int send(handle publication_handle, NRAttrVec *send_attrs);

  // NR Filter API functions

  handle addFilter(NRAttrVec *filter_attrs, u_int16_t priority,
		   FilterCallback *cb);

  int removeFilter(handle filter_handle);

  int sendMessage(Message *msg, handle h, u_int16_t priority = FILTER_KEEP_PRIORITY);

  // NR Timer API functions
  handle addTimer(int timeout, TimerCallback *callback);

  // This is an old API function that will be discontinued in
  // diffusion's next major release
  handle addTimer(int timeout, void *param, TimerCallbacks *cb);

  bool removeTimer(handle hdl);

  // NR API functions that allow single thread support

  void doIt();

  void doOne(long timeout = WAIT_FOREVER);

  int interestTimeout(HandleEntry *handle_entry);
  int filterKeepaliveTimeout(FilterEntry *filter_entry);

#ifndef NS_DIFFUSION
  // Outside NS, all these can be protected members
protected:
#endif // !NS_DIFFUSION
  void recvPacket(DiffPacket pkt);
  void recvMessage(Message *msg);
#ifdef NS_DIFFUSION
  // In NS, the protected members start here
protected:
  // Handle to MobileNode
  MobileNode *node_;
#endif // NS_DIFFUSION

  void sendMessageToDiffusion(Message *msg);
  void sendPacketToDiffusion(DiffPacket pkt, int len, int dst);

  void processMessage(Message *msg);
  void processControlMessage(Message *msg);

  bool checkSubscription(NRAttrVec *attrs);
  bool checkPublication(NRAttrVec *attrs);
  bool checkSend(NRAttrVec *attrs);
  bool isPushData(NRAttrVec *attrs);

  HandleEntry * removeHandle(handle my_handle, HandleList *hl);
  HandleEntry * findHandle(handle my_handle, HandleList *hl);

  FilterEntry * deleteFilter(handle my_handle);
  FilterEntry * findFilter(handle my_handle);
  bool hasScope(NRAttrVec *attrs);

  // Handle variables
  int next_handle_;
  HandleList pub_list_;
  HandleList sub_list_;
  FilterList filter_list_;

  // Threads and Mutexes
  pthread_mutex_t *dr_mtx_;

  // Data structures
  TimerManager *timers_manager_;

  // Lists
  DeviceList in_devices_;
  DeviceList local_out_devices_;

  // Node-specific variables
  u_int16_t diffusion_port_;
  int pkt_count_;
  int random_id_;

#ifdef NS_DIFFUSION
  int agent_id_;
#else
  u_int16_t agent_id_;
#endif // NS_DIFFUSION
};

#endif // !_DR_HH_
