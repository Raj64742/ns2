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
 * Implementation of Breadth First Search algorithm
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#ifndef __BFS_H__
#define __BFS_H__
#include "rnode.h"

// The prototype for the shortest path function
void BFS(RNodeVec_t&   N,       // List of nodes in the graph
         nodeid_t      root,    // Root node for route computations
         RoutingVec_t& NextHop, // Returned "FirstHop" vector
         RoutingVec_t& Parent); // Returned "Parent" vector
#endif
