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
 * 
 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 10/98.
 *
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/node.cc,v 1.22 2000/08/30 18:54:03 haoboy Exp $
 */

#include <phy.h>
#include <wired-phy.h>
#include <address.h>
#include <node.h>
#include <random.h>

#include "god.h"

static class LinkHeadClass : public TclClass {
public:
	LinkHeadClass() : TclClass("Connector/LinkHead") {}
	TclObject* create(int, const char*const*) {
		return (new LinkHead);
	}
} class_link_head;

LinkHead::LinkHead() : net_if_(0), node_(0), type_(0) { }

int32_t LinkHead::label() 
{
	if (net_if_)
		return net_if_->intf_label();
	printf("Configuration error:  Network Interface missing\n");
	exit(1);
	// Make msvc happy
	return 0;
}

int LinkHead::command(int argc, const char*const* argv)
{
        if (argc == 3) {
		if(strcmp(argv[1], "setnetinf") == 0) {
			net_if_ =
				(NetworkInterface*) TclObject::lookup(argv[2]);
			if (net_if_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "setnode") == 0) {
			node_ = (Node*) TclObject::lookup(argv[2]); 
			if (node_ == 0)
				return TCL_ERROR;
			return TCL_OK;
		}
	}
	return (Connector::command(argc, argv));
}

static class NodeClass : public TclClass {
public:
	NodeClass() : TclClass("Node") {}
	TclObject* create(int, const char*const*) {
                return (new Node);
        }
} class_node;

struct node_head Node::nodehead_ = { 0 }; // replaces LIST_INIT macro

char Node::nwrk_[NODE_NAMLOG_BUFSZ];

Node::Node() : 
	address_(-1), nodeid_ (-1), namChan_(0), 
	energy_model_(NULL), sleep_mode_(0), 
	total_sleeptime_(0), total_rcvtime_(0), total_sndtime_(0), 
	adaptivefidelity_(0), powersavingflag_(0), 
	last_time_gosleep(0),max_inroute_time_(300), maxttl_(5), node_on_(true)
{
	LIST_INIT(&ifhead_);
	LIST_INIT(&linklisthead_);
	insert(&(Node::nodehead_)); // insert self into static list of nodes

	neighbor_list.neighbor_cnt_ = 0;
	neighbor_list.head = NULL;
}

Node::~Node()
{
	LIST_REMOVE(this, entry);
}

int
Node::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if(strcmp(argv[1], "address?") == 0) {
			tcl.resultf("%d", address_);
 			return TCL_OK;
		} else if(strcmp(argv[1], "powersaving") == 0) {
			powersavingflag_ = 1;
			start_powersaving();
			return TCL_OK;
		} else if(strcmp(argv[1], "adaptivefidelity") == 0) {
			adaptivefidelity_ = 1;
			powersavingflag_ = 1;
			start_powersaving();
			return TCL_OK;
		} else if (strcmp(argv[1], "energy") == 0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%f", energy());
			return TCL_OK;
		} else if (strcmp(argv[1], "adjustenergy") == 0) {
			// assume every 10 sec schedule and 1.15 W 
			// idle energy consumption. needs to be
			// parameterized.
			idle_energy_patch(10, 1.15);
			total_sndtime_ = 0;
			total_rcvtime_ = 0;
			total_sleeptime_ = 0;
			return TCL_OK;
		} else if (strcmp(argv[1], "on") == 0) {
			node_on_ = true;
			tcl.evalf("%s set netif_(0)", name_);
			char *str = tcl.result();
			tcl.evalf("%s NodeOn", str);
			God::instance()->ComputeRoute();
			return TCL_OK;
		} else if (strcmp(argv[1], "off") == 0) {
			node_on_ = false;
			tcl.evalf("%s set netif_(0)", name_);
			char *str = tcl.result();
			tcl.evalf("%s NodeOff", str);

			tcl.evalf("%s set ragent_", name_);
			str = tcl.result();
			tcl.evalf("%s reset-state", str);

			God::instance()->ComputeRoute();
		     	return TCL_OK;
		}
	} else if (argc == 3) {
		if(strcmp(argv[1], "addif") == 0) {
			WiredPhy* phyp = (WiredPhy*)TclObject::lookup(argv[2]);
			if(phyp == 0)
				return TCL_ERROR;
			phyp->insertnode(&ifhead_);
			phyp->setnode(this);
			return TCL_OK;
		} else if (strcmp(argv[1], "addr") == 0) {
			address_ = Address::instance().str2addr(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "nodeid") == 0) {
			nodeid_ = atoi(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "addenergymodel") == 0) {
			energy_model_=(EnergyModel*)TclObject::lookup(argv[2]);
			if(!energy_model_)
				return TCL_ERROR;
			return TCL_OK;
		} else if(strcmp(argv[1], "addlinkhead") == 0) {
			LinkHead* slhp = (LinkHead*)TclObject::lookup(argv[2]);
			if (slhp == 0)
				return TCL_ERROR;
			slhp->insertlink(&linklisthead_);
			return TCL_OK;
		} else if (strcmp(argv[1], "setsleeptime") == 0) {
			afe_->set_sleeptime(atof(argv[2]));
			afe_->set_sleepseed(atof(argv[2]));
			return TCL_OK;
		} else if (strcmp(argv[1], "setenergy") == 0) {
			energy_model_->setenergy(atof(argv[2]));
			return TCL_OK;
		} else if (strcmp(argv[1], "settalive") == 0) {
			max_inroute_time_ = atof(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "maxttl") == 0) {
			maxttl_ = atoi(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "namattach") == 0) {
                        int mode;
                        namChan_ = Tcl_GetChannel(tcl.interp(), (char*)argv[2],
                                                  &mode);
                        if (namChan_ == 0) {
                                tcl.resultf("node: can't attach %s", argv[2]);
                                return (TCL_ERROR);
                        }
                        return (TCL_OK);
                }
	} else if (argc == 4) {
		if (strcmp(argv[1], "idleenergy") == 0) {
			idle_energy_patch(atof(argv[2]),atof(argv[3]));
			return TCL_OK;
		}
	}
	return TclObject::command(argc,argv);
}

