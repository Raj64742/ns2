// 
// stats.cc      : Collect various statistics for Diffusion
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: stats.cc,v 1.4 2002/05/29 21:58:13 haldar Exp $
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
  num_bytes_recv_ = 0;
  num_bytes_sent_ = 0;
  num_packets_recv_ = 0;
  num_packets_sent_ = 0;
  num_bcast_bytes_recv_ = 0;
  num_bcast_bytes_sent_ = 0;
  num_bcast_packets_recv_ = 0;
  num_bcast_packets_sent_ = 0;

  num_new_messages_sent_ = 0;
  num_new_messages_recv_ = 0;
  num_old_messages_sent_ = 0;
  num_old_messages_recv_ = 0;

  num_interest_messages_sent_ = 0;
  num_interest_messages_recv_ = 0;
  num_data_messages_sent_ = 0;
  num_data_messages_recv_ = 0;
  num_exploratory_data_messages_sent_ = 0;
  num_exploratory_data_messages_recv_ = 0;
  num_pos_reinforcement_messages_sent_ = 0;
  num_pos_reinforcement_messages_recv_ = 0;
  num_neg_reinforcement_messages_sent_ = 0;
  num_neg_reinforcement_messages_recv_ = 0;

  // Initialize id and time
  node_id_ = id;
  GetTime(&start_);
}

void DiffusionStats::logIncomingMessage(Message *msg)
{
  NeighborStatsEntry *neighbor;

  // We don't consider messages from local apps/filters
  if (msg->last_hop_ == LOCALHOST_ADDR)
    return;

  num_bytes_recv_ += (msg->data_len_ + sizeof(struct hdr_diff));
  num_packets_recv_++;

  if (msg->next_hop_ == BROADCAST_ADDR){
    num_bcast_packets_recv_++;
    num_bcast_bytes_recv_ += (msg->data_len_ + sizeof(struct hdr_diff));
  }

  if (msg->new_message_)
    num_new_messages_recv_++;
  else
    num_old_messages_recv_++;

  neighbor = getNeighbor(msg->last_hop_);
  neighbor->recv_messages_++;
  if (msg->next_hop_ == BROADCAST_ADDR)
    neighbor->recv_bcast_messages_++;

  switch (msg->msg_type_){

  case INTEREST:

    num_interest_messages_recv_++;
    
    break;

  case DATA:

    num_data_messages_recv_++;

    break;

  case EXPLORATORY_DATA:

    num_exploratory_data_messages_recv_++;

    break;

  case POSITIVE_REINFORCEMENT:

    num_pos_reinforcement_messages_recv_++;

    break;

  case NEGATIVE_REINFORCEMENT:

    num_neg_reinforcement_messages_recv_++;

    break;

  default:

    break;
  }
}

void DiffusionStats::logOutgoingMessage(Message *msg)
{
  NeighborStatsEntry *neighbor;

  num_bytes_sent_ += (msg->data_len_ + sizeof(struct hdr_diff));
  num_packets_sent_++;

  if (msg->next_hop_ == BROADCAST_ADDR){
    num_bcast_packets_sent_++;
    num_bcast_bytes_sent_ += (msg->data_len_ + sizeof(struct hdr_diff));
  }

  if (msg->new_message_)
    num_new_messages_sent_++;
  else
    num_old_messages_sent_++;

  if (msg->next_hop_ != BROADCAST_ADDR){
    neighbor = getNeighbor(msg->next_hop_);
    neighbor->sent_messages_++;
  }

  switch (msg->msg_type_){

  case INTEREST:

    num_interest_messages_sent_++;
    
    break;

  case DATA:

    num_data_messages_sent_++;

    break;

  case EXPLORATORY_DATA:

    num_exploratory_data_messages_sent_++;

    break;

  case POSITIVE_REINFORCEMENT:

    num_pos_reinforcement_messages_sent_++;

    break;

  case NEGATIVE_REINFORCEMENT:

    num_neg_reinforcement_messages_sent_++;

    break;

  default:

    break;
  }
}

