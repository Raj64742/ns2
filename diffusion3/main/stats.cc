// 
// stats.cc      : Collect various statistics for Diffusion
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: stats.cc,v 1.5 2002/03/21 19:30:55 haldar Exp $
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

#include "stats.hh"

DiffusionStats::DiffusionStats(int id)
{
  // Zero various counters
  num_bytes_recv = 0;
  num_bytes_sent = 0;
  num_packets_recv = 0;
  num_packets_sent = 0;
  num_bcast_bytes_recv = 0;
  num_bcast_bytes_sent = 0;
  num_bcast_packets_recv = 0;
  num_bcast_packets_sent = 0;

  num_new_messages_sent = 0;
  num_new_messages_recv = 0;
  num_old_messages_sent = 0;
  num_old_messages_recv = 0;

  num_interest_messages_sent = 0;
  num_interest_messages_recv = 0;
  num_data_messages_sent = 0;
  num_data_messages_recv = 0;
  num_exploratory_data_messages_sent = 0;
  num_exploratory_data_messages_recv = 0;
  num_pos_reinforcement_messages_sent = 0;
  num_pos_reinforcement_messages_recv = 0;
  num_neg_reinforcement_messages_sent = 0;
  num_neg_reinforcement_messages_recv = 0;

  // Initialize id and time
  node_id = id;
  getTime(&start);
}

void DiffusionStats::logIncomingMessage(Message *msg)
{
  Neighbor_Stats_Entry *neighbor;

  // We don't consider messages from local apps/filters
  if (msg->last_hop == (int)LOCALHOST_ADDR)
    return;

  num_bytes_recv += (msg->data_len + sizeof(struct hdr_diff));
  num_packets_recv++;

  if (msg->next_hop == (int)BROADCAST_ADDR){
    num_bcast_packets_recv++;
    num_bcast_bytes_recv += (msg->data_len + sizeof(struct hdr_diff));
  }

  if (msg->new_message)
    num_new_messages_recv++;
  else
    num_old_messages_recv++;

  neighbor = getNeighbor(msg->last_hop);
  neighbor->recv_messages++;
  if (msg->next_hop == (int)BROADCAST_ADDR)
    neighbor->recv_bcast_messages++;

  switch (msg->msg_type){

  case INTEREST:

    num_interest_messages_recv++;
    
    break;

  case DATA:

    num_data_messages_recv++;

    break;

  case EXPLORATORY_DATA:

    num_exploratory_data_messages_recv++;

    break;

  case POSITIVE_REINFORCEMENT:

    num_pos_reinforcement_messages_recv++;

    break;

  case NEGATIVE_REINFORCEMENT:

    num_neg_reinforcement_messages_recv++;

    break;

  default:

    break;
  }
}

void DiffusionStats::logOutgoingMessage(Message *msg)
{
  Neighbor_Stats_Entry *neighbor;

  num_bytes_sent += (msg->data_len + sizeof(struct hdr_diff));
  num_packets_sent++;

  if (msg->next_hop == (int)BROADCAST_ADDR){
    num_bcast_packets_sent++;
    num_bcast_bytes_sent += (msg->data_len + sizeof(struct hdr_diff));
  }

  if (msg->new_message)
    num_new_messages_sent++;
  else
    num_old_messages_sent++;

  if (msg->next_hop != (int)BROADCAST_ADDR){
    neighbor = getNeighbor(msg->next_hop);
    neighbor->sent_messages++;
  }

  switch (msg->msg_type){

  case INTEREST:

    num_interest_messages_sent++;
    
    break;

  case DATA:

    num_data_messages_sent++;

    break;

  case EXPLORATORY_DATA:

    num_exploratory_data_messages_sent++;

    break;

  case POSITIVE_REINFORCEMENT:

    num_pos_reinforcement_messages_sent++;

    break;

  case NEGATIVE_REINFORCEMENT:

    num_neg_reinforcement_messages_sent++;

    break;

  default:

    break;
  }
}

