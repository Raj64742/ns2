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
 * NixVector Routing module for ns
 * contributed to ns from 
 * George F. Riley, Georgia Tech, Spring 2000
 *
 */

#ifdef HAVE_STL
#ifdef NIXVECTOR

#include "rtmodule.h"

class NixRoutingModule : public RoutingModule {
public:
	NixRoutingModule() : RoutingModule() {}
	virtual const char* module_name() const { return "Nix"; }
	virtual int command(int argc, const char*const* argv);  
};

static class NixRoutingModuleClass : public TclClass {
public:
	NixRoutingModuleClass() : TclClass("RtModule/Nix") {}
	TclObject* create(int, const char*const*) {
		return (new NixRoutingModule);
	}
} class_nix_routing_module;

int NixRoutingModule::command(int argc, const char*const* argv) {
       if (argc == 3) {
               if (strcmp(argv[1] , "route-notify") == 0) {
                       return TCL_OK;
               }
       }
       return (RoutingModule::command(argc, argv));

}  

#endif /* NIXVECTOR */
#endif
