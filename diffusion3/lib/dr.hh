//
// dr.hh           : Diffusion Routing Class Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: dr.hh,v 1.10 2002/05/29 21:58:12 haldar Exp $
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

#include "main/events.hh"
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

typedef long handle;

class HandleEntry;
class TimerEntry;
class CallbackEntry;

typedef list<HandleEntry *> HandleList;
typedef list<CallbackEntry *> CallbackList;
typedef list<DiffusionIO *> DeviceList;

class TimerCallbacks {
public:
  virtual int expire(handle hdl, void *p) = 0;
  virtual void del(void *p) = 0;
};

class DiffusionRouting : public NR {
public:

#ifdef NS_DIFFUSION
  DiffusionRouting(u_int16_t port, DiffAppAgent *da);
  int getAgentId(int id = -1);
#else
  DiffusionRouting(u_int16_t port);
  void run(bool wait_condition, long max_timeout);
#endif // NS_DIFFUSION

  virtual ~DiffusionRouting();

  // NR Publish/Subscribe API functions

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

  // NR API functions that allow single thread support

  void doIt();

  void doOne(long timeout = WAIT_FOREVER);

#ifndef NS_DIFFUSION
  // Outside NS, all these can be protected members
protected:
#endif // !NS_DIFFUSION

  void recvPacket(DiffPacket pkt);
  void recvMessage(Message *msg);
  void interestTimeout(HandleEntry *entry);
  void filterKeepaliveTimeout(FilterEntry *entry);
  void applicationTimeout(TimerEntry *entry);

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
  pthread_mutex_t *queue_mtx_;

  // Data structures
  EventQueue *eq_;

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
