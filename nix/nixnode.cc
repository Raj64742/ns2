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
 * Nix-Vector routing capable node
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 *
 */
#ifdef HAVE_STL
#ifdef NIXVECTOR

// STL includes
#include <vector>

#include "nix/nixnode.h"
#include "routealgo/bfs.h"
#include "nix/nixvec.h"
#include "routealgo/routealgo.h"
#include "nix/hdr_nv.h"

static RNodeVec_t Nodes; // Vector of known nodes
static int NVCount = 0;  // Number of nix vectors
static Nixl_t NVMin = 0;    // Smallest nv
static Nixl_t NVMax = 0;    // Largest nv
static Nixl_t NVTot = 0;    // Total bitcount for all nv's (to compute avg)

NixNode::NixNode() : RNode(), m_Map(-1), m_pNixVecs(0)
{
	if(0)printf("Hello from NixNode Constructor\n");
  Nodes.push_back(this); // And save it

}

#ifdef MOVED_TO_NODE
int NixNode::command(int argc, const char*const* argv)
{
	if(0)printf("Hello from NixNode::Command argc %d argv[0] %s argv[1] %s\n", 
				 argc, argv[0], argv[1]);
  if (argc == 2)
    {
      Tcl& tcl = Tcl::instance();
      if(strcmp(argv[1], "address?") == 0)
				{
					tcl.resultf("%d", address_);
					return TCL_OK;
				}
      if(strcmp(argv[1], "report-id") == 0)
				{
					printf("Id is %ld\n", m_id);
					return TCL_OK;
				}
      if(strcmp(argv[1], "populate-objects") == 0)
				{
					PopulateObjects();
					return TCL_OK;
				}
    }
	if (argc == 3)
		{
      if (strcmp(argv[1], "get-nix-vector") == 0)
        {
					GetNixVector(atol(argv[2]));
					return TCL_OK;
				}
      if (strcmp(argv[1], "set-neighbor") == 0)
        {
          if(0)printf("Setting neighbor node %ld neighbor %s\n",
                 m_id, argv[2]);
          AddAdj(atol(argv[2]));
          return(TCL_OK);
        }
      if (strcmp(argv[1], "map-to") == 0)
        {
          m_Map = atol(argv[2]);
          return(TCL_OK);
        }
		}
  return Node::command(argc,argv);
}
#endif

void NixNode::AddAdj( // Add a new neighbor bit
    nodeid_t WhichN)
{    
  Edge* pE;

  if (0)
    {
      printf("Node %ld adding neighbor %ld\n", m_id, WhichN);
      fflush(stdout);
    }
  pE= new Edge(WhichN);
  m_Adj.push_back(pE);
}

int NixNode::IsNeighbor( // TRUE neighbor bit set
    nodeid_t WhichN)
{    
  for (EdgeVec_it i = m_Adj.begin(); i != m_Adj.end(); i++)
    {
      if ((*i)->m_n == WhichN) return(1); // Found it
    }
  return(0);
}

const NodeWeight_t NixNode::NextAdj( const NodeWeight_t& last)
{
Edge* pE;

  static EdgeVec_it prev;
  if (last.first == NODE_NONE)
    {
      prev = m_Adj.begin();
      if (prev == NULL) // ! How can this happen?
        return(NodeWeight_t(NODE_NONE, 0));
      pE = *prev;
      if(0)printf("NextAdj returning first edge %ld\n",
             pE->m_n);
      return(NodeWeight_t(pE->m_n, 1));
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
                     pE->m_n, 1);
              return(NodeWeight_t(pE->m_n, 1));
            }
        }
      else
        { // Need to code this
          printf("Non-linear advance of NextAdj\n");
          exit(5);
        }
    }
}

NixVec* NixNode::ComputeNixVector(nodeid_t t)
{ // Compute the NixVector to a target
RoutingVec_t Parent;
RoutingVec_t NextHop;

  if(0)printf("Computing nixvector from %ld to %ld\n", m_id,  t); 
  BFS(Nodes, m_id, NextHop, Parent);
  NixVec* pNv = new NixVec;
  NixRoute(m_id, t, Parent, Nodes, *pNv);
  return pNv;
}

