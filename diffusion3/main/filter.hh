// 
// filter.hh     : Filter definitions
// authors       : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: filter.hh,v 1.2 2001/11/20 22:28:17 haldar Exp $
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

#ifdef NS_DIFFUSION

#ifndef filter_hh
#define filter_hh

#include <sys/time.h>
#include <unistd.h>

#include <list>

#include "message.hh"

using namespace std;

class Filter_Entry;
class FilterCallback;

typedef list<Filter_Entry *> FilterList;
typedef long handle;

class Filter_Entry {
public:
  NRAttrVec          *filterAttrs;
  int16_t            handle;
  u_int16_t          priority;
  u_int16_t          agent;
  FilterCallback     *cb;
  struct timeval     tmv;
  bool               valid;
   
  Filter_Entry(int16_t _handle, u_int16_t _priority, u_int16_t _agent) : handle(_handle), priority(_priority), agent(_agent)
  {
    valid = true;
    cb = NULL;
    getTime(&tmv);
  }

  ~Filter_Entry()
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

#endif
#endif // NS
