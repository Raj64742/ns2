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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/node.h,v 1.18 2000/05/11 23:43:18 klan Exp $
 *
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.

 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 10/98.
 */
 
#ifndef __node_h__
#define __node_h__

#define CHECKFREQ  1
#define WAITING 0
#define POWERSAVING_STATE 1
#define INROUTE 2
#define MAX_WAITING_TIME 11

#include "connector.h"
#include "object.h"
#include "phy.h"
#include "list.h"
#include "net-interface.h"
#include "energy-model.h"
#include "location.h"

class LinkHead;

LIST_HEAD(linklist_head, LinkHead); // declare list head structure 
/*
 * The interface between a network stack and the higher layers.
 * This is analogous to the OTcl Link class's "head_" object.
 * XXX This object should provide a uniform API across multiple link objects;
 * right now it is a placeholder.  See satlink.h for now.  It is declared in
 * node.h for now since all nodes have a linked list of LinkHeads.
 */
class AdaptiveFidelityEntity;
class SoftNeighborHandler;

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
	~Node();
	virtual int command(int argc, const char*const* argv);
	inline int address() { return address_;}
	inline int nodeid() { return nodeid_;}
	inline double energy() { return energy_model_->energy();}
	inline double initialenergy() { return energy_model_->initialenergy();}
	inline double energy_level1() { return energy_model_->level1();}
	inline double energy_level2() { return energy_model_->level2();}
	inline EnergyModel *energy_model() { return energy_model_; }
	inline Location *location() { return location_;}
	inline int sleep() { return sleep_mode_;}
	inline int state() { return state_;}
	inline float state_start_time() {return state_start_time_;}
	inline float max_inroute_time() {return max_inroute_time_;}

	inline int adaptivefidelity() {return adaptivefidelity_;}
	inline int powersaving() { return powersavingflag_;}
	inline int getneighbors() {return neighbor_list.neighbor_cnt_;}
	

	virtual void set_node_sleep(int);
	virtual void set_node_state(int);
       	//virtual void get_node_idletime();
	virtual void add_rcvtime(float t) {total_rcvtime_ += t;}
	virtual void add_sndtime(float t) {total_sndtime_ += t;}
	virtual void idle_energy_patch(float, float);

	int address_;

	int nodeid_; //for nam use

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

	static Node* get_node_by_address(nsaddr_t);

	void add_neighbor(u_int32_t);      // for adaptive fidelity
	void scan_neighbor();
	void start_powersaving();
protected:
	LIST_ENTRY(Node) entry;  // declare list entry structure
	EnergyModel *energy_model_;
	Location * location_;
      	int sleep_mode_;	// = 1: radio is turned off
	int state_;		// used for AFECA state 
	float state_start_time_; // starting time of one AFECA state
	float total_sleeptime_;  // total time of radio in off mode
       	float total_rcvtime_;	 // total time in receiving data
	float total_sndtime_;	 // total time in sending data
	int adaptivefidelity_;   // Is AFECA activated ?
	int powersavingflag_;    // Is BECA activated ?
	int namDefinedFlag_;    // Is nam defined ?
	float last_time_gosleep; // time when radio is turned off
	float max_inroute_time_; // maximum time that a node can remaining
				 // active 
	int maxttl_;		 // how long a node can keep its neighbor
				 // list. For AFECA only.

	// XXX this structure below can be implemented by ns's LIST

	struct neighbor_list_item {
		u_int32_t id;        // node id
		int       ttl;    // time-to-live
		neighbor_list_item *next; // pointer to next item
	};

	struct {
		int neighbor_cnt_;   // how many neighbors in this list
		neighbor_list_item *head; 
	} neighbor_list;

	SoftNeighborHandler *snh_;
       	AdaptiveFidelityEntity *afe_;

      	
};

class AdaptiveFidelityEntity : public Handler {

public:  
        Node *nid_;
	virtual void start();
	virtual void handle(Event *e);

	virtual void adapt_it();
	inline void set_sleeptime(float t) {sleep_time_ = t;}
	inline void set_sleepseed(float t) {sleep_seed_ = t;}
	AdaptiveFidelityEntity (Node *nid) {
		nid_ = nid;
	}


protected:

	Event intr;
	float  sleep_time_;
	float sleep_seed_;
	float  idle_time_;
};

class SoftNeighborHandler : public Handler {
public:
	virtual void start();
	virtual void handle(Event *e); 
	Node *nid_;

	SoftNeighborHandler(Node *nid) {
		nid_ = nid;
	}
protected:
	Event  intr;
	
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



