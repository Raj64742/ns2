/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- 
 *
 * Copyright (C) 2000 by USC/ISI
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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mpls/mpls-module.cc,v 1.3 2001/02/22 19:45:40 haldar Exp $
 *
 * MPLS node plugin module
 */

#include <tclcl.h>

#include "node.h"
#include "mpls/ldp.h"
#include "mpls/mpls-module.h"
#include "mpls/classifier-addr-mpls.h"

static class MPLSModuleClass : public TclClass {
public:
	MPLSModuleClass() : TclClass("RtModule/MPLS") {}
	TclObject* create(int, const char*const*) {
		return (new MPLSModule);
	}
} class_mpls_module;

MPLSModule::~MPLSModule() 
{
	// Delete LDP agent list
	LDPListElem *e;
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; ) {
		e = p;
		p = p->link.le_next;
		delete e;
	}
}

void MPLSModule::detach_ldp(LDPAgent *a)
{
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; 
	     p = p->link.le_next)
		if (p->agt_ == a) {
			LIST_REMOVE(p, link);
			delete p;
			break;
		}
}

LDPAgent* MPLSModule::exist_ldp(int nbr)
{
	for (LDPListElem *p = ldplist_.lh_first; p != NULL; 
	     p = p->link.le_next)
		if (p->agt_->peer() == nbr)
			return p->agt_;
	return NULL;
}

int MPLSModule::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "new-incoming-label") == 0) {
			tcl.resultf("%d", ++last_inlabel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "new-outgoing-label") == 0) {
			tcl.resultf("%d", --last_outlabel_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-ldp-agents") == 0) {
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				tcl.resultf("%s %s", tcl.result(), 
					    e->agt_->name());
			return (TCL_OK);
		} else if (strcmp(argv[1], "trace-ldp") == 0) {
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				e->agt_->turn_on_trace();
			return (TCL_OK);
		} else if (strcmp(argv[1], "trace-msgtbl") == 0) {
			printf("%d : message-table\n", node()->nodeid());
			for (LDPListElem *e = ldplist_.lh_first; e != NULL;
			     e = e->link.le_next)
				e->agt_->MSGTdump();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "attach-ldp") == 0) {
			LDPAgent *a = (LDPAgent*)TclObject::lookup(argv[2]);
			if (a == NULL) {
				fprintf(stderr,"Wrong object name %s",argv[2]);
				return (TCL_ERROR);
			}
			attach_ldp(a);
			return (TCL_OK);
		} else if (strcmp(argv[1], "detach-ldp") == 0) {
			LDPAgent *a = (LDPAgent*)TclObject::lookup(argv[2]);
			if (a == NULL) {
				fprintf(stderr,"Wrong object name %s",argv[2]);
				return (TCL_ERROR);
			}
			detach_ldp(a);
			return (TCL_OK);
		} else if (strcmp(argv[1], "exist-ldp-agent") == 0) {
			tcl.resultf("%d", 
				    exist_ldp(atoi(argv[2])) == NULL ? 0 : 1 );
			return (TCL_OK);
		} else if (strcmp(argv[1], "get-ldp-agent") == 0) {
			LDPAgent *a = exist_ldp(atoi(argv[2]));
			if (a == NULL)
				tcl.result("");
			else 
				tcl.resultf("%s", a->name());
			return (TCL_OK);
		}
		else if (strcmp(argv[1] , "route-notify") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Invalid node object %s", argv[2]);
				return TCL_ERROR;
			}
			if (node != n_) {
				tcl.add_errorf("Node object %s different from n_", argv[2]);
				return TCL_ERROR;
			}
			n_->route_notify(this);
			return TCL_OK;
		}
		if (strcmp(argv[1] , "unreg-route-notify") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Invalid node object %s", argv[2]);
				return TCL_ERROR;
			}
			if (node != n_) {
				tcl.add_errorf("Node object %s different from n_", argv[2]);
				return TCL_ERROR;
			}
			n_->unreg_route_notify(this);
			return TCL_OK;
		}
		else if (strcmp(argv[1], "attach-classifier") == 0) {
			classifier_ = (MPLSAddressClassifier*)(TclObject::lookup(argv[2]));
			if (classifier_ == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
	return RoutingModule::command(argc, argv);
}

void MPLSModule::add_route(char *dst, NsObject *target) {
	if (classifier_) 
		((MPLSAddressClassifier *)classifier_)->do_install(dst, target);
	if (next_rtm_ != NULL)
		next_rtm_->add_route(dst, target); 
}
