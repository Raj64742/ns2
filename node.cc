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
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.
 * CMU-Monarch project's Mobility extensions ported by Padma Haldar, 
 * 10/98.
 */

#include <phy.h>
#include <wired-phy.h>
#include <address.h>
#include <node.h>

static class LinkHeadClass : public TclClass {
public:
	LinkHeadClass() : TclClass("Connector/LinkHead") {}
	TclObject* create(int, const char*const*) {
		return (new LinkHead);
	}
} class_link_head;

LinkHead::LinkHead() : net_if_(0), node_(0), type_(0) { }

int32_t LinkHead::label() {
		if (net_if_)
				return net_if_->intf_label();
		printf("Configuration error:  Network Interface missing\n");
		exit(1);
		// Make msvc happy
		return 0;
}

int LinkHead::command(int argc, const char*const* argv)
{
        //Tcl& tcl = Tcl::instance();
        if (argc == 2) {
        } else if (argc == 3) {
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

Node::Node(void) : address_(-1)
{
	LIST_INIT(&ifhead_);
	LIST_INIT(&linklisthead_);
	insert(&(Node::nodehead_)); // insert self into static list of nodes
	
}

int
Node::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		if(strcmp(argv[1], "address?") == 0) {
			tcl.resultf("%d", address_);
 			return TCL_OK;
		}
	}
	if (argc == 3) {
		if(strcmp(argv[1], "addif") == 0) {
			WiredPhy* phyp = (WiredPhy*) TclObject::lookup(argv[2]);
			if(phyp == 0)
				return TCL_ERROR;
			phyp->insertnode(&ifhead_);
			phyp->setnode(this);
			return TCL_OK;
		} else if (strcmp(argv[1], "addr") == 0) {
			address_ = Address::instance().\
				str2addr(argv[2]);
			return TCL_OK;
		} else if(strcmp(argv[1], "addlinkhead") == 0) {
			LinkHead* slhp = (LinkHead*) TclObject::lookup(argv[2]);
			if (slhp == 0)
				return TCL_ERROR;
			slhp->insertlink(&linklisthead_);
			return TCL_OK;
		}
	}
	return TclObject::command(argc,argv);
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


#ifdef zero	
static class HierNodeClass : public TclClass {
public:
	HierNodeClass() : TclClass("HierNode") {}
	TclObject* create(int, const char*const*) {
                return (new HierNode);
        }
} class_hier_node;

static class ManualRtNodeClass : public TclClass {
public:
	ManualRtNodeClass() : TclClass("ManualRtNode") {}
	TclObject* create(int, const char*const*) {
                return (new ManualRtNode);
        }
} class_ManualRt_node;

static class VirtualClassifierNodeClass : public TclClass {
public:
	VirtualClassifierNodeClass() : TclClass("VirtualClassifierNode") {}
	TclObject* create(int, const char*const*) {
                return (new VirtualClassifierNode);
        }
} class_VirtualClassifier_node;
#endif
