// 
// message.cc    : Message and Filters' factories
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: message.cc,v 1.4 2002/03/20 22:49:41 haldar Exp $
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

NRSimpleAttributeFactory<void *> ControlMsgAttr(CONTROL_MESSAGE_KEY, NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<void *> OriginalHdrAttr(ORIGINAL_HEADER_KEY, NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<int> NRScopeAttr(NRAttribute::SCOPE_KEY, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<int> NRClassAttr(NRAttribute::CLASS_KEY, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<void *> ReinforcementAttr(REINFORCEMENT_KEY, NRAttribute::BLOB_TYPE);

Message * CopyMessage(Message *msg)
{
   Message *newMsg;

   newMsg = new Message(msg->version, msg->msg_type, msg->source_port,
			msg->data_len, msg->num_attr, msg->pkt_num,
			msg->rdm_id, msg->next_hop, msg->last_hop);

   newMsg->new_message = msg->new_message;
   newMsg->next_port = msg->next_port;
   newMsg->msg_attr_vec = CopyAttrs(msg->msg_attr_vec);

   return newMsg;
}
