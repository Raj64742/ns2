// 
// message.hh    : Message definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: message.hh,v 1.2 2002/05/07 00:24:23 haldar Exp $
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

// This file defines the Message class, used inside a node to
// represent a diffusion message. It contains all elements from the
// packet header in addition to the packet's attributes. The Message
// class also contains other flags that are not part of diffusion
// packets. new_message_ indicates if this packet/message was seen in
// the past, indicating a loop (this is set by diffusion upon
// receiving the packet). next_port_ is used by the Filter API to
// indicate the packet's next destination.
//
// This file also defines two other attributes (ControlMsgAttr and
// OriginalHdrAttr) and two classes (ControlMessage and
// OriginalHeader). These are used when sending packets/messages from
// the diffusion core to the library API.
//
// The function CopyMessage can be used to copy a message. It returns
// a pointer to the newly created message. The original message is not
// changed.

#ifndef _MESSAGE_HH_
#define _MESSAGE_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "nr/nr.hh"
#include "header.hh"
#include "attrs.hh"

class Message {
public:
  // Read directly from the packet header
  int8_t  version_;
  int8_t  msg_type_;
  u_int16_t source_port_;
  int16_t data_len_;
  int16_t num_attr_;
  int32_t pkt_num_;
  int32_t rdm_id_;
  int32_t next_hop_;
  int32_t last_hop_;

  // Added variables
  int new_message_;
  u_int16_t next_port_;

  // Message attributes
  NRAttrVec *msg_attr_vec_;

  Message(int8_t version, int8_t msg_type, u_int16_t source_port,
	  u_int16_t data_len, u_int16_t num_attr, int32_t pkt_num,
	  int32_t rdm_id, int32_t next_hop, int32_t last_hop) :
    version_(version),
    msg_type_(msg_type),
    source_port_(source_port),
    data_len_(data_len),
    num_attr_(num_attr),
    pkt_num_(pkt_num),
    rdm_id_(rdm_id),
    next_hop_(next_hop),
    last_hop_(last_hop)
  {
    msg_attr_vec_ = NULL;
    next_port_ = 0;
    new_message_ = 1;             // New message by default, will be changed
                                  // later if message is found to be old
  }

  ~Message()
  {
    if (msg_attr_vec_){
      ClearAttrs(msg_attr_vec_);
      delete msg_attr_vec_;
    }
  }
};

extern NRSimpleAttributeFactory<void *> ControlMsgAttr;
extern NRSimpleAttributeFactory<void *> OriginalHdrAttr;

#define CONTROL_MESSAGE_KEY  1400
#define ORIGINAL_HEADER_KEY  1401

// Control Message types
typedef enum ctl_t_ {
  ADD_FILTER,
  REMOVE_FILTER,
  KEEPALIVE_FILTER,
  SEND_MESSAGE
} ctl_t;

class ControlMessage {
public:

  ControlMessage(int32_t command, int32_t param1, int32_t param2) :
    command_(command), param1_(param1), param2_(param2) {};

  int32_t command_;
  int32_t param1_;
  int32_t param2_;
};

class RedirectMessage {
public:

  RedirectMessage(int8_t new_message, int8_t msg_type, u_int16_t source_port,
		  u_int16_t data_len, u_int16_t num_attr, int32_t rdm_id,
		  int32_t pkt_num, int32_t next_hop, int32_t last_hop,
		  int32_t handle, u_int16_t next_port) :
    new_message_(new_message), msg_type_(msg_type), source_port_(source_port),
    data_len_(data_len), num_attr_(num_attr), rdm_id_(rdm_id),
    pkt_num_(pkt_num), next_hop_(next_hop), last_hop_(last_hop),
    handle_(handle), next_port_(next_port) {};

  int8_t  new_message_;
  int8_t  msg_type_;
  u_int16_t source_port_;
  u_int16_t data_len_;
  u_int16_t num_attr_;
  int32_t rdm_id_;
  int32_t pkt_num_;
  int32_t next_hop_;
  int32_t last_hop_;
  int32_t handle_;
  u_int16_t next_port_;
};

Message * CopyMessage(Message *msg);

#endif // !_MESSAGE_HH_