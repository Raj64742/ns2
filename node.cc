/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/node.cc,v 1.29 2001/02/01 22:56:21 haldar Exp $
 *
 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 10/98.
 */

#include <stdio.h>
#include <stdarg.h>

#include "address.h"
#ifdef NIXVECTOR
#include "nix/nixnode.h"
#endif /* NIXVECTOR */
#include "node.h"

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

/* Additions for NixRouting */
int NixRoutingUsed = -1;

Node::Node() : 
	address_(-1), nodeid_ (-1), namChan_(0),
	rtnotif_(NULL),
#ifdef NIXVECTOR
	nixnode_(NULL),
#endif /* NIXVECTOR */
	energy_model_(NULL), location_(NULL)
{
	LIST_INIT(&ifhead_);
	LIST_INIT(&linklisthead_);
	insert(&(Node::nodehead_)); // insert self into static list of nodes
#ifdef NIXVECTOR
	// Mods for Nix-Vector routing
	if (NixRoutingUsed < 0)	{
		// Find out if nix routing is in use
		Tcl& tcl = Tcl::instance();
		tcl.evalf("Simulator set nix-routing");
		tcl.resultAs(&NixRoutingUsed);
	}
	if (NixRoutingUsed) {
		// Create the NixNode pointer
		if(0)printf("Nix routing in use, creating NixNode\n");
		nixnode_ = new NixNode();
	}
#endif /* NIXVECTOR */
	neighbor_list_ = NULL;
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
#ifdef NIXVECTOR
		// Mods for Nix-Vector Routing
		if(strcmp(argv[1], "populate-objects") == 0) {
			if (nixnode_) {
				nixnode_->PopulateObjects();
			}
			return TCL_OK;
		}
		// End mods for Nix-Vector routing
#endif /* NIXVECTOR */
		if(strcmp(argv[1], "address?") == 0) {
			tcl.resultf("%d", address_);
 			return TCL_OK;
		}
			

	} else if (argc == 3) {
#ifdef NIXVECTOR
		// Mods for Nix-Vector Routing
		if (strcmp(argv[1], "get-nix-vector") == 0) {
			if (nixnode_) {
				nixnode_->GetNixVector(atol(argv[2]));
			}
			return TCL_OK;
		}
#endif /* NIXVECTOR */
		if (strcmp(argv[1], "set-neighbor") == 0) {
#ifdef NIXVECTOR
			if (nixnode_) {
				nixnode_->AddAdj(atol(argv[2]));
			}
#endif /* NIXVECTOR */
			return(TCL_OK);
		}
		if (strcmp(argv[1], "addr") == 0) {
			address_ = Address::instance().str2addr(argv[2]);
#ifdef NIXVECTOR
			if (nixnode_) {
				nixnode_->Id(address_);
			}
#endif /* NIXVECTOR */
			return TCL_OK;
		// End mods for Nix-Vector routing
		} else if (strcmp(argv[1], "nodeid") == 0) {
			nodeid_ = atoi(argv[2]);
			return TCL_OK;
		} else if(strcmp(argv[1], "addlinkhead") == 0) {
			LinkHead* slhp = (LinkHead*)TclObject::lookup(argv[2]);
			if (slhp == 0)
				return TCL_ERROR;
			slhp->insertlink(&linklisthead_);
			return TCL_OK;
		} else if (strcmp(argv[1], "addenergymodel") == 0) {
			energy_model_=(EnergyModel*)TclObject::lookup(argv[2]);
			if(!energy_model_)
				return TCL_ERROR;
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
		} else if (strcmp(argv[1], "add-neighbor") == 0) {
			Node * node = (Node *)TclObject::lookup(argv[2]);
 			if (node == 0) {
 				tcl.resultf("Invalid node %s", argv[2]);
                                 return (TCL_ERROR);
			}
			addNeighbor(node);
			return TCL_OK;
		}
		// this will go away when rtnotif_ becomes
		// a shared obj across c++/otcl boundary
		if(strcmp(argv[1], "route_notify") == 0) {
			RoutingModule * rtmod = ((RoutingModule *)TclObject::lookup(argv[2]));
			if (rtmod == 0) {
				tcl.resultf("Invalid rtmodule %s", argv[2]);
                                 return (TCL_ERROR);
			}
			add_to_rtnotif(rtmod);
			return(TCL_OK);
		}
		else if(strcmp(argv[1], "unreg_route_notify") == 0) {
			RoutingModule * rtmod = ((RoutingModule *)TclObject::lookup(argv[2]));
			if (rtmod == 0) {
				tcl.resultf("Invalid rtmodule %s", argv[2]);
				return (TCL_ERROR);
			}
			if (!rem_from_rtnotif(rtmod)) {
				tcl.resultf("Rtmodule %s not found",argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
	}
		
	return TclObject::command(argc,argv);
}

void Node::add_to_rtnotif(RoutingModule * rtmod) {
	rtm_node* node = (rtm_node *)malloc(sizeof(rtm_node));
	node->rtm = rtmod;
	node->next = rtnotif_;
	rtnotif_ = node;
}

int Node::rem_from_rtnotif(RoutingModule * rtmod) {
	rtm_node *tmp1 = rtnotif_;
	rtm_node *tmp2 ;
	while(tmp1) {
		if (tmp1->rtm == rtmod) {
			if (rtnotif_ == tmp1) {
				rtnotif_ = tmp1->next;
				free (tmp1);
				return 1;
			}
			else {
				tmp2->next = tmp1->next;
				free(tmp1);
				return 1;
			}
		}
		else {
			tmp2 = tmp1;
			tmp1 = tmp1->next;
		}
	}
	return 0;
}

void Node::addNeighbor(Node * neighbor) {

	neighbor_list_node* nlistItem = (neighbor_list_node *)malloc(sizeof(neighbor_list_node));
	nlistItem->nodeid = neighbor->nodeid();
	nlistItem->next = neighbor_list_;
	neighbor_list_=nlistItem; 
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

// A class static method. Return the node instance from the static node list
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
