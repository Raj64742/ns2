// 
// message.cc    : Message and Filters' factories
// authors       : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: message.cc,v 1.6 2003/07/09 17:50:02 haldar Exp $
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

#include "message.hh"

NRSimpleAttributeFactory<void *> ControlMsgAttr(CONTROL_MESSAGE_KEY,
						NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<void *> OriginalHdrAttr(ORIGINAL_HEADER_KEY,
						 NRAttribute::BLOB_TYPE);

Message * CopyMessage(Message *msg)
{
   Message *newMsg;

   newMsg = new Message(msg->version_, msg->msg_type_, msg->source_port_,
			msg->data_len_, msg->num_attr_, msg->pkt_num_,
			msg->rdm_id_, msg->next_hop_, msg->last_hop_);

   newMsg->new_message_ = msg->new_message_;
   newMsg->next_port_ = msg->next_port_;
   newMsg->msg_attr_vec_ = CopyAttrs(msg->msg_attr_vec_);

   return newMsg;
}
