//
// iolog.cc      : IO Log Layer
// Authors       : Fabio Silva and Yutaka Mori
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: iolog.cc,v 1.1 2002/09/16 17:57:28 haldar Exp $
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

#include <stdio.h>
#include <stdlib.h>

#include "iolog.hh"

IOLog::IOLog(int32_t id) : IOHook()
{
  node_id_ = id;
}

DiffPacket IOLog::recvPacket(int fd)
{
  DiffPacket incoming_packet;
  struct hdr_diff *dfh;
  u_int16_t data_len, packet_len;
  int32_t last_hop;
  int msg_type;
  char *msg_name;

  // Receive the packet first
  incoming_packet = IOHook::recvPacket(fd);

  if (incoming_packet){
    // Log incoming packet
    dfh = HDR_DIFF(incoming_packet);
    last_hop = ntohl(LAST_HOP(dfh));
    data_len = ntohs(DATA_LEN(dfh));
    msg_type = MSG_TYPE(dfh);
    packet_len = data_len + sizeof(hdr_diff);

    switch (msg_type){
    case INTEREST:
      msg_name = strdup("Interest");
      break;
    case DATA:
      msg_name = strdup("Data");
      break;
    case EXPLORATORY_DATA:
      msg_name = strdup("Exploratory Data");
      break;
    case PUSH_EXPLORATORY_DATA:
      msg_name = strdup("Push Exploratory Data");
      break;
    case POSITIVE_REINFORCEMENT:
      msg_name = strdup("Positive Reinforcement");
      break;
    case NEGATIVE_REINFORCEMENT:
      msg_name = strdup("Negative Reinforcement");
      break;
    default:
      msg_name = strdup("Unknown");
      break;
    }

    if (last_hop != LOCALHOST_ADDR){
      fprintf(stdout,
	      "Diffusion Log: Node %d received %d bytes from node %d message %s\n",
	      node_id_, packet_len, last_hop, msg_name);
      fflush(NULL);
    }

    free(msg_name);
  }

  return incoming_packet;
}

void IOLog::sendPacket(DiffPacket pkt, int len, int dst)
{
  struct hdr_diff *dfh;
  int msg_type;
  char *msg_name;

  // Log outgoing packet
  dfh = HDR_DIFF(pkt);
  msg_type = MSG_TYPE(dfh);

  switch (msg_type){
  case INTEREST:
    msg_name = strdup("Interest");
    break;
  case DATA:
    msg_name = strdup("Data");
    break;
  case EXPLORATORY_DATA:
    msg_name = strdup("Exploratory Data");
    break;
  case PUSH_EXPLORATORY_DATA:
    msg_name = strdup("Push Exploratory Data");
    break;
  case POSITIVE_REINFORCEMENT:
    msg_name = strdup("Positive Reinforcement");
    break;
  case NEGATIVE_REINFORCEMENT:
    msg_name = strdup("Negative Reinforcement");
    break;
  default:
    msg_name = strdup("Unknown");
    break;
  }

  fprintf(stdout,
	  "Diffusion Log: Node %d sending %d bytes to node %d message %s\n",
	  node_id_, len, dst, msg_name);
  fflush(NULL);

  free(msg_name);

  // Send packet to device
  IOHook::sendPacket(pkt, len, dst);
}