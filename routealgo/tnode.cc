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

#ifdef HAVE_STL
#ifdef NIXVECTOR

#include <stdio.h>

#include "routealgo/tnode.h"

void Node::AddAdj( nodeid_t a, int w)
{
  Edge* pE;

  pE= new Edge(a, w);
  m_Adj.push_back(pE);
}

const NodeWeight_t Node::NextAdj( const NodeWeight_t& last)
{
Edge* pE;

  static EdgeVec_it prev;
  if (last.first == NODE_NONE)
    {
      prev = m_Adj.begin();
      pE = *prev;
      if(0)printf("NextAdj returning first edge %ld w %d\n",
             pE->m_n, pE->m_w);
      return(NodeWeight_t(pE->m_n, pE->m_w));
    }
  else
    { // See if last is prev
      if (last.first == (*prev)->m_n)
        { //Yep, just advance iterator
          prev++;
          if (prev == m_Adj.end())
            { // No more
              return(NodeWeight_t(NODE_NONE, 0));
            }
          else
            {
              pE = *prev;
              if(0)printf("NextAdj returning next edge %ld w %d\n",
                     pE->m_n, pE->m_w);
              return(NodeWeight_t(pE->m_n, pE->m_w));
            }
        }
      else
        { // Need to code this
          printf("Non-linear advance of NextAdj\n");
          exit(1);
        }
    }
}

#endif /* NIXVECTOR */
#endif