NixPair_t NixNode::GetNix(nodeid_t t)  // Get neighbor index/length
{
  if(0)printf("Node %ld Getnix to target %ld, adjsize %d\n",
         m_id, t, m_Adj.size());
  Nixl_t l = GetNixl();
  for (unsigned long i = 0; i < m_Adj.size(); i++)
    {
      if(0)printf("Node %ld checking nb %ld\n", m_id, m_Adj[i]->m_n);
      if (m_Adj[i]->m_n == t)
        return(NixPair_t(i, l));  // Found it
    }
  return(NixPair_t(NIX_NONE, 0)); // Did not find it
}

Nixl_t NixNode::GetNixl()         // Get bits needed for nix entry
{
  return NixVec::GetBitl(m_Adj.size() - 1);
}

nodeid_t NixNode::GetNeighbor( Nix_t n, NixVec* pNv)
{ // Reconstruct a neighbor from the neighbor index
	if (n >= m_Adj.size())
		{ // Foulup of some sort, print stuff out and abort
			printf("Nix %ld out of range (0 - %d\n", n, m_Adj.size());
			pNv->DBDump();
			exit(0);
		}
  return(m_Adj[n]->m_n);
}


NsObject* NixNode::GetNsNeighbor(Nix_t n)   // Get the ns nexthop neighbor
{
	if (n >= m_AdjObj.size()) return(NULL);   // Out of range
  return(m_AdjObj[n]);
}

NixVec* NixNode::GetNixVector(nodeid_t t) // Get a nix vector for a target
{
NVMap_it i;

 if (!m_pNixVecs)
	 { // No current list
		 m_pNixVecs = new NVMap_t;
	 }
 
 i = m_pNixVecs->find(t);
 if (i == m_pNixVecs->end())
	 { // Does not exist, compute it and add to the hash-map
		 NixVec* pNv = ComputeNixVector(t);
		 // Debug statistics follow
		 if (NVCount == 0)
			 { // First one
				 NVMin = pNv->ALth();
				 NVMax = pNv->ALth();
			 }
		 else
			 {
				 NVMin = (pNv->ALth() < NVMin) ? pNv->ALth() : NVMin;
				 NVMax = (pNv->ALth() > NVMax) ? pNv->ALth() : NVMax;
			 }
		 NVCount++;
		 NVTot += pNv->ALth();
		 // End debug stats
#ifdef TRY_DIFFERENT
		 m_pNixVecs[(const nodeid_t)t] = (const NixVec const *)pNv;
#else
		 NVPair_t p = NVPair_t(t, pNv);
                 //const pair <const nodeid_t, NixVec*> p1 = new pair<const nodeid_t, NixVec*>(t,pNv);
		 m_pNixVecs->insert(p);
#endif
		 pNv->Reset();
		 // debug follows
#ifdef DEBUG_VERBOSE		
		 printf("Nixvec from %ld to %ld\n", m_id, t);
		 pNv->DBDump();
#endif
		 return(pNv); // Return a the vector
	 }
 (*i).second->Reset();
 return((*i).second); // Return the vector
}

void NixNode::PopulateObjects(void)
{
Edge*      pEdge;
EdgeVec_it it;
Tcl& tcl = Tcl::instance();

  for (it = m_Adj.begin(); it != m_Adj.end(); it++)
		{
			pEdge = *it;
			if(0)printf("Node %ld processing edge %ld\n", m_id, pEdge->m_n);
			tcl.evalf("[Simulator instance] get-link-head %d %d", m_id, pEdge->m_n);
			NsObject* pHead = (NsObject*)TclObject::lookup(tcl.result());
			if(0)printf("Found head at %s\n", pHead->name());
			m_AdjObj.push_back(pHead); // And add it to vector
			if(0)tcl.evalf("%s info class", pHead->name());
			if(0)printf("Class is %s\n", tcl.result());
		} 
}

// Static functions
NixNode*   NixNode::GetNodeObject(nodeid_t n) // Get a node obj. based on id
{
	if (n >= Nodes.size()) return(NULL); // Does not exist
	return((NixNode*)Nodes[n]);
}

void NixNode::PopulateAllObjects(void)
{  // Called after $ns run, populates the next hop NS objects
NixNode*    pN;
RNodeVec_it it;

  for (it = Nodes.begin(); it != Nodes.end(); it++)
		{
			pN = (NixNode*)*it;
			pN->PopulateObjects();
		}
}

// Global function (debug only)
void ReportNixStats()
{
	printf("Total NV %d Min %ld Max %ld Avg %f\n", 
				 NVCount, NVMin, NVMax, (double)NVTot/(double)NVCount);
}

#endif /* NIXVECTOR */
#endif /* STL */
