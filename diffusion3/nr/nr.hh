// 
// nr.hh         : Network Routing Class Definitions
// authors       : Dan Coffin, John Heidemann, Dan Van Hook
// authors       : Fabio Silva
// 
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: nr.hh,v 1.10 2002/03/21 22:07:41 haldar Exp $
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

#ifndef _NR_H
#define _NR_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>

#ifdef NS_DIFFUSION
#include "config.h"
#endif // NS_DIFFUSION

using namespace std;

typedef signed int int32_t;
typedef signed short int16_t;
#ifdef sparc
typedef char int8_t;
#else
// Conflicts with system declaration of int8_t in Solaris
typedef signed char int8_t;
#endif // sparc

#define FAIL -1
#define OK    0

class NRAttribute;

typedef vector<NRAttribute *> NRAttrVec;

/*
 * xxx: gcc-2.91.66 doesn't handle member templates completely
 * (it worked for NRSimpleFactory<> but not for Blob and String
 * factories.  A work-around is to move all the templates out
 * to top-level code.
 * I'm told (by a gcc-developer) that this limitation is fixed
 * in gcc-2.95.2.
 */

/*
 * NRAttribute encapsulates a single, generic attribute.
 */
class NRAttribute {
public:
  enum keys {
    // reserved constant values used for key
    // range 1000-1999 is diffusion-specific
    SCOPE_KEY = 1001,             // INT32_TYPE
    CLASS_KEY = 1002,             // INT32_TYPE

    // range 2000-2999 is app specific
    LATITUDE_KEY = 2001,          // FLOAT_TYPE
    LONGITUDE_KEY = 2002,         // FLOAT_TYPE
    DATABLOCK_KEY = 2003,         // BLOB_TYPE
    TASK_FREQUENCY_KEY = 2004,    // FLOAT_TYPE, in secs
    TASK_NAME_KEY = 2005,         // STRING_TYPE
    TASK_QUERY_DETAIL_KEY = 2006, // BLOB_TYPE
    TARGET_KEY = 2007,            // STRING_TYPE
    TARGET_RANGE_KEY = 2008,      // FLOAT_TYPE
    CONFIDENCE_KEY = 2009,        // FLOAT_TYPE
    ROUTE_KEY = 2010,             // STRING_TYPE
    SOURCE_ROUTE_KEY = 2011       // STRING_TYPE

    // range 3000-3999 is reserved for experimentation
    // and user-defined keys
  };

  // Values for diffusion-specific keys
  enum classes { INTEREST_CLASS, DISINTEREST_CLASS, DATA_CLASS };
  enum scopes { NODE_LOCAL_SCOPE, GLOBAL_SCOPE };

  // Key Type values
  enum types { INT32_TYPE,    // 32-bit signed integer
	       FLOAT32_TYPE,  // 32-bit
	       FLOAT64_TYPE,  // 64-bit
	       STRING_TYPE,   // UTF-8 format, max length 1024 chars
	       BLOB_TYPE };   // uninterpreted binary data

  // Match Operator values
  enum operators { IS, LE, GE, LT, GT, EQ, NE, EQ_ANY };
  // with EQ_ANY, the val is ignored

  NRAttribute();
  NRAttribute(int key, int type, int op, int len, void *val = NULL);
  NRAttribute(const NRAttribute &rhs);
  virtual ~NRAttribute();

  static NRAttribute * find_key(int key, NRAttrVec *attrs,
				NRAttrVec::iterator *place = NULL) {

    return find_key_from(key, attrs, attrs->begin(), place);
  };

  static NRAttribute * find_key_from(int key, NRAttrVec *attrs,
				     NRAttrVec::iterator start,
				     NRAttrVec::iterator *place = NULL);

  NRAttribute * find_matching_key_from(NRAttrVec *attrs,
				       NRAttrVec::iterator start,
				       NRAttrVec::iterator *place = NULL) {
    return find_key_from(key_, attrs, start, place);
  };

  NRAttribute * find_matching_key(NRAttrVec *attrs,
				  NRAttrVec::iterator *place = NULL) {
    return find_key_from(key_, attrs, attrs->begin(), place);
  };

  int32_t getKey() { return key_; };
  int8_t getType() { return type_; };
  int8_t getOp() { return op_; };
  int16_t getLen() { return len_; };

  void * getGenericVal() { return val_; };

  bool isSameKey(NRAttribute *attr) {
    return ((type_ == attr->getType()) && (key_ == attr->getKey()));
  };

  bool isEQ(NRAttribute *attr);

  bool isGT(NRAttribute *attr);

  bool isGE(NRAttribute *attr);

  bool isNE(NRAttribute *attr) {
    return (!isEQ(attr));
  };

  bool isLT(NRAttribute *attr) {
    return (!isGE(attr));
  };

  bool isLE(NRAttribute *attr) {
    return (!isGT(attr));
  };

protected:

  int32_t key_;
  int8_t  type_;
  int8_t  op_;
  int16_t len_;
  void *val_;
};


/*
 * NRSimpleAttribute<X> provide type-safe ways to accesss generic attributes
 * of type X.
 *
 * We specialize for strings and blobs to handle lengths properly.
 */

template<class T>
class NRSimpleAttribute : public NRAttribute{
public:
  NRSimpleAttribute(int key, int type, int op, T val, int size = 0) :
    NRAttribute(key, type, op, sizeof(T)) {

    assert(type != STRING_TYPE && type != BLOB_TYPE);
    val_ = new T(val);
  }

  ~NRSimpleAttribute() {
    assert(type_ != STRING_TYPE && type_ != BLOB_TYPE);
    delete (T*) val_;
  };

  T getVal() { return *(T*)val_; };
  int getLen() { return len_; };
  void setVal(T value) { *(T *)val_ = value; };
};

// string specialization
class NRSimpleAttribute<char *>: public NRAttribute {
public:
  NRSimpleAttribute(int key, int type, int op, char *val, int size = 0);

  ~NRSimpleAttribute() {
    assert(type_ == STRING_TYPE);
    delete [] (char *) val_;
  };

  char * getVal() { return (char *)val_; };
  int getLen() {return len_; };
  void setVal(char *value);
};

// blob specialization
class NRSimpleAttribute<void *>: public NRAttribute {
public:
  NRSimpleAttribute(int key, int type, int op, void *val, int size);

  ~NRSimpleAttribute() {
    assert(type_ == BLOB_TYPE);
    delete [] (char *) val_;
  };

  void * getVal() { return (void *)val_; };
  int getLen() {return len_; };
  void setVal(void *value, int len);
};

/*
 * NRAttributeFactory and NRSimpleAttributeFactory
 * are used to define "factories" that make attributes of a given
 * key and type.
 *
 * NRAttributeFactory only for internal use.
 * (This class cannot be a template because of the static variables.)
 */
class NRAttributeFactory {
protected:
  int16_t key_;
  int8_t type_;

  NRAttributeFactory *next_;
  static NRAttributeFactory *first_;

  // Keep a list of all factories and verify that they don't conflict.
  static void verify_unique(NRAttributeFactory *baby);

  NRAttributeFactory(int key, int type) : key_(key), type_(type), next_(NULL) {};
};

/*
 * NRAttributeFactory for users
 */
template<class T>
class NRSimpleAttributeFactory : public NRAttributeFactory {
public:
  NRSimpleAttributeFactory(int key, int type) : NRAttributeFactory(key, type) {
    verify_unique(this);
  }
  NRSimpleAttribute<T>* make(int op, T val, int size = -1) {
    return new NRSimpleAttribute<T>(key_, type_, op, val, size);
  };
  NRSimpleAttribute<T>* find(NRAttrVec *attrs,
			     NRAttrVec::iterator *place = NULL) {
    return (NRSimpleAttribute<T>*)NRAttribute::find_key_from(key_, attrs, attrs->begin(), place);
  };

  NRSimpleAttribute<T>* find_from(NRAttrVec *attrs,
				  NRAttrVec::iterator start,
				  NRAttrVec::iterator *place = NULL) {
    return (NRSimpleAttribute<T>*)NRAttribute::find_key_from(key_, attrs, start, place);
  };
  int getKey() { return key_; };
  int getType() { return type_; };
};

/*
 * Some pre-defined attribute factories.
 * There's no reason these can't also appear in user code.
 * (Not all factories need to be defined here.)
 */
extern NRSimpleAttributeFactory<float> LatitudeAttr;
extern NRSimpleAttributeFactory<float> LongitudeAttr;
extern NRSimpleAttributeFactory<char *> RouteAttr;
extern NRSimpleAttributeFactory<char *> SourceRouteAttr;

#ifdef NS_DIFFUSION
class DiffAppAgent;
#endif // NS_DIFFUSION

class NR {
public:
  typedef long handle;

  class Callback {
  public:
    virtual void recv(NRAttrVec *data, handle h) = 0;
  };

  // Factory to create an NR class specialized for ISI-W or MIT-LL's
  // implementation (whichever is compiled in).
#ifdef NS_DIFFUSION
  static NR * create_ns_NR(u_int16_t port, DiffAppAgent *da);
#else
  static NR * createNR(u_int16_t port = 0);
#endif // NS_DIFFUSION

  virtual handle subscribe(NRAttrVec *interest_declarations,
			   NR::Callback * cb) = 0;

  virtual int unsubscribe(handle subscription_handle) = 0;

  virtual handle publish(NRAttrVec *publication_declarations) = 0;

  virtual int unpublish(handle publication_handle) = 0;

  virtual int send(handle publication_handle, NRAttrVec *) = 0;
};
			    
#endif // _NR_H

