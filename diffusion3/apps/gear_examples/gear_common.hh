//
// gear_common.hh : Gear Application Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_common.hh,v 1.2 2003/07/10 21:18:55 haldar Exp $
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

#ifndef _GEAR_COMMON_HH_
#define _GEAR_COMMON_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "diffapp.hh"

#define COUNTER_KEY 3601
#define TIME_KEY    3602

extern NRSimpleAttributeFactory<char *> GearTargetAttr;
extern NRSimpleAttributeFactory<int> GearCounterAttr;
extern NRSimpleAttributeFactory<void *> GearTimeAttr;

class EventTime {
public:
  long seconds_;
  long useconds_;
};

#endif // !_GEAR_COMMON_HH_