void Node::namlog(const char* fmt, ...)
{
	// Don't do anything if we don't have a log file.
	if (namChan_ == 0) 
		return;
	va_list ap;
	va_start(ap, fmt);
	vsprintf(nwrk_, fmt, ap);
	namdump();
}

void Node::namdump()
{
        int n = 0;
        /* Otherwise nwrk_ isn't initialized */
	n = strlen(nwrk_);
	if (n >= NODE_NAMLOG_BUFSZ-1) {
		fprintf(stderr, 
			"Node::namdump() exceeds buffer size. Bail out.\n");
		abort();
	}
	if (n > 0) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		nwrk_[n] = '\n';
		nwrk_[n + 1] = 0;
		(void)Tcl_Write(namChan_, nwrk_, n + 1);
		nwrk_[n] = 0;
	}
}

// Given an interface label for a NetworkInterface on this node, we return 
// the head of that link
NsObject* Node::intf_to_target(int32_t label)
{
	LinkHead *lhp = linklisthead_.lh_first;
	for (; lhp; lhp = lhp->nextlinkhead()) 
		if (label == lhp->label())
			return ((NsObject*) lhp);
	return NULL;
}

void 
Node::start_powersaving()
{
	snh_ = new SoftNeighborHandler(this);
	snh_->start();
	
	afe_ = new AdaptiveFidelityEntity(this);
	afe_->start();

	state_ = POWERSAVING_STATE;
	state_start_time_ = Scheduler::instance().clock();
}

void
Node::set_node_sleep(int status)
{
	Tcl& tcl=Tcl::instance();
	//static float last_time_gosleep;
	// status = 1 to set node into sleep mode
	// status = 0 to put node back to idle mode.
	// time in the sleep mode should be used as credit to idle 
	// time energy consumption
	if (status) {
		last_time_gosleep = Scheduler::instance().clock();
		//printf("id=%d : put node into sleep at %f\n",
		// address_,last_time_gosleep);
		sleep_mode_ = status;
		// Although add-mark{} won't have any problem if nam trace is 
		// not turned on (tcl/lib/ns-namsupp.tcl), this conditional 
		// may save running time.
		if (namChan_)
			tcl.evalf("%s add-mark m1 blue hexagon", name());
	} else {
		sleep_mode_ = status;
		if (namChan_)
			tcl.evalf("%s delete-mark m1",name()); 
		//printf("id= %d last_time_sleep = %f\n",
		// address_, last_time_gosleep);
		if (last_time_gosleep) {
			total_sleeptime_ += Scheduler::instance().clock() -
				last_time_gosleep;
			last_time_gosleep = 0;
		}
	}	
}

void
Node::set_node_state(int state)
{
	switch (state_) { 
	case POWERSAVING_STATE:
	case WAITING:
		state_ = state;
		state_start_time_ = Scheduler::instance().clock();
		break;
	case INROUTE:
		if (state == POWERSAVING_STATE) {
			state_ = state;
		}
		if (state == INROUTE) {
			// a data packet is forwarded, needs to reset 
			// state_start_time_
			state_start_time_= Scheduler::instance().clock();
		}
		break;
	default:
		printf("Wrong state, quit...\n");
		abort();
	}
}

