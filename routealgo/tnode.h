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
 * Test nodes for testing routing algorithms
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#ifndef __TNODE_H__
#define __TNODE_H__

#include <stdio.h>

#include "routealgo/rnode.h"

// Define the edge class
class Edge {
public :
  Edge( nodeid_t n, int w) : m_n(n), m_w(w) { };
  Edge( Edge& e) : m_n(e.m_n), m_w(e.m_w) { printf("Edge copy const\n");}
public :
  nodeid_t m_n;
  int      m_w;
};

typedef vector<Edge*>         EdgeVec_t;
typedef EdgeVec_t::iterator   EdgeVec_it;

class Node : public RNode {
public :
  Node( nodeid_t id) : RNode(id) { };
  Node( const Node& n) : RNode(n.m_id) { };
  virtual ~Node() { };
  virtual const NodeWeight_t NextAdj( const NodeWeight_t&);
  virtual void AddAdj(nodeid_t a, int w = 1);
  virtual NixPair_t GetNix(nodeid_t);  // Get neighbor index for specified node
public :
  EdgeVec_t m_Adj; // Adjacent edges
};

#endif

