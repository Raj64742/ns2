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

#ifdef HAVE_STL
#ifdef NIXVECTOR


#include <stdio.h>

#include "routealgo/rnode.h"

// Forward declarations

static void PrintPred(nodeid_t, nodeid_t, RoutingVec_t&);
static void NixPred(nodeid_t, nodeid_t, RoutingVec_t&, RNodeVec_t&, NixVec&);

// Globally visible routines

void PrintRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred)
{ // Print the route, given source s, destination d and predecessor vector pred
  printf("Route to node %ld is : ", d);
  PrintPred(s, d, pred);
  printf("\n");
}

// Create the NixVector
void NixRoute( nodeid_t s, nodeid_t d, RoutingVec_t& pred,
               RNodeVec_t& nodes, NixVec& nv)
{
  NixPred(s, d, pred, nodes, nv);
}


// Print the parent list (debug)
void PrintParents(RoutingVec_t& pred)
{
  printf("Parent vector is");
  for (RoutingVec_it i = pred.begin(); i != pred.end(); i++)
    {
      printf(" %ld", *i);
    }
  printf("\n");
}

// Print the route from the nixvector (debug)
void PrintNix(nodeid_t s, RNodeVec_t nodes, NixVec& nv)
{
  printf("Printing Nv from %ld\n", s);
  while(1)
    {
      printf("%ld\n", s);
      fflush(stdout);
      if (!nodes[s])
        {
          printf("PrintNix Error, no node for %ld\n", s);
          break;
        }
      printf("Node %ld nixl %ld\n", s, nodes[s]->GetNixl());
      Nix_t n = nv.Extract(nodes[s]->GetNixl());
      if (n == NIX_NONE) break; // Done
      nodeid_t s1 = nodes[s]->GetNeighbor(n);
      if (s1 == NODE_NONE)
        {
          printf("Huh?  Node %ld can't get neighbor %ld\n", s, n);
          break;
        }
      s = s1;
    }
  printf("\n");
  nv.Reset();
}



// helpers
static void PrintPred(nodeid_t s, nodeid_t p, RoutingVec_t& pred)
{
  if (s == p)
    {
      printf(" %ld", s);
    }
  else
    {
      if (pred[p] == NODE_NONE)
        {
          printf(" No path..");
        }
      else
        {
          PrintPred(s, pred[p], pred);
          printf(" %ld", p);
        }
    }
}

static void NixPred(nodeid_t s, nodeid_t p, RoutingVec_t& pred,
                    RNodeVec_t& nodes, NixVec& nv)
{
  if (s == p)
    {
      return; // Got there
    }
  else
    {
      if (pred[p] == NODE_NONE)
        {
          if(0)printf(" No path..");
          return;
        }
      else
        {
          NixPred(s, pred[p], pred, nodes, nv);
          NixPair_t n = nodes[pred[p]]->GetNix(p);
          if (n.first == NIX_NONE)
            { // ! Error.
              printf("RouteAlgo::GetNix returned NIX_NONE!\n");
              exit(1);
            }
          if(0)printf("NVAdding ind %ld lth %ld\n", n.first, n.second);
          nv.Add(n);
        }
    }
}

#endif /* NIXVECTOR */
#endif /* STL */
