//
// gradient.hh    : Gradient Filter Include File
// author         : Fabio Silva and Chalermek Intanagonwiwat
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: gradient.hh,v 1.2 2001/11/20 22:31:04 haldar Exp $
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
//
//

#ifdef NS_DIFFUSION

#ifndef GRADIENT_HH
#define GRADIENT_HH

#include <stdlib.h>
#include <stdio.h>
#include "dr.hh"

#ifdef NS_DIFFUSION
#include <tcl.h>
#else
#include "hashutils.hh"
#endif 

#include "diffapp.h"

#define GRADIENT_FILTER_PRIORITY 2
#define OLD_MESSAGE -1
#define NEW_MESSAGE 1

typedef list<Tcl_HashEntry *> HashList;

class Hash_Entry {
public:
  int32_t last_hop;

  Hash_Entry() {
    last_hop = 0;
  }
};

class Attribute_Entry;
class Routing_Entry;
class Agents_Entry;

typedef list<Attribute_Entry *> AttributeList;
typedef list<Routing_Entry *> RoutingList;
typedef list<Agents_Entry *> AgentsList;

class Agents_Entry : public Agent_Entry {
public:
  bool reinforced;
  int neighbor_flag;

  Agents_Entry() : Agent_Entry() {
    reinforced = false;
    neighbor_flag = 0;
  };
};

class GradientFilter;
class MyFilterReceive : public FilterCallback {
public:
  MyFilterReceive(GradientFilter *gf) { app_ = gf; }
  void recv(Message *msg, handle h);
protected:
  GradientFilter *app_;
};

class MyTimerReceive : public TimerCallbacks {
public:
  MyTimerReceive(GradientFilter *gf) { app_ = gf; }
  int recv(handle hdl, void *p);
  void del(void *p);
protected:
  GradientFilter *app_;  
};

class TimerType {
public:
  int   which_timer;
  void  *param;

  TimerType(int _which_timer) : which_timer(_which_timer)
  {
    param = NULL;
  };

  ~TimerType() {};
};

class ReinforcementMessage {
public:
  int32_t rdm_id;
  int32_t pkt_num;
};

class Attribute_Entry {
public:
  NRAttrVec       *attrs;
  struct timeval  tv;

  Attribute_Entry() {
    attrs        = NULL;
    getTime(&tv);
  };

  ~Attribute_Entry() {
    ClearAttrs(attrs);
    delete attrs;
  };
};

class Routing_Entry {
public:
  AgentsList      active;
  AgentsList      agents;
  NRAttrVec       *attrs;
  AttributeList   attr_list;
  struct timeval  tv;
  AgentsList      data_neighbors;
 
  Routing_Entry();
  ~Routing_Entry() {
    AttributeList::iterator ATitr;
    AgentsList::iterator AGitr;
  
    // Clear Attributes
    ClearAttrs(attrs);
    delete attrs;

    ATitr = attr_list.begin();

    while (ATitr != attr_list.end()){
      delete (*ATitr);
      ATitr++;
    }
    attr_list.clear();

    AGitr = active.begin();

    while (AGitr != active.end()){
      delete (*AGitr);
      AGitr++;
    }
    active.clear();

    AGitr = agents.begin();

    while (AGitr != agents.end()){
      delete (*AGitr);
      AGitr++;
    }
    agents.clear();
  };
};
#ifdef NS_DIFFUSION
 //class GradientFilter : public DiffApp {
class GradientFilter : public Application {
#else
class GradientFilter {
#endif // NS
public:
#ifdef NS_DIFFUSION
  GradientFilter(const char *dr);
  int command(int argc, const char*const* argv);
  void run() {}
  void start();
#else
  GradientFilter(int argc, char **argv);
  void usage();
  void run();
  void TimetoStop();
#endif //ns

  void recv(Message *msg, handle h);
  int ProcessTimers(handle hdl, void *p);

protected:

  NR *dr;

  // General Variables
  handle filterHandle;
  int pkt_count;
  int rdm_id;

  // Hashing structures
  HashList hash_list;
  Tcl_HashTable htable;

  // Receive Callback for the filter
  MyFilterReceive *fcb;
  MyTimerReceive *tcb;

  // List of all known datatypes
  RoutingList routing_list;

  // Setup the filter
  handle setupFilter();

  // Matching functions
  Routing_Entry * FindPerfectMatch(NRAttrVec *attrs);
  void DeleteRoutingEntry(Routing_Entry *entry);
  Routing_Entry * FindRoutingEntry(NRAttrVec *attrs, RoutingList::iterator start, RoutingList::iterator *place);
  Attribute_Entry * FindMatchingSubscription(Routing_Entry *entry, NRAttrVec *attrs);

  // Data structure management
  void UpdateGradient(Routing_Entry *entry, int32_t last_hop, bool reinforced);
  void UpdateAgent(Routing_Entry *entry, u_int16_t source_port);
  Agents_Entry * FindReinforcedGradients(AgentsList *agents, AgentsList::iterator start, AgentsList::iterator *place);
  Agents_Entry * FindReinforcedGradient(int32_t node_addr, Routing_Entry *entry);
  void DeleteGradient(Routing_Entry *entry, Agents_Entry *ae);
  void SetReinforcementFlags(Routing_Entry *entry, int32_t last_hop, int new_message);

  // Message forwarding functions
  void SendInterest(NRAttrVec *attrs, Routing_Entry *entry);
  void SendDisinterest(NRAttrVec *attrs, Routing_Entry *entry);
  void ForwardData(Message *msg, Routing_Entry *entry, bool &reinforced);

  // Message Processing functions
  void ProcessOldMessage(Message *msg);
  void ProcessNewMessage(Message *msg);

  // Timers
  void MessageTimeout(Message *msg);
  void InterestTimeout(Message *msg);
  void GradientTimeout();
  void ReinforcementTimeout();
  int SubscriptionTimeout(NRAttrVec *attrs);

  
  // Hashing functions
  Hash_Entry *GetHash(unsigned int, unsigned int);
  void PutHash(Hash_Entry *newHashPtr, unsigned int, unsigned int);
};

#endif //GRADIENT_HH
#endif // NS
