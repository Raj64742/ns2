/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/node.h,v 1.9 1999/06/21 18:13:59 tomh Exp $
 *
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.

 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 10/98.
 */
 
#ifndef __node_h__
#define __node_h__

#include "connector.h"
#include "object.h"
#include "phy.h"
#include "list.h"
#include "net-interface.h"

class LinkHead;
LIST_HEAD(linklist_head, LinkHead); // declare list head structure 
/*
 * The interface between a network stack and the higher layers.
 * This is analogous to the OTcl Link class's "head_" object.
 * XXX This object should provide a uniform API across multiple link objects;
 * right now it is a placeholder.  See satlink.h for now.  It is declared in
 * node.h for now since all nodes have a linked list of LinkHeads.
 */
class Node;
class NetworkInterface;
class LinkHead : public Connector {
public:
	LinkHead(); 

	// API goes here
	Node* node() { return node_; }
	int type() { return type_; }
	int32_t label();
	// Future API items could go here 

	// list of all networkinterfaces on a node
	inline void insertlink(struct linklist_head *head) {
                LIST_INSERT_HEAD(head, this, link_entry_);
        }
        LinkHead* nextlinkhead(void) const { return link_entry_.le_next; }

protected:
	virtual int command(int argc, const char*const* argv);
	NetworkInterface* net_if_; // Each link has a Network Interface
	Node* node_; // Pointer to node object
	int type_; // Type of link
	// Each node has a linked list of link heads
	LIST_ENTRY(LinkHead) link_entry_;

};

LIST_HEAD(node_head, Node); // declare list head structure 

// srm-topo.h already uses class Node- hence the odd name "Node"
//
// For now, this is a dummy split object for class Node. In future, we
// may move somethings over from OTcl.
//

class Node : public TclObject {
 public:
	Node(void);
	virtual int command(int argc, const char*const* argv);
	inline int address() { return address_;}
	int address_;

	// XXX can we deprecate the next line (used in MobileNode)
	// in favor of accessing phys via a standard link API?
	struct if_head ifhead_; // list of phys for this node
	struct linklist_head linklisthead_; // list of link heads on node

	NsObject* intf_to_target(int32_t); 

	// The remaining objects handle a (static) linked list of nodes
	static struct node_head nodehead_;  // static head of list of nodes
	inline void insert(struct node_head* head) {
		LIST_INSERT_HEAD(head, this, entry);
	}
	inline Node* nextnode() { return entry.le_next; }
protected:
	LIST_ENTRY(Node) entry;  // declare list entry structure
	
};

#ifdef zero
class HierNode : public Node {
public:
	HierNode() {}    
};

class ManualRtNode : public Node {
public:
	ManualRtNode() {}    
};

class VirtualClassifierNode : public Node {
public:
	VirtualClassifierNode() {}    
};
#endif

#endif /* __node_h__ */



