/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * delaymodel.cc
 * Copyright (C) 1997 by USC/ISI
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/delaymodel.cc,v 1.8 1998/08/22 02:41:00 haoboy Exp $ (UCB)";
#endif

#include "packet.h"
#include "delaymodel.h"

static class DelayModelClass : public TclClass {
public:
	DelayModelClass() : TclClass("DelayModel") {}
	TclObject* create(int, const char*const*) {
		return (new DelayModel);
	}
} class_delaymodel;

DelayModel::DelayModel() : Connector(), bandwidth_(0)
{
}


int DelayModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "ranvar") == 0) {
			ranvar_ = (RandomVariable*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "bandwidth") == 0) {
			bandwidth_ = atof(argv[2]);
			return (TCL_OK);
		} 
	} else if (argc == 2) {
		if (strcmp(argv[1], "ranvar") == 0) {
			tcl.resultf("%s", ranvar_->name());
			return (TCL_OK);
		}
	}
	return Connector::command(argc, argv);
}

void DelayModel::recv(Packet* p, Handler*)
{
	double delay = ranvar_->value();
	//static int tmp = 0;

	double txt = txtime(p);
	Scheduler& s = Scheduler::instance();
	//printf ("trans %f, delay %f\n", txt, delay);
	s.schedule(target_, p, txt + delay);

	//s.schedule(h, &intr_, txt);
}


