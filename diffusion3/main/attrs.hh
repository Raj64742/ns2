//
// attrs.hh        : Attribute Functions Definitions
// authors         : John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: attrs.hh,v 1.6 2002/03/20 22:49:40 haldar Exp $
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

//#ifdef freeBSD
//#include <sys/param.h>
//#else
//#include <netinet/in.h>
//#endif // freeBSD

#include <unistd.h>
#include <netinet/in.h>

#include "nr.hh"
#include "header.hh"
#include "tools.hh"

// Here we define a few functions that will help developers create
// and manipulate attribute sets.

// These functions allow easy attribute manipulation
NRAttrVec * CopyAttrs(NRAttrVec *src_attrs);
void AddAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
void ClearAttrs(NRAttrVec *attr_vec);
void PrintAttrs(NRAttrVec *attr_vec);

// These functions allow attributes to be placed into packets
DiffPacket AllocateBuffer(NRAttrVec *attr_vec);
int CalculateSize(NRAttrVec *attr_vec);
int PackAttrs(NRAttrVec *attr_vec, char *start_pos);
NRAttrVec * UnpackAttrs(DiffPacket pkt, int num_attr);

// These functions can be used to match attribute sets
bool PerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool OneWayPerfectMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool MatchAttrs(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);
bool OneWayMatch(NRAttrVec *attr_vec1, NRAttrVec *attr_vec2);

#endif
