//
// attrs.cc        : Attribute Functions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: attrs.cc,v 1.3 2001/12/11 23:21:44 haldar Exp $
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

#include "attrs.hh"
#include <stdlib.h>
#include <stdio.h>

void ClearAttrs(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    delete *itr;
  }

  attr_vec->clear();
}

void printAttrs(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;
  NRAttribute *aux;
  int counter = 0;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    aux = *itr;
    diffPrint(DEBUG_ALWAYS, "Attribute #%d\n", counter);
    counter++;
    diffPrint(DEBUG_ALWAYS, "-------------\n");
    diffPrint(DEBUG_ALWAYS, "Type = %d\n", aux->getType());
    diffPrint(DEBUG_ALWAYS, "Key  = %d\n", aux->getKey());
    diffPrint(DEBUG_ALWAYS, "Op   = %d\n", aux->getOp());
    diffPrint(DEBUG_ALWAYS, "Len  = %d\n", aux->getLen());
    switch(aux->getType()){
    case NRAttribute::INT32_TYPE:
      diffPrint(DEBUG_ALWAYS, "Val  = %d\n",
		((NRSimpleAttribute<int> *)aux)->getVal());
      break;
    case NRAttribute::FLOAT32_TYPE:
      diffPrint(DEBUG_ALWAYS, "Val  = %f\n",
		((NRSimpleAttribute<float> *)aux)->getVal());
      break;
    case NRAttribute::FLOAT64_TYPE:
      diffPrint(DEBUG_ALWAYS, "Val  = %f\n",
		((NRSimpleAttribute<double> *)aux)->getVal());
      break;
    case NRAttribute::STRING_TYPE:
      diffPrint(DEBUG_ALWAYS, "Val  = %s\n",
		((NRSimpleAttribute<char *> *)aux)->getVal());
      break;
    case NRAttribute::BLOB_TYPE:
      diffPrint(DEBUG_ALWAYS, "Val  = %s\n",
		((NRSimpleAttribute<void *> *)aux)->getVal());
      break;
    default:
      diffPrint(DEBUG_ALWAYS, "Val  = Unknown\n");
      break;
    }
    diffPrint(DEBUG_ALWAYS, "\n");
  }
}

int CalculateSize(NRAttrVec *attr_vec)
{
  NRAttrVec::iterator itr;
  NRAttribute *temp;
  int pad, attr_len;
  int data_len = 0;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    temp = *itr;
    attr_len = temp->getLen();
    pad = 0;
    if (attr_len % sizeof(int)){
      // We have to pad to avoid bus errors
      pad = sizeof(int) - (attr_len % sizeof(int));
    }
    data_len = data_len + sizeof(Packed_Attribute) - sizeof(int32_t) + attr_len + pad;
  }
  return data_len;
}

int PackAttrs(NRAttrVec *attr_vec, char *start_pos)
{
  NRAttrVec::iterator itr;
  NRAttribute *temp;
  int32_t t;
  Packed_Attribute *current;
  char *pos;
  int pad;
  int data_len = 0;
  int my_len;

  current = (Packed_Attribute *) start_pos;

  for (itr = attr_vec->begin(); itr != attr_vec->end(); ++itr){
    temp = *itr;
    current->key = htonl(temp->getKey());
    current->type = temp->getType();
    current->op = temp->getOp();
    my_len = temp->getLen();
    current->len = htons(my_len);
    if (my_len > 0){
      switch (current->type){
      case NRAttribute::INT32_TYPE:

	t = htonl(((NRSimpleAttribute<int> *)temp)->getVal());
	memcpy(&current->val, &t, sizeof(int32_t));

	break;

      default:

	memcpy(&current->val, temp->getGenericVal(), my_len);

	break;
      }
    }
    pad = 0;
    if (my_len % sizeof(int)){
      // We have to pad to avoid bus errors
      pad = sizeof(int) - (my_len % sizeof(int));
    }
    data_len = data_len + sizeof(Packed_Attribute) - sizeof(int32_t) + my_len + pad;
    pos = (char *) current + sizeof(Packed_Attribute) - sizeof(int32_t) + my_len + pad;
    current = (Packed_Attribute *) pos;
  }
  return data_len;
}

NRAttrVec * UnpackAttrs(DiffPacket pkt, int num_attr)
{
  NRAttrVec *attr_vec;
  Packed_Attribute *current;

  int attr_len;
  char *pos;
  int pad;

  attr_vec = new NRAttrVec;

  pos = (char *) &pkt[0];
  current = (Packed_Attribute *) (pos + sizeof(struct hdr_diff));

  for (int i = 0; i < num_attr; i++){
    switch (current->type){
    case NRAttribute::INT32_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<int>(ntohl(current->key),
						NRAttribute::INT32_TYPE,
						current->op,
						ntohl(*(int *)&current->val)));
      break;

    case NRAttribute::FLOAT32_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<float>(ntohl(current->key),
						NRAttribute::FLOAT32_TYPE,
					        current->op,
					        *(float *)&current->val));
      break;

    case NRAttribute::FLOAT64_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<double>(ntohl(current->key),
					     NRAttribute::FLOAT64_TYPE,
					     current->op,
					     *(double *)&current->val));
      break;

    case NRAttribute::STRING_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<char *>(ntohl(current->key),
					    NRAttribute::STRING_TYPE,
					    current->op,
					    (char *)&current->val));
      break;

    case NRAttribute::BLOB_TYPE:
      attr_vec->push_back(new NRSimpleAttribute<void *>(ntohl(current->key),
					  NRAttribute::BLOB_TYPE,
				          current->op,
					  (void *)&current->val,
			                  ntohs(current->len)));
      break;

    default:

      printf("Unknown attribute type found while unpacking a message !\n");
      break;
    }

    attr_len = ntohs(current->len);

    pad = 0;
    if (attr_len % sizeof(int)){
      // We have to pad to avoid bus errors
      pad = sizeof(int) - (attr_len % sizeof(int));
    }
    pos = (char *) current + sizeof(Packed_Attribute) - sizeof(int32_t) + attr_len + pad;
    current = (Packed_Attribute *) pos;
  }

  return attr_vec;
}

