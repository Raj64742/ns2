// 
// stats.hh      : Collect various statistics for Diffusion
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: stats.hh,v 1.4 2002/03/20 22:49:41 haldar Exp $
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

#ifndef stats_hh
#define stats_hh

#include <list>

#include "message.hh"
#include "tools.hh"

using namespace std;

class Neighbor_Stats_Entry {
public:
  int id;
  int recv_messages;
  int recv_bcast_messages;
  int sent_messages;

  Neighbor_Stats_Entry(int _id) : id(_id)
  {
    recv_messages = 0;
    recv_bcast_messages = 0;
    sent_messages = 0;
  }
};

typedef list<Neighbor_Stats_Entry *> NeighborStatsList;

class DiffusionStats {
public:
  DiffusionStats(int id);
  void logIncomingMessage(Message *msg);
  void logOutgoingMessage(Message *msg);
  void printStats(FILE *output);

private:
  int node_id;
  struct timeval start;
  struct timeval finish;
  NeighborStatsList nodes;

  // Counters
  int num_bytes_recv;
  int num_bytes_sent;
  int num_packets_recv;
  int num_packets_sent;
  int num_bcast_bytes_recv;
  int num_bcast_bytes_sent;
  int num_bcast_packets_recv;
  int num_bcast_packets_sent;

  // Message counter
  int num_interest_messages_sent;
  int num_interest_messages_recv;
  int num_data_messages_sent;
  int num_data_messages_recv;
  int num_exploratory_data_messages_sent;
  int num_exploratory_data_messages_recv;
  int num_pos_reinforcement_messages_sent;
  int num_pos_reinforcement_messages_recv;
  int num_neg_reinforcement_messages_sent;
  int num_neg_reinforcement_messages_recv;

  int num_new_messages_sent;
  int num_new_messages_recv;
  int num_old_messages_sent;
  int num_old_messages_recv;

  Neighbor_Stats_Entry * getNeighbor(int id);
};





#endif
