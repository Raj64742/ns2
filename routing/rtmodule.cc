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
 * $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/routing/rtmodule.cc,v 1.15 2002/12/18 03:36:37 sundarra Exp $
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

static class SourceRoutingModuleClass : public TclClass {
public:
        SourceRoutingModuleClass() : TclClass("RtModule/Source") {}
        TclObject* create(int, const char*const*) {
                return (new SourceRoutingModule);
        }
} class_source_routing_module;

static class QSRoutingModuleClass : public TclClass {
public:
        QSRoutingModuleClass() : TclClass("RtModule/QS") {}
        TclObject* create(int, const char*const*) {
                return (new QSRoutingModule);
        }
} class_qs_routing_module;

static class VcRoutingModuleClass : public TclClass {
public:
	VcRoutingModuleClass() : TclClass("RtModule/VC") {}
	TclObject* create(int, const char*const*) {
		return (new VcRoutingModule);
	}
} class_vc_routing_module;


#ifdef HAVE_STL
static class PgmRoutingModuleClass : public TclClass {
public:
        PgmRoutingModuleClass() : TclClass("RtModule/PGM") {}
        TclObject* create(int, const char*const*) {
                return (new PgmRoutingModule);
        }
} class_pgm_routing_module;
#endif //STL

// LMS
static class LmsRoutingModuleClass : public TclClass {
public:
        LmsRoutingModuleClass() : TclClass("RtModule/LMS") {}
        TclObject* create(int, const char*const*) {
                return (new LmsRoutingModule);
        }
} class_lms_routing_module;

RoutingModule::RoutingModule() : 
	next_rtm_(NULL), n_(NULL), classifier_(NULL) {
	bind("classifier_", (TclObject**)&classifier_);
}

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
		//if (strcmp(argv[1], "attach-classifier") == 0) {
		//classifier_ = (Classifier*)(TclObject::lookup(argv[2]));
		//if (classifier_ == NULL) {
		//tcl.add_errorf("Wrong object name %s",argv[2]);
		//return TCL_ERROR;
		//}
		//return TCL_OK;
		//}
	}
	return TclObject::command(argc, argv);
}

int BaseRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}

int SourceRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}

int QSRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}

int McastRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}

int HierRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		//if (strcmp(argv[1], "attach-classifier") == 0) {
		//classifier_ = (HierClassifier*)(TclObject::lookup(argv[2]));
		//if (classifier_ == NULL) {
		//tcl.add_errorf("Wrong object name %s",argv[2]);
		//return TCL_ERROR;
		//}
		//return TCL_OK;
		//}
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}


int ManualRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
			Node *node = (Node *)(TclObject::lookup(argv[2]));
			if (node == NULL) {
				tcl.add_errorf("Invalid node object %s", argv[2]);
				return TCL_ERROR;
			}
			if (node != n_) {
				tcl.add_errorf("Node object %s different from node_", argv[2]);
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
	}
	return (RoutingModule::command(argc, argv));
}

void VcRoutingModule::add_route(char *, NsObject *) { }
	

int VcRoutingModule::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1] , "route-notify") == 0) {
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
	}
	return (RoutingModule::command(argc, argv));
}

void RoutingModule::route_notify(RoutingModule *rtm) {
	if (next_rtm_ != NULL)
		next_rtm_->route_notify(rtm);
	else
		next_rtm_ = rtm;
}

void RoutingModule::unreg_route_notify(RoutingModule *rtm) {
	if (next_rtm_) {
		if (next_rtm_ == rtm) {
			//RoutingModule *tmp = next_rtm_;
			next_rtm_ = next_rtm_->next_rtm_;
			//free (tmp);
		}
		else {
			next_rtm_->unreg_route_notify(rtm);
		}
	}
}

void RoutingModule::add_route(char *dst, NsObject *target) 
{
	if (classifier_) 
		classifier_->do_install(dst,target); 
	if (next_rtm_ != NULL)
		next_rtm_->add_route(dst, target); 
}

void RoutingModule::delete_route(char *dst, NsObject *nullagent)
{
	if (classifier_) 
		classifier_->do_install(dst, nullagent); 
	if (next_rtm_)
		next_rtm_->add_route(dst, nullagent); 
}

void RoutingModule::set_table_size(int nn)
{
	if (classifier_)
		classifier_->set_table_size(nn);
	if (next_rtm_)
		next_rtm_->set_table_size(nn);
}

void RoutingModule::set_table_size(int level, int size)
{
	if (classifier_)
		classifier_->set_table_size(level, size);
	if (next_rtm_)
		next_rtm_->set_table_size(level, size);
}

//  void BaseRoutingModule::add_route(char *dst, NsObject *target) {
//  	if (classifier_) 
//  		((DestHashClassifier *)classifier_)->do_install(dst, target);
//  	if (next_rtm_ != NULL)
//  		next_rtm_->add_route(dst, target); 
//  }

//  void McastRoutingModule::add_route(char *dst, NsObject *target) {
//  	if (classifier_) 
//  		((DestHashClassifier *)classifier_)->do_install(dst, target);
//  	if (next_rtm_ != NULL)
//  		next_rtm_->add_route(dst, target); 
//  }

//  void HierRoutingModule::add_route(char *dst, NsObject *target) {
//  	if (classifier_) 
//  		((HierClassifier *)classifier_)->do_install(dst, target);
//  	if (next_rtm_ != NULL)
//  		next_rtm_->add_route(dst, target); 
//  }

void ManualRoutingModule::add_route(char *dst, NsObject *target) {
	int slot = classifier_->install_next(target);
	if (strcmp(dst, "default") == 0) {
		classifier_->set_default(slot);
	} else {
		int encoded_dst_address = 
			(atoi(dst)) << (AddrParamsClass::instance().node_shift(1));
		if (0 > (classifier_->do_set_hash(0, encoded_dst_address, 0, slot))) {
			fprintf(stderr, "HashClassifier::set_hash() return value less than 0\n"); }
	}
	if (next_rtm_ != NULL)
		next_rtm_->add_route(dst, target); 
}

