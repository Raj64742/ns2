//
// gear_common.cc : Gear App Common File
// author         : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_common.cc,v 1.1 2003/07/09 17:43:30 haldar Exp $
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

#include "gear_common.hh"

NRSimpleAttributeFactory<char *> TargetAttr(NRAttribute::TARGET_KEY, NRAttribute::STRING_TYPE);
NRSimpleAttributeFactory<int> AppCounterAttr(COUNTER_KEY, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<void *> TimeAttr(TIME_KEY, NRAttribute::BLOB_TYPE);