void DiffusionStats::printStats(FILE *output)
{
  NeighborStatsList::iterator itr;
  NeighborStatsEntry *neighbor;
  long seconds;
  long useconds;
  float total_time;

  // Compute elapsed running time
  GetTime(&finish_);

  seconds = finish_.tv_sec - start_.tv_sec;
  if (finish_.tv_usec < start_.tv_usec){
    seconds--;
    finish_.tv_usec += 1000000;
  }

  useconds = finish_.tv_usec - start_.tv_usec;

  total_time = (float) (1.0 * seconds) + ((float) useconds / 1000000.0);

  fprintf(output, "Diffusion Stats\n");
  fprintf(output, "---------------\n\n");
  fprintf(output, "Node id : %d\n", node_id_);
  fprintf(output, "Running time : %f seconds\n\n", total_time);

  fprintf(output, "Total bytes/packets sent    : %d/%d\n",
	  num_bytes_sent_, num_packets_sent_);
  fprintf(output, "Total bytes/packets recv    : %d/%d\n",
	  num_bytes_recv_, num_packets_recv_);
  fprintf(output, "Total BC bytes/packets sent : %d/%d\n",
	  num_bcast_bytes_sent_, num_bcast_packets_sent_);
  fprintf(output, "Total BC bytes/packets recv : %d/%d\n",
	  num_bcast_bytes_recv_, num_bcast_packets_recv_);

  fprintf(output, "\n");

  fprintf(output, "Messages\n");
  fprintf(output, "--------\n");
  fprintf(output, "Old               - Sent : %d - Recv : %d\n",
	  num_old_messages_sent_, num_old_messages_recv_);
  fprintf(output, "New               - Sent : %d - Recv : %d\n",
	  num_new_messages_sent_, num_new_messages_recv_);

  fprintf(output, "\n");

  fprintf(output, "Interest          - Sent : %d - Recv : %d\n",
	  num_interest_messages_sent_, num_interest_messages_recv_);
  fprintf(output, "Data              - Sent : %d - Recv : %d\n",
	  num_data_messages_sent_, num_data_messages_recv_);
  fprintf(output, "Exploratory Data  - Sent : %d - Recv : %d\n",
	  num_exploratory_data_messages_sent_, num_exploratory_data_messages_recv_);
  fprintf(output, "Pos Reinforcement - Sent : %d - Recv : %d\n",
	  num_pos_reinforcement_messages_sent_,
	  num_pos_reinforcement_messages_recv_);
  fprintf(output, "Neg Reinforcement - Sent : %d - Recv : %d\n",
	  num_neg_reinforcement_messages_sent_,
	  num_neg_reinforcement_messages_recv_);

  fprintf(output, "\n");

  fprintf(output, "Neighbors Stats (in packets)\n");
  fprintf(output, "----------------------------\n");

  for (itr = nodes_.begin(); itr != nodes_.end(); ++itr){
    neighbor = *itr;
    if (neighbor){
      fprintf(output, "Node : %d - Sent(U/B/T) : %d/%d/%d - Recv(U/B/T) : %d/%d/%d\n",
	      neighbor->id_, neighbor->sent_messages_, num_bcast_packets_sent_,
	      (neighbor->sent_messages_ + num_bcast_packets_sent_),
	      (neighbor->recv_messages_ - neighbor->recv_bcast_messages_),
	      neighbor->recv_bcast_messages_,
	      neighbor->recv_messages_);
    }
  }

  fprintf(output, "\n");
  fprintf(output, "Key: U - Unicast\n");
  fprintf(output, "     B - Broadcast\n");
  fprintf(output, "     T - Total\n");

  fprintf(output, "\n");
}

NeighborStatsEntry * DiffusionStats::getNeighbor(int id)
{
  NeighborStatsList::iterator itr;
  NeighborStatsEntry *new_neighbor;

  for (itr = nodes_.begin(); itr != nodes_.end(); ++itr){
    if ((*itr)->id_ == id)
      break;
  }

  if (itr == nodes_.end()){
    // New neighbor
    new_neighbor = new NeighborStatsEntry(id);
    nodes_.push_front(new_neighbor);
    return new_neighbor;
  }

  return (*itr);
}
