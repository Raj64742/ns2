//
// attrs.hh        : Attribute Functions Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: attrs.hh,v 1.1 2001/11/08 17:42:31 haldar Exp $
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

#ifndef ATTRS_HH
#define ATTRS_HH

#include "netinet/in.h"
#include "nr.hh"
#include "header.hh"

int CalculateSize(NRAttrVec *attr_vec);
int PackAttrs(NRAttrVec *attr_vec, char *start_pos);
NRAttrVec * CopyAttrs(NRAttrVec *src_attrs);
NRAttrVec * UnpackAttrs(DiffPacket pkt, int num_attr);
void ClearAttrs(NRAttrVec *attr_vec);
bool PerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool OneWayPerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool MatchAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool OneWayMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
void printAttrs(NRAttrVec *attr_vec);

#endif
