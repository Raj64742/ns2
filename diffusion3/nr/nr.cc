// 
// nr.cc         : Network Routing
// authors       : Dan Coffin, John Heidemann, Dan Van Hook
// authors       : Fabio Silva
// 
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: nr.cc,v 1.4 2002/03/20 22:49:41 haldar Exp $
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

#include <stdio.h>   // for null
#include <string.h>  // for strdup
#include <algorithm>  // for stl min
#include "nr.hh"

NRSimpleAttributeFactory<float> LatitudeAttr(NRAttribute::LATITUDE_KEY, NRAttribute::FLOAT32_TYPE);
NRSimpleAttributeFactory<float> LongitudeAttr(NRAttribute::LONGITUDE_KEY, NRAttribute::FLOAT32_TYPE);
NRSimpleAttributeFactory<char *> RouteAttr(NRAttribute::ROUTE_KEY, NRAttribute::STRING_TYPE);
NRSimpleAttributeFactory<char *> SourceRouteAttr(NRAttribute::SOURCE_ROUTE_KEY, NRAttribute::STRING_TYPE);

NRAttributeFactory *NRAttributeFactory::first_ = NULL;

NRAttribute::NRAttribute(int key, int type, int op, int len, void *val)
   : key_(key), type_(type), op_(op), len_(len)
{
   if (val != NULL){
      val_ = (void *) new char[len_];
      memcpy(val_, val, len_);
   }
}

NRAttribute::NRAttribute() :    
   key_(0), type_(0), op_(0), len_(0), val_(NULL) 
{}


NRAttribute::NRAttribute(const NRAttribute &rhs)
{
   key_ = rhs.key_;
   type_ = rhs.type_;
   op_ = rhs.op_;
   len_ = rhs.len_;
   val_ = new char[len_];
   memcpy(val_, rhs.val_, len_);
}

NRAttribute::~NRAttribute()
{
}

NRAttribute * NRAttribute::find_key_from(int key, NRAttrVec *attrs,
					 NRAttrVec::iterator start,
					 NRAttrVec::iterator *place = NULL) {
   
   NRAttrVec::iterator i;
   
   for (i = start; i != attrs->end(); ++i) {
      if ((*i)->getKey() == key) {
	 if (place)
	    *place = i;
	 return (*i);
      };
   };
   return NULL;
}

bool NRAttribute::isEQ(NRAttribute *attr) {

   // Keys need to be the same
   if (!isSameKey(attr))
      abort();

   switch (type_){

   case INT32_TYPE:
      return (*(int32_t *) val_ == *(int32_t *) attr->getGenericVal());
      break;

   case FLOAT32_TYPE:
      return (*(float *) val_ == *(float *) attr->getGenericVal());
      break;

   case FLOAT64_TYPE:
      return (*(double *) val_ == *(double *) attr->getGenericVal());
      break;

   case STRING_TYPE:
      if (len_ != attr->getLen())
        return false;
      return (!strncmp((char *) val_, (char *) attr->getGenericVal(), len_));
      break;

   case BLOB_TYPE:
      if (len_ != attr->getLen())
        return false;
      return (!memcmp((void *) val_, (void *) attr->getGenericVal(), len_));
      break;

   default:
      abort();
      break;
   }
}

bool NRAttribute::isGT(NRAttribute *attr) {

   int cmplen, cmp;  // must be here for initialization reasons

   // Keys need to be the same
   if (!isSameKey(attr))
      abort();

   switch (type_) {

   case INT32_TYPE:
      return (*(int32_t *) val_ > *(int32_t *) attr->getGenericVal());
      break;

   case FLOAT32_TYPE:
      return (*(float *) val_ > *(float *) attr->getGenericVal());
      break;

   case FLOAT64_TYPE:
      return (*(double *) val_ > *(double *) attr->getGenericVal());
      break;

   case STRING_TYPE:
      return strncmp((char *) val_, (char *) attr->getGenericVal(),
		     min(len_, attr->getLen()));
      break;

   case BLOB_TYPE:
      cmplen = min(len_, attr->getLen());
      cmp = memcmp((void *) val_, (void *) attr->getGenericVal(), cmplen);
      
      // We are greater than attr
      if (cmp > 0)
	 return true;
      // We are equal to attr up to len_, but we are longer
      if (cmp == 0 && (len_ > attr->getLen()))
	 return true;
      return false;
      break;

   default:
      abort();
      break;
   }
}

bool NRAttribute::isGE(NRAttribute *attr) {

   int cmplen, cmp;  // must be here for initialization reasons

   // Keys need to be the same
   if (!isSameKey(attr))
      abort();

   switch (type_) {

   case INT32_TYPE:
      return (*(int32_t *) val_ >= *(int32_t *) attr->getGenericVal());
      break;

   case FLOAT32_TYPE:
      return (*(float *) val_ >= *(float *) attr->getGenericVal());
      break;

   case FLOAT64_TYPE:
      return (*(double *) val_ >= *(double *) attr->getGenericVal());
      break;

   case STRING_TYPE:
      return (strcmp((char *) val_, (char *) attr->getGenericVal()) >= 0);
      break;

   case BLOB_TYPE:
      cmplen = min(len_, attr->getLen());
      cmp = memcmp((void *) val_, (void *) attr->getGenericVal(), cmplen);
      
      // We are greater or equal to attr
      if (cmp >= 0)
	 return true;
      return false;
      break;

   default:
      abort();
      break;
   }
}

NRSimpleAttribute<char *>::NRSimpleAttribute(int key, int type, int op, char *val, int size) :
   NRAttribute(key, type, op, (strlen(val) + 1))
{
   assert(type == STRING_TYPE);
   val_ = NULL;
   setVal(val);
}

void NRSimpleAttribute<char *>::setVal(char *value) {
   delete [] (char *) val_;
   len_ = strlen(value) + 1;
   val_ = (void *) new char[len_ + 1];
   memcpy(val_, value, len_);
}

NRSimpleAttribute<void *>::NRSimpleAttribute(int key, int type, int op, void *val, int size) :
   NRAttribute(key, type, op, size)
{
   assert(type == BLOB_TYPE);
   val_ = NULL;
   setVal(val, len_);
}

void NRSimpleAttribute<void *>::setVal(void *value, int len) {
   delete [] (char *) val_;
   len_ = len;
   val_ = (void *) new char[len_];
   memcpy(val_, value, len_);
}

void NRAttributeFactory::verify_unique(NRAttributeFactory *baby) {
   
   NRAttributeFactory *i = first_, *last = NULL;
   while (i) {
      if (baby->key_ == i->key_) {
	 // identical factories are ok
	 assert(baby->type_ == i->type_);
	 return;  // don't enlist duplictates
      }
      last = i;
      i = i->next_;
   }
   // must not exist, add it
   if (last)
      last->next_ = first_;
   first_ = baby;
}
