/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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

/* All that is needed it to override the find method, to use the */
/* NixVector to find the next hop neighbor */

#ifndef __CLASSIFIER_NIX_H__
#define __CLASSIFIER_NIX_H__

#include "packet.h"
#include "classifier.h"
#include "nixvec.h"
#include "nixnode.h"

class NixClassifier : public Classifier {
public:
	NixClassifier() : Classifier(), m_NodeId(NODE_NONE), m_Dmux(0) { }
	virtual NsObject* find(Packet*);
	virtual int command(int argc, const char*const* argv);
public:
	nodeid_t m_NodeId; // If of associated node object
	NsObject* m_Dmux;  // Pointer to dmux object
};

#endif