NRAttrVec * CopyAttrs(NRAttrVec *src_attrs)
{
  NRAttrVec *dst_attrs;
  NRAttrVec::iterator itr;
  NRAttribute *src;

  dst_attrs = new NRAttrVec;

  for (itr = src_attrs->begin(); itr != src_attrs->end(); ++itr){
    src = *itr;
    switch (src->getType()){
    case NRAttribute::INT32_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<int>(src->getKey(),
						NRAttribute::INT32_TYPE,
					        src->getOp(),
					       ((NRSimpleAttribute<int> *)src)->getVal()));
      break;

    case NRAttribute::FLOAT32_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<float>(src->getKey(),
						NRAttribute::FLOAT32_TYPE,
						src->getOp(),
						((NRSimpleAttribute<float> *)src)->getVal()));
      break;

    case NRAttribute::FLOAT64_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<double>(src->getKey(),
						 NRAttribute::FLOAT64_TYPE,
						 src->getOp(),
						 ((NRSimpleAttribute<double> *)src)->getVal()));
      break;

    case NRAttribute::STRING_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<char *>(src->getKey(),
						NRAttribute::STRING_TYPE,
						src->getOp(),
						((NRSimpleAttribute<char *> *)src)->getVal()));
      break;

    case NRAttribute::BLOB_TYPE:
      dst_attrs->push_back(new NRSimpleAttribute<void *>(src->getKey(),
					      NRAttribute::BLOB_TYPE,
					      src->getOp(),
					      ((NRSimpleAttribute<void *> *)src)->getVal(),
					      src->getLen()));
      break;

    default:

      printf("Unknown attribute type found while copying attributes !\n");
      break;
    }
  }
  return dst_attrs;
}

bool PerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  if (OneWayPerfectMatch(attr_vec1, attr_vec2))
    if (OneWayPerfectMatch(attr_vec2, attr_vec1))
      return true;

  return false;
}

bool OneWayPerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  NRAttrVec::iterator itr1, itr2;
  NRAttribute *a;
  NRAttribute *b;

  if (attr_vec1->size() != attr_vec2->size())
    return false;

  for (itr1 = attr_vec1->begin(); itr1 != attr_vec1->end(); ++itr1){

    // For each attribute in vec1, we look in vec2 for the same attr
    a = *itr1;
    itr2 = attr_vec2->begin();
    b = a->find_matching_key_from(attr_vec2, itr2, &itr2);

    while (b){
      // They should have the same operator and be "EQUAL"
      if (a->getOp() == b->getOp())
	if (a->isEQ(b))
	  break;

      // Keys match but value/operator don't. Let's
      // Try to find another attribute with the same key
      itr2++;
      b = a->find_matching_key_from(attr_vec2, itr2, &itr2);
    }

    // Check if attribute found
    if (b == NULL)
      return false;
  }

  // All attributes match !
  return true;
}

bool MatchAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  if (OneWayMatch(attr_vec1, attr_vec2))
    if (OneWayMatch(attr_vec2, attr_vec1))
      return true;

  return false;
}

bool OneWayMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2)
{
  NRAttrVec::iterator itr1, itr2;
  NRAttribute *a;
  NRAttribute *b;
  bool found_attr;

  for (itr1 = attr_vec1->begin(); itr1 != attr_vec1->end(); ++itr1){

    // For each attribute in vec1 that has an operator different
    // than "IS", we need to find a "matchable" one in vec2
    a = *itr1;
    if (a->getOp() == NRAttribute::IS)
      continue;

    itr2 = attr_vec2->begin();
    for (;;){
      b = a->find_matching_key_from(attr_vec2, itr2, &itr2);

      // Found ?
      if (!b)
	return false;

      // If Op is not "IS" we need keep looking
      if (b->getOp() != NRAttribute::IS){
	itr2++;
	continue;
      }

      // If a's Op is "EQ_ANY" that's enough   
      if (a->getOp() == NRAttribute::EQ_ANY){
	found_attr = true;
	break;
      }

      found_attr = false;

      switch (a->getOp()){

	 case NRAttribute::EQ:

	   if (b->isEQ(a))
	     found_attr = true;

	   break;

      case NRAttribute::GT:

	if (b->isGT(a))
	  found_attr = true;

	break;

      case NRAttribute::GE:

	if (b->isGE(a))
	  found_attr = true;

	break;

      case NRAttribute::LT:

	if (b->isLT(a))
	  found_attr = true;

	break;

      case NRAttribute::LE:

	if (b->isLE(a))
	  found_attr = true;

	break;

      case NRAttribute::NE:

	if (b->isNE(a))
	  found_attr = true;

	break;

      default:

	printf("Unknown operator found in OneWayMacth !\n");
	break;
      }

      if (found_attr)
	break;

      itr2++;
    }
  }

  // All attributes found !
  return true;
}

