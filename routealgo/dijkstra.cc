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
 * Implementation of Dijkstra's SPF algorithm
 * contributed to ns
 * George Riley, Georgia Tech, Winter 2000
 */

#ifdef HAVE_STL
#ifdef NIXVECTOR

#include "routealgo/dijkstra.h"
#include "routealgo/routealgo.h"
#include "routealgo/tnode.h"

#include <stdio.h>

#ifndef TEST_DIJ
// Declare a multimap for maintaining a sorted Q set
typedef multimap<dist_t, nodeid_t> Q_t;
typedef Q_t::iterator              Q_it;
typedef Q_t::value_type            QPair_t;

void Dijkstra(
              RNodeVec_t& N,
              nodeid_t root,
              RoutingVec_t& NextHop,
              RoutingVec_t& Parent)
{  // Compute shortest path to all nodes from node S
Q_t Q;
Q_t S; // The completed set, with final weights

 // First make the Q set, with all infinity except the source
 for ( RNodeVec_it i = N.begin(); i != N.end(); i++)
   {
     if ((*i)->m_id == root)
       Q.insert(QPair_t(0, root));
     else
       Q.insert(QPair_t(INF, (*i)->m_id));
   }
 // Fill in all "NONE" in the next hop neighbors and predecessors
 NextHop.erase(NextHop.begin(), NextHop.end());
 Parent.erase(Parent.begin(), Parent.end());
 for (unsigned int i = 0; i < N.size(); i++)
   {
     NextHop.push_back(NODE_NONE);
     Parent.push_back(NODE_NONE);
   }

 while(Q.size() != 0)
   {
     Q_it u = Q.begin(); // Smallest
     if(0)printf("Smallest is node %ld dist %ld\n", u->second, u->first);
     S.insert(QPair_t(u->first, u->second)); // Add to S for later printout
     // Now relax each of the adjacent edges
     RNodeVec_it n;
     for (n = N.begin(); n != N.end() ;n++)
       {
         if ((*n)->m_id == u->second) break; // Found it
       }
     if (n == N.end())
       {
         printf("ERROR! Can't find node %ld\n", u->second);
         exit(1);
       }
     NodeWeight_t e(NODE_NONE, 0);
     while(1)
       { // Relax each adjacent node
         Q_it a;
         e = (*n)->NextAdj(e); // Next adj
         if (e.first == NODE_NONE) break; // Done..
         if(0)printf("Looking for adj node %ld\n", e.first);
         for (a = Q.begin(); a != Q.end(); a++)
           {
             if(0) printf("Searching,a->second %ld e.first %ld\n",
                      a->second,e.first);
             if (a->second == e.first) break; // Found it
           }
         if (a != Q.end())
           { // Not removed yet
             if(0)printf("Found it, edge weight is %d\n", e.second);
             dist_t d = u->first + e.second; // New distance to the adj
             if (d < a->first)
               { // Found new best path, remove and re-insert
                 // Update the next hop neighbor vector
                 if (u->second == root)
                   { // First hop
                     if(0)printf("Assuming first hop %ld\n",u->second);
                     NextHop[a->second] = a->second;
                   }
                 else
                   { // Else hop to new one through same one to me
                     NextHop[a->second] = NextHop[u->second];
                   }
                 // And re-insert new smaller dist into Q set
                 nodeid_t nid = a->second;
                 if(0)printf("Found new best d to node %ld dist %ld\n",nid, d);
                 if(0)printf("u->first %ld e->m_w %d\n", u->first, e.second);
                 Q.erase(a);
                 Q.insert(QPair_t(d, nid));
                 // And set parent
                 Parent[nid] = u->second;
               }
           }
       }
     Q.erase(u);
   }
  for (Q_it q = S.begin(); q != S.end(); q++)
    printf("Node %ld dist %ld\n", q->second, q->first);
}
#endif

#ifdef TEST_DIJ
RNodeVec_t Nodes;

int main()
{
Node N0(0);
Node N1(1);
Node N2(2);
Node N3(3);
Node N4(4);
RoutingVec_t NextHop;
RoutingVec_t Parent;

 N0.AddAdj(1, 10);
 N0.AddAdj(2, 5);

 N1.AddAdj(3, 1);
 N1.AddAdj(2, 2);

 N2.AddAdj(4, 2);
 N2.AddAdj(1, 3);
 N2.AddAdj(3, 9);

 N3.AddAdj(4, 4);

 N4.AddAdj(0, 7);
 N4.AddAdj(3, 6);

 Nodes.push_back(&N0);
 Nodes.push_back(&N1);
 Nodes.push_back(&N2);
 Nodes.push_back(&N3);
 Nodes.push_back(&N4);

 for (nodeid_t i = 0; i < Nodes.size(); i++)
   { // Get shortest path for each root node
     printf("\nFrom root %ld\n", i);
     Dijkstra(Nodes, i, NextHop, Parent);
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
#endif /* STL */
