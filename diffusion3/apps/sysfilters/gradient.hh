//
// gradient.hh    : Gradient Filter Include File
// author         : Fabio Silva and Chalermek Intanagonwiwat
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: gradient.hh,v 1.1 2002/05/06 22:54:08 haldar Exp $
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

#ifndef _GRADIENT_HH_
#define _GRADIENT_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "diffapp.hh"

#ifdef NS_DIFFUSION
#include <tcl.h>
#include "diffagent.h"
#else
#include "main/hashutils.hh"
#endif // NS_DIFFUSION

#define GRADIENT_FILTER_PRIORITY 4
#define OLD_MESSAGE -1
#define NEW_MESSAGE 1

extern NRSimpleAttributeFactory<void *> ReinforcementAttr;

typedef list<Tcl_HashEntry *> HashList;

class ReinforcementBlob {
public:
  ReinforcementBlob(int32_t rdm_id, int32_t pkt_num) :
    rdm_id_(rdm_id), pkt_num_(pkt_num) {};

  int32_t rdm_id_;
  int32_t pkt_num_;
};

class HashEntry {
public:
  HashEntry(int32_t last_hop) : last_hop_(last_hop) {};

  int32_t last_hop_;
};

class GradientEntry {
public:
  GradientEntry(int32_t node_addr) : node_addr_(node_addr)
  {
    GetTime(&tv_);
    reinforced_ = false;
  };

  int32_t node_addr_;
  struct timeval tv_;
  bool reinforced_;
};

class AgentEntry {
public:
  AgentEntry(u_int16_t port) : port_(port)
  {
    GetTime(&tv_);
  };

  u_int16_t port_;
  struct timeval tv_;
};

class DataNeighborEntry {
public:
  DataNeighborEntry(int32_t neighbor_id, int data_flag) :
    neighbor_id_(neighbor_id), data_flag_(data_flag)
  {
    GetTime(&tv_);
  };

  int32_t neighbor_id_;
  struct timeval tv_;
  int data_flag_;
};

class AttributeEntry {
public:
  AttributeEntry(NRAttrVec *attrs) : attrs_(attrs) {
    GetTime(&tv_);
  };

  ~AttributeEntry() {
    ClearAttrs(attrs_);
    delete attrs_;
  };

  struct timeval tv_;
  NRAttrVec *attrs_;
};

typedef list<AttributeEntry *> AttributeList;
typedef list<AgentEntry *> AgentList;
typedef list<GradientEntry *> GradientList;
typedef list<DataNeighborEntry *> DataNeighborList;

class RoutingEntry {
public:
  RoutingEntry() {
    GetTime(&tv_);
  };

  ~RoutingEntry() {
    DataNeighborList::iterator data_neighbor_itr;
    AttributeList::iterator attr_itr;
    GradientList::iterator grad_itr;
    AgentList::iterator agents_itr;

    // Clear Attributes
    ClearAttrs(attrs_);
    delete attrs_;

    // Clear the attribute list
    for (attr_itr = attr_list_.begin(); attr_itr != attr_list_.end(); attr_itr++){
      delete (*attr_itr);
    }
    attr_list_.clear();

    // Clear the gradient list
    for (grad_itr = gradients_.begin(); grad_itr != gradients_.end(); grad_itr++){
      delete (*grad_itr);
    }
    gradients_.clear();

    // Clear the local agent's list
    for (agents_itr = agents_.begin(); agents_itr != agents_.end(); agents_itr++){
      delete (*agents_itr);
    }
    agents_.clear();

    // Clear the data neighbor's list
    for (data_neighbor_itr = data_neighbors_.begin(); data_neighbor_itr != data_neighbors_.end(); data_neighbor_itr++){
      delete (*data_neighbor_itr);
    }
    data_neighbors_.clear();
  };

  struct timeval tv_;
  NRAttrVec *attrs_;
  AgentList agents_;
  GradientList gradients_;
  AttributeList attr_list_;
  DataNeighborList data_neighbors_;
};

typedef list<RoutingEntry *> RoutingTable;
class GradientFilter;

class GradientFilterReceive : public FilterCallback {
public:
  GradientFilterReceive(GradientFilter *app) : app_(app) {};
  void recv(Message *msg, handle h);

  GradientFilter *app_;
};

class GradientTimerReceive : public TimerCallbacks {
public:
  GradientTimerReceive(GradientFilter *app) : app_(app) {};
  int expire(handle hdl, void *p);
  void del(void *p);

  GradientFilter *app_;
};

class TimerType {
public:
  TimerType(int which_timer) : which_timer_(which_timer)
  {
    param_ = NULL;
  };

  int which_timer_;
  void *param_;
};

class GradientFilter : public DiffApp {
public:
#ifdef NS_DIFFUSION
  GradientFilter(const char *dr);
  int command(int argc, const char*const* argv);
  void run() {}
#else
  GradientFilter(int argc, char **argv);
  void run();
#endif // NS_DIFFUSION

  virtual ~GradientFilter()
  {
    // Nothing to do
  };

  void recv(Message *msg, handle h);
  int ProcessTimers(handle hdl, void *p);

protected:

  // General Variables
  handle filter_handle_;
  int pkt_count_;
  int random_id_;

  // Hashing structures
  HashList hash_list_;
  Tcl_HashTable htable_;

  // Receive Callback for the filter
  GradientFilterReceive *filter_callback_;
  GradientTimerReceive *timer_callback_;

  // List of all known datatypes
  RoutingTable routing_list_;

  // Setup the filter
  handle setupFilter();

  // Matching functions
  RoutingEntry * findRoutingEntry(NRAttrVec *attrs);
  void deleteRoutingEntry(RoutingEntry *routing_entry);
  RoutingEntry * matchRoutingEntry(NRAttrVec *attrs, RoutingTable::iterator start, RoutingTable::iterator *place);
  AttributeEntry * findMatchingSubscription(RoutingEntry *routing_entry, NRAttrVec *attrs);

  // Data structure management
  void updateGradient(RoutingEntry *routing_entry, int32_t last_hop, bool reinforced);
  void updateAgent(RoutingEntry *routing_entry, u_int16_t source_port);
  GradientEntry * findReinforcedGradients(GradientList *agents, GradientList::iterator start, GradientList::iterator *place);
  GradientEntry * findReinforcedGradient(int32_t node_addr, RoutingEntry *routing_entry);
  void deleteGradient(RoutingEntry *routing_entry, GradientEntry *gradient_entry);
  void setReinforcementFlags(RoutingEntry *routing_entry, int32_t last_hop, int new_message);

  // Message forwarding functions
  void sendInterest(NRAttrVec *attrs, RoutingEntry *routing_entry);
  void sendDisinterest(NRAttrVec *attrs, RoutingEntry *routing_entry);
  void sendPositiveReinforcement(NRAttrVec *reinf_attrs, int32_t data_rdm_id,
				 int32_t data_pkt_num, int32_t destination);
  void forwardData(Message *msg, RoutingEntry *routing_entry);
  void forwardPushExploratoryData(Message *msg);
  

  // Message Processing functions
  void processOldMessage(Message *msg);
  void processNewMessage(Message *msg);

  // Timers
  void messageTimeout(Message *msg);
  void interestTimeout(Message *msg);
  void gradientTimeout();
  void reinforcementTimeout();
  int subscriptionTimeout(NRAttrVec *attrs);

  // Hashing functions
  HashEntry * getHash(unsigned int pkt_num, unsigned int rdm_id);
  void putHash(HashEntry *new_hash_entry, unsigned int pkt_num, unsigned int rdm_id);
};

#endif // !_GRADIENT_HH_
