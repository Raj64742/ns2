//
// dr.hh           : Diffusion Routing Class Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: dr.hh,v 1.5 2002/02/25 20:23:53 haldar Exp $
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

#ifndef DR_HH
#define DR_HH

#include <pthread.h>
#include <string.h>

#include "events.hh"
#include "filter.hh"
#include "config.hh"
#include "agent.hh"
#include "iodev.hh"
#include "tools.hh"

#ifdef NS_DIFFUSION
#include "diffagent.h"
#endif // NS_DIFFUSION

#ifdef UDP
#include "UDPlocal.hh"
#endif // UDP

#define WAIT_FOREVER       -1
#define POLLING_INTERVAL   10 // seconds

class Handle_Entry;
class Callback_Entry;

typedef long handle;
typedef list<Handle_Entry *> HandleList;
typedef list<Callback_Entry *> CallbackList;
typedef list<DiffusionIO *> DeviceList;

class TimerCallbacks {
public:
  virtual int recv(handle hdl, void *p) = 0;
  virtual void del(void *p) = 0;
};

class Timer_Entry {
public:
  handle         hdl;
  int            timeout;
  void           *p;
  TimerCallbacks *cb;

  Timer_Entry(handle _hdl, int _timeout, void *_p, TimerCallbacks *_cb) : 
    hdl(_hdl), timeout(_timeout), p(_p), cb(_cb) {};
};

class Handle_Entry {
public:
  handle hdl;
  bool valid;
  NRAttrVec *attrs;
  NR::Callback *cb;

  Handle_Entry()
  {
    valid = true;
    cb = NULL;
  };

  ~Handle_Entry(){

    ClearAttrs(attrs);
    delete attrs;
  };
};

class Callback_Entry {
public:
  NR::Callback *cb;
  NR::handle subscription_handle;
};

class DiffusionRouting : public NR {
public:

#ifdef NS_DIFFUSION
  DiffusionRouting(u_int16_t port, DiffAppAgent *da);
  int get_agentid(int id = -1);
#else
  DiffusionRouting(u_int16_t port);
  void run(bool wait_condition, long max_timeout);
#endif // NS_DIFFUSION

  ~DiffusionRouting()
  {
    HandleList::iterator itr;
    Handle_Entry *current;

    // Delete all Handles
    for (itr = sub_list.begin(); itr != sub_list.end(); ++itr){
      current = *itr;
      delete current;
    }

    for (itr = pub_list.begin(); itr != pub_list.end(); ++itr){
      current = *itr;
      delete current;
    }
  };

  // NR (publish-subscribe) API functions

  handle subscribe(NRAttrVec *subscribeAttrs, NR::Callback *cb);

  int unsubscribe(handle subscription_handle);

  handle publish(NRAttrVec *publishAttrs);

  int unpublish(handle publication_handle);

  int send(handle publication_handle, NRAttrVec *sendAttrs);

  // NR Filter API functions

  handle addFilter(NRAttrVec *filterAttrs, u_int16_t priority,
		   FilterCallback *cb);

  int removeFilter(handle filterHandle);

  int sendMessage(Message *msg, handle h, u_int16_t priority = FILTER_KEEP_PRIORITY);

  // NR Timer API functions

  handle addTimer(int timeout, void *param, TimerCallbacks *cb);

  int removeTimer(handle hdl);

  void doIt();

  void doOne(long timeout = WAIT_FOREVER);

#ifndef NS_DIFFUSION
  // Outside NS, all this can be protected members
protected:
#endif // NS_DIFFUSION

  void recvPacket(DiffPacket pkt);
  void recvMessage(Message *msg);
  void InterestTimeout(Handle_Entry *entry);
  void FilterKeepaliveTimeout(Filter_Entry *entry);
  void ApplicationTimeout(Timer_Entry *entry);

#ifdef NS_DIFFUSION
  // In NS, the protected members start here
protected:
#endif // NS_DIFFUSION

  void sendMessageToDiffusion(Message *msg);
  void sendPacketToDiffusion(DiffPacket pkt, int len, int dst);

  void processMessage(Message *msg);
  void processControlMessage(Message *msg);

  bool checkSubscription(NRAttrVec *attrs);
  bool checkPublication(NRAttrVec *attrs);
  bool checkSend(NRAttrVec *attrs);

  Handle_Entry * removeHandle(handle my_handle, HandleList *hl);
  Handle_Entry * findHandle(handle my_handle, HandleList *hl);

  Filter_Entry * deleteFilter(handle my_handle);
  Filter_Entry * findFilter(handle my_handle);
  bool hasScope(NRAttrVec *attrs);

  // Handle variables
  int next_handle;
  HandleList pub_list;
  HandleList sub_list;
  FilterList filter_list;

  // Threads and Mutexes
  pthread_mutex_t *drMtx;
  pthread_mutex_t *queueMtx;

  // Data structures
  eventQueue *eq;

  // Lists
  DeviceList in_devices;
  DeviceList local_out_devices;

  // Node-specific variables
  u_int16_t diffusion_port;
  int pkt_count;
  int rdm_id;

#ifdef NS_DIFFUSION
  int agent_id;
#else
  u_int16_t agent_id;
#endif // NS_DIFFUSION
};

#endif // DR_HH
