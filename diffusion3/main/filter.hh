// 
// filter.hh     : Filter definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: filter.hh,v 1.4 2002/03/20 22:49:40 haldar Exp $
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

#ifndef filter_hh
#define filter_hh

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
  NRAttrVec          *filterAttrs;
  int16_t            handle;
  u_int16_t          priority;
  u_int16_t          agent;
  FilterCallback     *cb;
  struct timeval     tmv;
  bool               valid;
   
  FilterEntry(int16_t _handle, u_int16_t _priority, u_int16_t _agent) : handle(_handle), priority(_priority), agent(_agent)
  {
    valid = true;
    cb = NULL;
    getTime(&tmv);
  }

  ~FilterEntry()
  {
    if (filterAttrs){
      ClearAttrs(filterAttrs);
      delete filterAttrs;
    }
  }
};

class FilterCallback {
public:
  virtual void recv(Message *msg, handle h) = 0;
};

#endif // filter_hh
