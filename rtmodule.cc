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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/rtmodule.cc,v 1.4 2001/02/01 22:56:21 haldar Exp $
 */

#include "rtmodule.h"
#include <assert.h>
#include "node.h"


static class RoutingModuleClass : public TclClass {
public:
	RoutingModuleClass() : TclClass("RtModule") {}
	TclObject* create(int, const char*const*) {
		return (new RoutingModule);
	}
} class_routing_module;

static class BaseRoutingModuleClass : public TclClass {
public:
	BaseRoutingModuleClass() : TclClass("RtModule/Base") {}
	TclObject* create(int, const char*const*) {
		return (new BaseRoutingModule);
	}
} class_base_routing_module;

static class McastRoutingModuleClass : public TclClass {
public:
	McastRoutingModuleClass() : TclClass("RtModule/Mcast") {}
	TclObject* create(int, const char*const*) {
		return (new McastRoutingModule);
	}
} class_mcast_routing_module;

static class HierRoutingModuleClass : public TclClass {
public:
	HierRoutingModuleClass() : TclClass("RtModule/Hier") {}
	TclObject* create(int, const char*const*) {
		return (new HierRoutingModule);
	}
} class_hier_routing_module;


static class ManualRoutingModuleClass : public TclClass {
public:
	ManualRoutingModuleClass() : TclClass("RtModule/Manual") {}
	TclObject* create(int, const char*const*) {
		return (new ManualRoutingModule);
	}
} class_manual_routing_module;

static class VcRoutingModuleClass : public TclClass {
public:
	VcRoutingModuleClass() : TclClass("RtModule/VC") {}
	TclObject* create(int, const char*const*) {
		return (new VcRoutingModule);
	}
} class_vc_routing_module;


int RoutingModule::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "node") == 0) {
			assert(n_ != NULL);
			tcl.resultf("%s", n_->name());
			return TCL_OK;
		} else if (strcmp(argv[1], "module-name") == 0) {
			if (module_name() != NULL)
				tcl.resultf("%s", module_name());
			else 
				tcl.result("");
			return TCL_OK;
		}
		
	} else if (argc == 3) {
		if (strcmp(argv[1], "attach-node") == 0) {
			n_ = (Node*)TclObject::lookup(argv[2]);
			if (n_ == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		if (strcmp(argv[1], "attach-classifier") == 0) {
			classifier_ = (Classifier*)(TclObject::lookup(argv[2]));
			if (classifier_ == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		
	}
	else if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) {
			char dst[SMALL_LEN];
			strcpy(dst, argv[2]);
			NsObject* target = (NsObject *)(TclObject::lookup(argv[3]));
			if (target == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[3]);
			}
			if (classifier_)
				classifier_->do_install(dst, target);
			return TCL_OK;
		}
	}
	
	return TclObject::command(argc, argv);
}

int BaseRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) {
			char *dst;
			strcpy(dst, argv[2]);
			NsObject* target = (NsObject *)(TclObject::lookup(argv[3]));
			if (target == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[3]);
			}
			if (classifier_) 
				((DestHashClassifier *)classifier_)->do_install(dst, target);
			return TCL_OK;
		}
	}
	return (RoutingModule::command(argc, argv));
}

int McastRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) {
			char *dst;
			strcpy(dst, argv[2]);
			NsObject* target = (NsObject *)(TclObject::lookup(argv[3]));
			if (target == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[3]);
			}
			if (classifier_) 
				((DestHashClassifier *)classifier_)->do_install(dst, target);
			return TCL_OK;
		}
	}
	return (RoutingModule::command(argc, argv));
}

int HierRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) {
			char *dst;
			strcpy(dst, argv[2]);
			NsObject* target = (NsObject *)(TclObject::lookup(argv[3]));
			if (target == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[3]);
			}
			if (classifier_) {
				((HierClassifier *)classifier_)->do_install(dst, target);
				return TCL_OK;
			} else 
				return TCL_ERROR;
		}
	}
	return (RoutingModule::command(argc, argv));
}


int ManualRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) {
			char *dst;
			strcpy(dst, argv[2]);
			NsObject* target = (NsObject *)(TclObject::lookup(argv[3]));
			if (target == NULL) {
				tcl.add_errorf("Wrong object name %s",argv[3]);
			}
			add_route(dst,target);
			return TCL_OK;
		}
	}
	return (RoutingModule::command(argc, argv));
}

void ManualRoutingModule::add_route(char *dst, NsObject *target) {
	int slot = ((DestHashClassifier *)classifier_)->install_next(target);
	if (strcmp(dst, "default") == 0) {
		((DestHashClassifier *)classifier_)->set_default(slot);
	} else {
		int encoded_dst_address = 
			(atoi(dst)) << (AddrParamsClass::instance().node_shift(1));
		if (0 > (((DestHashClassifier *)classifier_)->do_set_hash(0, encoded_dst_address, 0, slot))) {
			fprintf(stderr, "HashClassifier::set_hash() return value less than 0\n"); }
	}
}

int VcRoutingModule::command(int argc, const char*const* argv) {
	if (argc == 4) {
		if (strcmp(argv[1], "add_route") == 0) { 
			return TCL_OK;
		}
	}
	return (RoutingModule::command(argc, argv));
}




