//
// ping.hh        : Ping Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: ping.hh,v 1.2 2002/03/20 22:49:39 haldar Exp $
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

#include "diffapp.hh"

#define COUNTER_KEY 3601
#define TIME_KEY    3602

extern NRSimpleAttributeFactory<char *> TargetAttr;
extern NRSimpleAttributeFactory<int> AppCounterAttr;
extern NRSimpleAttributeFactory<void *> TimeAttr;

class EventTime {
public:
  long seconds;
  long useconds;
};

