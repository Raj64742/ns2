// 
// filter.hh     : Filter definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: filter.hh,v 1.3 2002/05/13 22:33:45 haldar Exp $
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

#ifndef _FILTER_HH_
#define _FILTER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#define FILTER_MIN_PRIORITY 1
#define FILTER_MAX_PRIORITY 254
#define FILTER_KEEP_PRIORITY 255
#define FILTER_ZERO_PRIORITY 0
#define FILTER_INVALID_PRIORITY 256

#include <sys/time.h>
#include <unistd.h>

#include <list>

#include "message.hh"

using namespace std;

class FilterEntry;
class FilterCallback;

typedef list<FilterEntry *> FilterList;
typedef long handle;

class FilterEntry {
public:
  NRAttrVec          *filterAttrs_;
  int16_t            handle_;
  u_int16_t          priority_;
  u_int16_t          agent_;
  FilterCallback     *cb_;
  struct timeval     tmv_;
  bool               valid_;
   
  FilterEntry(int16_t handle, u_int16_t priority, u_int16_t agent) :
    handle_(handle), priority_(priority), agent_(agent)
  {
    valid_ = true;
    cb_ = NULL;
    GetTime(&tmv_);
  }

  ~FilterEntry()
  {
    if (filterAttrs_){
      ClearAttrs(filterAttrs_);
      delete filterAttrs_;
    }
  }
};

class FilterCallback {
public:
  virtual void recv(Message *msg, handle h) = 0;
};

#endif // !_FILTER_HH_
