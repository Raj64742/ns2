/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999 Regents of the University of California.
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
 *      This product includes software developed by the MASH Research
 *      Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * Contributed by Tom Henderson, UCB Daedalus Research Group, June 1999
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/satnode.cc,v 1.2 1999/06/23 23:41:57 tomh Exp $";
#endif

#include "satnode.h"
#include "satlink.h"
#include "sattrace.h"
#include "sathandoff.h"

static class SatNodeClass : public TclClass {
public:
	SatNodeClass() : TclClass("Node/SatNode") {}
	TclObject* create(int , const char*const* ) {
		return (new SatNode);
	}
} class_satnode;

int SatNode::dist_routing_ = 0;

SatNode::SatNode() : ragent_(0), trace_(0), hm_(0)  
{
	bind_bool("dist_routing_", &dist_routing_);
}

int SatNode::command(int argc, const char*const* argv) {     
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "set_downlink") == 0) {
			if (downlink_ != NULL) {
				tcl.result(downlink_->name());
				return (TCL_OK);
			}
		} else if (strcmp(argv[1], "set_uplink") == 0) {
			if (downlink_ != NULL) {
				tcl.result(uplink_->name());
				return (TCL_OK);
			}
		} else if (strcmp(argv[1], "start_handoff") == 0) {
			if (hm_)
				hm_->start();
			else {
				printf("Error: starting non-existent ");
				printf("handoff mgr\n");
				exit(1);
			}
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "set_uplink") == 0) {
			uplink_ = (SatChannel *) TclObject::lookup(argv[2]);
			if (uplink_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set_downlink") == 0) {
			downlink_ = (SatChannel *) TclObject::lookup(argv[2]);
			if (downlink_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set_trace") == 0) {
			trace_ = (SatTrace *) TclObject::lookup(argv[2]);
			if (trace_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set_ragent") == 0) {
			ragent_ = (SatRouteAgent *) TclObject::lookup(argv[2]);
			if (ragent_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if(strcmp(argv[1], "addif") == 0) {
                        SatPhy* n = (SatPhy*) TclObject::lookup(argv[2]);
                        if(n == 0)
                                return TCL_ERROR; 
                        n->insertnode(&ifhead_);
                        n->setnode(this);
                        return TCL_OK;
		} else if (strcmp(argv[1], "set_position") == 0) {
			pos_ = (SatPosition*) TclObject::lookup(argv[2]);
			if (pos_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set_handoff_mgr") == 0) {
			hm_ = (LinkHandoffMgr*) TclObject::lookup(argv[2]);
			if (hm_ == 0) {
				tcl.resultf("no such object %s", argv[2]);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}

	}
	return (Node::command(argc, argv));
}

