//
// header.hh     : Diffusion Datagram Header
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: header.hh,v 1.5 2002/11/26 22:45:39 haldar Exp $
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

#ifndef _HEADER_HH_
#define _HEADER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "nr/nr.hh"

typedef int *DiffPacket;

#define MAX_PKT_SIZE           8192
#define BROADCAST_ADDR           -1
#define LOCALHOST_ADDR           -2
#define DIFFUSION_VERSION         3

#ifdef NS_DIFFUSION
#define DEFAULT_DIFFUSION_PORT  255
#else
#define DEFAULT_DIFFUSION_PORT 2000
#endif // NS_DIFFUSION

#define DIFFUSION_PORT         DEFAULT_DIFFUSION_PORT

// For accessing fields in the diffusion header
#define LAST_HOP(x)     (x)->last_hop
#define NEXT_HOP(x)     (x)->next_hop
#define DIFF_VER(x)     (x)->version
#define MSG_TYPE(x)     (x)->msg_type
#define DATA_LEN(x)     (x)->data_len
#define PKT_NUM(x)      (x)->pkt_num
#define RDM_ID(x)       (x)->rdm_id
#define NUM_ATTR(x)     (x)->num_attr
#define SRC_PORT(x)     (x)->src_port

// Message types
typedef enum msg_t_ {
  INTEREST,
  POSITIVE_REINFORCEMENT,
  NEGATIVE_REINFORCEMENT,
  DATA,
  EXPLORATORY_DATA,
  PUSH_EXPLORATORY_DATA,
  CONTROL,
  REDIRECT,
} diff_msg_t;

// Header structure
struct hdr_diff {
  int32_t         last_hop;           // forwarder node id
  int32_t         next_hop;           // next hop node id
  int8_t          version;            // protocol version
  int8_t          msg_type;           // message type
  int16_t         data_len;           // data length
  int32_t         pkt_num;            // packet number
  int32_t         rdm_id;             // random id
  int16_t         num_attr;           // number of attributes
  int16_t         src_port;           // sender port id
};

#define HDR_DIFF(x)   (struct hdr_diff *)(x)

#endif // !_HEADER_HH_
