// 
// filter_core.hh  : Diffusion definitions
// authors         : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: filter_core.hh,v 1.1 2003/07/08 17:55:57 haldar Exp $
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

#ifndef _FILTER_CORE_HH_
#define _FILTER_CORE_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "main/timers.hh"
#include "main/message.hh"
#include "main/filter.hh"
#include "main/config.hh"
#include "main/tools.hh"
#include "main/iodev.hh"

#ifdef IO_LOG
#include "iolog.hh"
#endif // IO_LOG

#ifdef NS_DIFFUSION
#include <tcl.h>
#include "diffrtg.h"
#else
#include "main/hashutils.hh"
#endif // NS_DIFFUSION

#ifdef UDP
#include "drivers/UDPlocal.hh"
#ifdef WIRED
#include "drivers/UDPwired.hh"
#endif // WIRED
#endif // UDP

#ifdef USE_RPC
#include "drivers/RPCio.hh"
#endif // USE_RPC

#ifdef USE_WINSNG2
#include "drivers/WINSng2.hh"
#endif // USE_WINSNG2

#ifdef USE_MOTE_NIC
#include "drivers/MOTEio.hh"
#endif // USE_MOTE_NIC

#ifdef USE_SMAC
#include "drivers/SMAC.hh"
#endif // USE_SMAC

#ifdef USE_EMSTAR
#include "drivers/emstar.hh"
#endif // USE_EMSTAR

#ifdef STATS
#include "iostats.hh"
#define STATS_ARGS "si:"
#else
#define STATS_ARGS ""
#endif // STATS

#ifdef IO_LOG
#define IO_LOG_ARGS "l"
#else
#define IO_LOG_ARGS ""
#endif // IO_LOG

#define COMMAND_LINE_ARGS "f:hd:vt:p:" IO_LOG_ARGS STATS_ARGS

class DiffusionCoreAgent;
class HashEntry;
class NeighborEntry;

typedef list<NeighborEntry *> NeighborList;
typedef list<Tcl_HashEntry *> HashList;
typedef list<int32_t> BlackList;

class DiffusionCoreAgent {
public:
#ifdef NS_DIFFUSION
  friend class DiffRoutingAgent;
  DiffusionCoreAgent(DiffRoutingAgent *diffrtg, int nodeid);
#else
  DiffusionCoreAgent(int argc, char **argv);
  void usage(char *s);
  void run();
#endif // NS_DIFFUSION

  void timeToStop();
  // Timer functions
  void neighborsTimeout();
  void filterTimeout();

protected:
  float lon_;
  float lat_;

#ifdef STATS
  DiffusionStats *stats_;
#endif // STATS

  // Ids and counters
  int32_t my_id_;
  u_int16_t diffusion_port_;
  int pkt_count_;
  int random_id_;

  // Configuration options
  char *config_file_;

  // Lists
  DeviceList in_devices_;
  DeviceList out_devices_;
  DeviceList local_out_devices_;
  NeighborList neighbor_list_;
  FilterList filter_list_;
  BlackList black_list_;
  HashList hash_list_;

  // Data structures
  TimerManager *timers_manager_;
  //  EventQueue *eq_;
  Tcl_HashTable htable_;

  // Device-Independent send
  void sendMessageToLibrary(Message *msg, u_int16_t dst_agent_id);
  void sendPacketToLibrary(DiffPacket pkt, int len, u_int16_t dst);

  void sendMessageToNetwork(Message *msg);
  void sendPacketToNetwork(DiffPacket pkt, int len, int dst);

  // Receive functions
  void recvPacket(DiffPacket pkt);
  void recvMessage(Message *msg);

  // Hashing functions
  HashEntry * getHash(unsigned int, unsigned int);
  void putHash(unsigned int, unsigned int);

  // Neighbors functions
  void updateNeighbors(int id);

  // Message functions
  void processMessage(Message *msg);
  void processControlMessage(Message *msg);
  void logControlMessage(Message *msg, int command, int param1, int param2);
  bool restoreOriginalHeader(Message *msg);

  // Filter functions
  FilterList * getFilterList(NRAttrVec *attrs);
  FilterEntry * findFilter(int16_t handle, u_int16_t agent);
  FilterEntry * deleteFilter(int16_t handle, u_int16_t agent);
  bool addFilter(NRAttrVec *attrs, u_int16_t agent, int16_t handle,
		 u_int16_t priority);
  FilterList::iterator findMatchingFilter(NRAttrVec *attrs,
					  FilterList::iterator filter_itr);
  u_int16_t getNextFilterPriority(int16_t handle, u_int16_t priority,
				  u_int16_t agent);

  // Send messages to modules
  void forwardMessage(Message *msg, FilterEntry *filter_entry);
  void sendMessage(Message *msg);
};

class NeighborsTimeoutTimer : public TimerCallback {
public:
  NeighborsTimeoutTimer(DiffusionCoreAgent *agent) : agent_(agent) {};
  ~NeighborsTimeoutTimer() {};
  int expire();

  DiffusionCoreAgent *agent_;
};

class FilterTimeoutTimer : public TimerCallback {
public:
  FilterTimeoutTimer(DiffusionCoreAgent *agent) : agent_(agent) {};
  ~FilterTimeoutTimer() {};
  int expire();

  DiffusionCoreAgent *agent_;
};

class DiffusionStopTimer : public TimerCallback {
public:
  DiffusionStopTimer(DiffusionCoreAgent *agent) : agent_(agent) {};
  ~DiffusionStopTimer() {};
  int expire();

  DiffusionCoreAgent *agent_;
};

#endif // !_FILTER_CORE_HH_