void
Node::idle_energy_patch(float /*total*/, float /*P_idle*/)
{
}

void 
Node::add_neighbor(u_int32_t nodeid)
{
	neighbor_list_item *np;
       
	np = neighbor_list.head;
	
	for (; np; np = np->next) {
		if (np->id == nodeid) {
			np->ttl = maxttl_;
			break;
		}
	}

	if (!np) {      // insert this new entry
	    np = new neighbor_list_item;
	    np->id = nodeid;
	    np->ttl = maxttl_;
	    np->next = neighbor_list.head;
	    neighbor_list.head = np;
	    
	    neighbor_list.neighbor_cnt_++;
	}
}

void
Node::scan_neighbor()
{
	neighbor_list_item *np, *lp;
	if (neighbor_list.neighbor_cnt_ > 0) {
		lp = neighbor_list.head;
		np = lp->next;
		for (; np; np = np->next) {
			np->ttl--;
			if (np->ttl <= 0){
				lp->next = np->next;
				delete np;
				np = lp;
				neighbor_list.neighbor_cnt_--;
			} 
			lp = np;
		}
		// process the first element
		np = neighbor_list.head;
		np->ttl--;
		if (np->ttl <= 0) {
			neighbor_list.head = np->next;
			delete np;
			neighbor_list.neighbor_cnt_--;
		}
	}
}

void
SoftNeighborHandler::start()
{
	Scheduler::instance().schedule(this, &intr, CHECKFREQ);
}

void
SoftNeighborHandler::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	nid_->scan_neighbor();
	s.schedule(this, &intr, CHECKFREQ);
}

void 
AdaptiveFidelityEntity::start()
{
	sleep_time_ = 2;
	sleep_seed_ = 2;
	idle_time_ = 10;
	nid_->set_node_sleep(0);
	Scheduler::instance().schedule(this, &intr, 
				       Random::uniform(0, idle_time_));
}

void 
AdaptiveFidelityEntity::handle(Event *)
{
	Scheduler &s = Scheduler::instance();
	int node_state = nid_->state();
	//printf("Node %d at State %d at time %f is in sleep %d \n", 
	// nid_->address(), node_state, s.clock(), nid_->sleep());
	//printf("node id = %d: sleep_time=%f\n",nid_->address(),sleep_time_);
	switch (node_state) {
	case POWERSAVING_STATE:
		if (nid_->sleep()) {
			// node is in sleep mode, wake it up
			nid_->set_node_sleep(0);
			adapt_it();
			s.schedule(this, &intr, idle_time_);
		} else {
			// node is in idle mode, put it into sleep
			nid_->set_node_sleep(1);
			adapt_it();
			s.schedule(this, &intr, sleep_time_);
		}
		break;
	case INROUTE:
		// 100s is the maximum INROUTE time.
		if (s.clock()-(nid_->state_start_time()) < 
		    nid_->max_inroute_time()) {
			s.schedule(this, &intr, idle_time_);
		} else {
			nid_->set_node_state(POWERSAVING_STATE);
			adapt_it();
			nid_->set_node_sleep(1);
			s.schedule(this, &intr, sleep_time_); 
		}
		break;
	case WAITING:
		// 10s is the maximum WAITING time
		if (s.clock()-(nid_->state_start_time()) < MAX_WAITING_TIME) {
			s.schedule(this, &intr, idle_time_);
		} else {
			nid_->set_node_state(POWERSAVING_STATE);
			adapt_it();
			nid_->set_node_sleep(1);
			s.schedule(this, &intr, sleep_time_); 
		}
		break;
	default:
		fprintf(stderr, "Illegal Node State!");
		abort();
	}
}

void 
AdaptiveFidelityEntity::adapt_it()
{
	float delay;
	// use adaptive fidelity
	if (nid_->adaptivefidelity()) {
		int neighbors = nid_->getneighbors();
		if (!neighbors) 
			neighbors = 1;
		delay = sleep_seed_*Random::uniform(1,neighbors); 
	      	set_sleeptime(delay);
	}
}

// return the node instance from the static node list
Node* Node::get_node_by_address (nsaddr_t id)
{
	Node * tnode = nodehead_.lh_first;
	for (; tnode; tnode = tnode->nextnode()) {
		if (tnode->address_ == id ) {
			return (tnode);
		}
	}
	return NULL;
}