void DiffusionStats::printStats(FILE *output)
{
  NeighborStatsList::iterator itr;
  Neighbor_Stats_Entry *neighbor;
  long seconds;
  long useconds;
  float total_time;

  // Compute elapsed running time
  getTime(&finish);

  seconds = finish.tv_sec - start.tv_sec;
  if (finish.tv_usec < start.tv_usec){
    seconds--;
    finish.tv_usec += 1000000;
  }

  useconds = finish.tv_usec - start.tv_usec;

  total_time = (float) (1.0 * seconds) + ((float) useconds / 1000000.0);

  fprintf(output, "Diffusion Stats\n");
  fprintf(output, "---------------\n\n");
  fprintf(output, "Node id : %d\n", node_id);
  fprintf(output, "Running time : %f seconds\n\n", total_time);

  fprintf(output, "Total bytes/packets sent    : %d/%d\n",
	  num_bytes_sent, num_packets_sent);
  fprintf(output, "Total bytes/packets recv    : %d/%d\n",
	  num_bytes_recv, num_packets_recv);
  fprintf(output, "Total BC bytes/packets sent : %d/%d\n",
	  num_bcast_bytes_sent, num_bcast_packets_sent);
  fprintf(output, "Total BC bytes/packets recv : %d/%d\n",
	  num_bcast_bytes_recv, num_bcast_packets_recv);

  fprintf(output, "\n");

  fprintf(output, "Messages\n");
  fprintf(output, "--------\n");
  fprintf(output, "Old               - Sent : %d - Recv : %d\n",
	  num_old_messages_sent, num_old_messages_recv);
  fprintf(output, "New               - Sent : %d - Recv : %d\n",
	  num_new_messages_sent, num_new_messages_recv);

  fprintf(output, "\n");

  fprintf(output, "Interest          - Sent : %d - Recv : %d\n",
	  num_interest_messages_sent, num_interest_messages_recv);
  fprintf(output, "Data              - Sent : %d - Recv : %d\n",
	  num_data_messages_sent, num_data_messages_recv);
  fprintf(output, "Exploratory Data  - Sent : %d - Recv : %d\n",
	  num_exploratory_data_messages_sent, num_exploratory_data_messages_recv);
  fprintf(output, "Pos Reinforcement - Sent : %d - Recv : %d\n",
	  num_pos_reinforcement_messages_sent,
	  num_pos_reinforcement_messages_recv);
  fprintf(output, "Neg Reinforcement - Sent : %d - Recv : %d\n",
	  num_neg_reinforcement_messages_sent,
	  num_neg_reinforcement_messages_recv);

  fprintf(output, "\n");

  fprintf(output, "Neighbors Stats (in packets)\n");
  fprintf(output, "----------------------------\n");

  for (itr = nodes.begin(); itr != nodes.end(); ++itr){
    neighbor = *itr;
    if (neighbor){
      fprintf(output, "Node : %d - Sent(U/B/T) : %d/%d/%d - Recv(U/B/T) : %d/%d/%d\n",
	      neighbor->id, neighbor->sent_messages, num_bcast_packets_sent,
	      (neighbor->sent_messages + num_bcast_packets_sent),
	      (neighbor->recv_messages - neighbor->recv_bcast_messages),
	      neighbor->recv_bcast_messages,
	      neighbor->recv_messages);
    }
  }

  fprintf(output, "\n");
  fprintf(output, "Key: U - Unicast\n");
  fprintf(output, "     B - Broadcast\n");
  fprintf(output, "     T - Total\n");

  fprintf(output, "\n");
}

Neighbor_Stats_Entry * DiffusionStats::getNeighbor(int id)
{
  NeighborStatsList::iterator itr;
  Neighbor_Stats_Entry *new_neighbor;

  for (itr = nodes.begin(); itr != nodes.end(); ++itr){
    if ((*itr)->id == id)
      break;
  }

  if (itr == nodes.end()){
    // New neighbor
    new_neighbor = new Neighbor_Stats_Entry(id);
    nodes.push_front(new_neighbor);
    return new_neighbor;
  }

  return (*itr);
}
