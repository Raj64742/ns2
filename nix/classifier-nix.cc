/* -*-	Mode:C++; c-basic-offset:2; tab-width:2; indent-tabs-mode:f -*- */
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
 * NixVector classifier for stateless routing
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 *
 */

#ifdef HAVE_STL
#ifdef NIXVECTOR


#include <stdio.h>

#include "ip.h"
#include "nixvec.h"
#include "classifier-nix.h"
#include "hdr_nv.h"

/* Define the TCL Class */
static class NixClassifierClass : public TclClass {
public:
	NixClassifierClass() : TclClass("Classifier/Nix") {}
	TclObject* create(int, const char*const*) {
		return (new NixClassifier());
	}
} class_nix_classifier;



/****************** NixClassifier Methods ************/
NsObject* NixClassifier::find(Packet* p)
{
	if (m_NodeId == NODE_NONE)
		{
			printf("NixClassifier::find(), Unknown node id\n");
			return(NULL);
		}
	NixNode* pN = NixNode::GetNodeObject(m_NodeId);
	hdr_ip* ip = hdr_ip::access(p);
	if ((nodeid_t)ip->daddr() == pN->Id())
		{ // Arrived at destination, pass on to the dmux object
			if(0)printf("Returning Dmux objet %p\n", m_Dmux);
			return m_Dmux;
		}
	hdr_nv* nv = hdr_nv::access(p);
	NixVec* pNv = nv->nv();
	if (pNv == NULL)
		{
			printf("Error! NixClassifier %s called with no NixVector\n", name());
			return(NULL);
		}
	if (pN == NULL)
		{
			printf("NixClassifier::find(), can't find node id %ld\n", m_NodeId);
			return(NULL);
		}
	Nix_t Nix = pNv->Extract(pN->GetNixl(), nv->used()); // Get the next nix
  nodeid_t n = pN->GetNeighbor(Nix,pNv); // this is just for debug print
  NsObject* nsobj = pN->GetNsNeighbor(Nix);
	if(0)printf("Classifier %s, Node %ld next hop %ld (%s)\n",
				 name(), pN->Id(), n, nsobj->name());
	return(nsobj); // And return the nexthop head object
}

int NixClassifier::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1],"set-node-id") == 0)
		{
			m_NodeId = atoi(argv[2]);
			return(TCL_OK);
		}
	if (argc == 4 && strcmp(argv[1],"install") == 0)
		{
			int target = atoi(argv[2]);
			NixNode* pN = NixNode::GetNodeObject(m_NodeId);
			if (pN)
				{
					if(0)printf("%s NixClassifier::Command node %ld install %s %s\n",
								 name(), pN->Id(), argv[2], argv[3]);
					if ((nodeid_t)target == pN->Id())
						{ // This is the only case we care about, need to note the dmux obj
							m_Dmux = (NsObject*)TclObject::lookup(argv[3]);
							if(0)printf("m_Dmux obj is %p\n", m_Dmux);
						}
				}
			return(TCL_OK);
		}
	return (Classifier::command(argc, argv));
}
 
#endif /* NIXVECTOR */
#endif
