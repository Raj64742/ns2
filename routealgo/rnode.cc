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
 * Dummy rnode implementation
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#ifdef HAVE_STL
#ifdef NIXVECTOR


#include <stdio.h>
#include "rnode.h"

RNode::RNode( ) : m_id(NODE_NONE) { }
RNode::RNode( nodeid_t id) : m_id(id)  { }
RNode::RNode( const RNode& n) : m_id(n.m_id) { }
RNode::~RNode() { }
const NodeWeight_t RNode::NextAdj( const NodeWeight_t& ) 
{
  return(NodeWeight_t(NODE_NONE, 0));
}

void RNode::AddAdj(nodeid_t, int)
{
}

NixPair_t RNode::GetNix(nodeid_t)
{
  printf("Hello from RNOde::Getnix (should never occur)\n");
  return(NixPair_t(NIX_NONE, 0));
}

nodeid_t RNode::GetNeighbor(Nix_t)
{
  return(NODE_NONE);
}

Nixl_t RNode::GetNixl()
{
  return(0);
}

#endif /* NIXVECTOR */
#endif /* STL */
