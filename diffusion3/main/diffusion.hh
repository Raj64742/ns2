// 
// diffusion.hh  : Diffusion definitions
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: diffusion.hh,v 1.2 2001/11/20 22:28:17 haldar Exp $
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

#ifdef NS_DIFFUSION

#ifndef diffusion_hh
#define diffusion_hh

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "events.hh"
#include "message.hh"
#include "filter.hh"
#include "config.hh"
#include "tools.hh"
#include "iodev.hh"


#ifdef NS_DIFFUSION
#include <tcl.h>
#else
#include "hashutils.hh"
#endif // NS 


#ifdef UDP
#include "UDPlocal.hh"
#ifdef WIRED
#include "UDPwired.hh"
#endif // WIRED
#endif // UDP

#ifdef USE_RPC
#include "RPCio.hh"
#endif // USE_RPC

#ifdef USE_WINSNG2
#include "WINSng2.hh"
#endif // USE_WINSNG2

#ifdef USE_MOTE_NIC
#include "MOTEio.hh"
#endif // USE_MOTE_NIC

#ifdef STATS
#include "stats.hh"
#endif

#ifndef WIRED
#    ifdef USE_RPC
#include "drivers/rpc_stats.hh"
#    endif
#endif

DiffPacket DupPacket(DiffPacket pkt);  // Allocate and copy pkt

#define SKT_INFO(x)       x->skt_info

class DiffusionCoreAgent;
class Neighbor_Entry;

typedef list<Neighbor_Entry *> NeighborList;
typedef list<Tcl_HashEntry *> HashList;
typedef list<DiffusionIO *> DeviceList;

class Hash_Entry {
public:
  bool dummy;

  Hash_Entry() { 
    dummy  = false;
  }
};

class Neighbor_Entry {
public:
  int32_t id;
  struct timeval tmv;

  Neighbor_Entry(int _id) : id(_id)
  {
    getTime(&tmv);
  }
};

#ifdef NS_DIFFUSION
class DiffRoutingAgent;

class LocalApp : public DiffusionIO {
public:
  LocalApp(DiffRoutingAgent *agent) { agent_ = agent;}
  DiffPacket RecvPacket(int fd);
  void SendPacket(DiffPacket pkt, int len, int dst); 
protected:
  DiffRoutingAgent *agent_;
};

class LinkLayerAbs : public DiffusionIO {
public:
  LinkLayerAbs(DiffRoutingAgent *agent) { agent_ = agent;}
  DiffPacket RecvPacket(int fd);
  void SendPacket(DiffPacket pkt, int len, int dst); 
protected:
  DiffRoutingAgent *agent_;
};
#endif // NS

class DiffusionCoreAgent {
public:
#ifdef NS_DIFFUSION
  friend class DiffRoutingAgent;
  friend class NeighborTimer;
  friend class FilterTimer;
  DiffusionCoreAgent(DiffRoutingAgent *dra);
#else
  DiffusionCoreAgent(int argc, char **argv);
  void DiffusionCoreAgent::usage();
  void run();
  void TimetoStop();
#endif
  
protected:
  float lon;
  float lat;
  
#ifdef STATS
  DiffusionStats *stats;
#  ifndef WIRED
#     ifdef USE_RPC
  RPCStats *rpcstats;
#     endif
#  endif
  
#endif

  // Ids and counters
  int32_t myId;
  u_int16_t diffusion_port;
  int pkt_count;
  int rdm_id;

  // Configuration options
  char *config_file;

  // Lists
  DeviceList in_devices;
  DeviceList out_devices;
  DeviceList local_out_devices;
  NeighborList my_neighbors;
  FilterList filter_list;
  HashList hash_list;

  // Data structures
  eventQueue *eq;
  Tcl_HashTable htable;

  // Methods start here

  // Device-Independent send
  void dvi_send(char *pkt, int length);

  // Receive and packet processing
  void recv(DiffPacket pkt);
  DiffPacket AllocateBuffer(NRAttrVec *attrs);

  // Hashing functions
  Hash_Entry *GetHash(unsigned int, unsigned int);
  void PutHash(unsigned int, unsigned int);

  // Neighbors functions
  void UpdateNeighbors(int id);

  // Message functions
  void ProcessMessage(Message *msg);
  void ProcessControlMessage(Message *msg);
  void LogControlMessage(Message *msg, int command, int param1, int param2);
  bool RestoreOriginalHeader(Message *msg);

  // Filter functions
  FilterList * GetFilterList(NRAttrVec *attrs);
  Filter_Entry * FindFilter(int16_t handle, u_int16_t agent);
  Filter_Entry * DeleteFilter(int16_t handle, u_int16_t agent);
  void AddFilter(NRAttrVec *attrs, u_int16_t agent, int16_t handle, u_int16_t priority);
  FilterList::iterator FindMatchingFilter(NRAttrVec *attrs, FilterList::iterator itr);

  // Send messages to modules
  void ForwardMessage(Message *msg, Filter_Entry *dst);
  void SendMessage(Message *msg, u_int16_t port);

  // Timer functions
  void NeighborsTimeOut();
  void FilterTimeOut();
  void EnergyTimeOut();
};


#endif

#endif // NS










