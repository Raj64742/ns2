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
 *
 */

#ifdef HAVE_STL
#ifdef NIXVECTOR

#include "routealgo/bfs.h"
#include "routealgo/routealgo.h"
#include "routealgo/rnode.h"
#include "routealgo/tnode.h"
#include "routealgo/rbitmap.h"
#include <stdio.h>

#ifndef TEST_BFS

void BFS(
         RNodeVec_t& N,
         nodeid_t root,
         RoutingVec_t& NextHop,
         RoutingVec_t& Parent)
{  // Compute shortest path to all nodes from node S using BreadthFirstSearch
BitMap B(N.size()); // Make a bitmap for the colors (grey/white) (white = 0)
RNodeDeq_t Q;       // And a vector for the Q list
DistVec_t  D;       // And the distance vector

 // Fill in all "NONE" in the next hop neighbors and predecessors
 NextHop.erase(NextHop.begin(), NextHop.end());
 Parent.erase(Parent.begin(), Parent.end());
 for (unsigned int i = 0; i < N.size(); i++)
   {
     NextHop.push_back(NODE_NONE);
     Parent.push_back(NODE_NONE);
     D.push_back(INF);
     // Debug...print adj lists
     NodeWeight_t v(NODE_NONE, INF);
     if(0)printf("Printing adj for node %ld (addr %p)\n", N[i]->m_id, N[i]);
     if(0)while(1)
       {
         v = N[i]->NextAdj(v);
         if (v.first == NODE_NONE) break;
         if(0)printf("Found adj %ld\n", v.first);
       }
   }
 B.Set(root); // Color the root grey
 if(0)B.DBPrint();
 Q.push_back(N[root]); // And put the root in Q
 D[root] = 0;
 while(Q.size() != 0)
   {
     RNodeDeq_it it = Q.begin();
     NodeWeight_t v(NODE_NONE, INF);
     RNode* u = *it;
     if(0)printf("Working on node %ld addr %p\n", u->m_id, u);
     while(1)
       {
         v = u->NextAdj(v);
         if (v.first == NODE_NONE) break;
         if(0)printf("Found adj %ld\n", v.first);
         if (B.Get(v.first) == 0)
           { // White
             Q.push_back(N[v.first]);     // Add to Q set
             B.Set(v.first);              // Change to grey
             D[v.first] = D[u->m_id] + 1; // Set new distance
             Parent[v.first] = u->m_id;   // Set parent
             if (u->m_id == root)
               { // First hop is new node since this is root
                 NextHop[v.first] = v.first;
               }
             else
               { // First hop is same as this one
                 NextHop[v.first] = NextHop[u->m_id];
               }
             if(0)printf("Enqueued %ld\n", v.first);
           }
       }
     Q.pop_front();
   }
}
#endif

#ifdef TEST_BFS
RNodeVec_t Nodes;

int main()
{
  // See the sample BFS search in Fig23.3, p471 CLR Algorithms book
Node N0(0);
Node N1(1);
Node N2(2);
Node N3(3);
Node N4(4);
Node N5(5);
Node N6(6);
Node N7(7);
RoutingVec_t NextHop;
RoutingVec_t Parent;

 N0.AddAdj(1);
 N0.AddAdj(2);

 N1.AddAdj(0);

 N2.AddAdj(0);
 N2.AddAdj(3);

 N3.AddAdj(2);
 N3.AddAdj(4);
 N3.AddAdj(5);

 N4.AddAdj(3);
 N4.AddAdj(5);
 N4.AddAdj(6);

 N5.AddAdj(4);
 N5.AddAdj(7); 

 N6.AddAdj(4);
 N6.AddAdj(7);

 N7.AddAdj(5);
 N7.AddAdj(6);

 Nodes.push_back(&N0);
 Nodes.push_back(&N1);
 Nodes.push_back(&N2);
 Nodes.push_back(&N3);
 Nodes.push_back(&N4);
 Nodes.push_back(&N5);
 Nodes.push_back(&N6);
 Nodes.push_back(&N7);

 for (nodeid_t i = 0; i < Nodes.size(); i++)
   { // Get shortest path for each root node
     printf("\nFrom root %ld\n", i);
     BFS(Nodes, i, NextHop, Parent);
     PrintParents(Parent);
     for (unsigned int k = 0; k < Nodes.size(); k++)
       printf("Next hop for node %d is %ld\n", k, NextHop[k]);
     printf("Printing paths\n");
     for (nodeid_t j = 0; j < Nodes.size(); j++)
       {
         PrintRoute(i, j, Parent);
       }
   }
 return(0);
}
#endif

#endif /* NIXVECTOR */
#endif
