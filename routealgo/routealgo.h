/*
 * Copyright (c) 2000 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Generic helpers (all routing algorithms)
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#ifndef __ROUTEALGO_H__
#define __ROUTEALGO_H__

#include "rnode.h"
// Print the route, given source s, destination d and predecessor vector pred
void PrintRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred);

// Create the NixVector
void NixRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred,
               RNodeVec_t& nodes, NixVec& nv);

// Print the parent list (debug)
void PrintParents(RoutingVec_t& pred);

// Print the route from the nixvector (debug)
void PrintNix(nodeid_t s, RNodeVec_t nodes, NixVec& nv);

#endif
