// 
// message.hh    : Message definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: message.hh,v 1.4 2002/03/20 22:49:41 haldar Exp $
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

#ifndef message_hh
#define message_hh

#include "nr.hh"
#include "header.hh"
#include "attrs.hh"

class Message {
public:
  // Read directly from the packet header
  int8_t  version;
  int8_t  msg_type;
  u_int16_t source_port;
  int16_t data_len;
  int16_t num_attr;
  int32_t pkt_num;
  int32_t rdm_id;
  int32_t next_hop;
  int32_t last_hop;

  // Added variables
  int new_message;
  u_int16_t next_port;

  // Message attributes
  NRAttrVec *msg_attr_vec;

  Message(int8_t _version, int8_t _msg_type, u_int16_t _source_port,
	  u_int16_t _data_len, u_int16_t _num_attr, int32_t _pkt_num,
	  int32_t _rdm_id, int32_t _next_hop, int32_t _last_hop) :
    version(_version),
    msg_type(_msg_type),
    source_port(_source_port),
    data_len(_data_len),
    num_attr(_num_attr),
    pkt_num(_pkt_num),
    rdm_id(_rdm_id),
    next_hop(_next_hop),
    last_hop(_last_hop)
  {
    msg_attr_vec = NULL;
    next_port = 0;
    new_message = 1;             // New message by default, will be changed
                                 // later if message is found to be old
  }

  ~Message()
  {
    if (msg_attr_vec){
      ClearAttrs(msg_attr_vec);
      delete msg_attr_vec;
    }
  }
};

extern NRSimpleAttributeFactory<void *> ControlMsgAttr;
extern NRSimpleAttributeFactory<void *> OriginalHdrAttr;
extern NRSimpleAttributeFactory<int> NRScopeAttr;
extern NRSimpleAttributeFactory<int> NRClassAttr;
extern NRSimpleAttributeFactory<int> NRNewSubsAttr;
extern NRSimpleAttributeFactory<void *> ReinforcementAttr;

#define CONTROL_MESSAGE_KEY  2400
#define REINFORCEMENT_KEY    2401
#define ORIGINAL_HEADER_KEY  2402

// Control Message types
typedef enum ctl_t_ {
  ADD_FILTER,
  REMOVE_FILTER,
  KEEPALIVE_FILTER,
  SEND_MESSAGE
} ctl_t;

class ControlMessage {
public:
  int32_t command;
  int32_t param1;
  int32_t param2;
};

class RedirectMessage {
public:
  int8_t  new_message;
  int8_t  msg_type;
  u_int16_t source_port;
  u_int16_t data_len;
  u_int16_t num_attr;
  int32_t rdm_id;
  int32_t pkt_num;
  int32_t next_hop;
  int32_t last_hop;
  int32_t handle;
  u_int16_t next_port;
};

Message * CopyMessage(Message *msg);

#endif // message_hh
