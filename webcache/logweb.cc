/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 2001 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Generate web traffic based on HTTP log
// Xuan Chen (xuanc@isi.edu)
//
#include <tclcl.h>

#include "logweb.h"


static class LogWebTrafPoolClass : public TclClass {
public:
        LogWebTrafPoolClass() : TclClass("PagePool/WebTraf/Log") {}
        TclObject* create(int, const char*const*) {
		return (new LogWebTrafPool());
	}
} class_logwebtrafpool;

LogWebTrafPool::LogWebTrafPool() {
	num_obj = 0;
}

LogWebTrafPool::~LogWebTrafPool() {
	if (fp)
		fclose(fp);
}

int LogWebTrafPool::loadLog(const char* filename) {
	fp = fopen(filename, "r");
	if (fp == 0)
		return(0);
	
	int time, client, server, size;
	while (!feof(fp)) {
		fscanf(fp, "%d %d %d %d\n", 
		       &time, &client, &server, &size);
		printf("%d %d %d %d\n", 
		       time, client, server, size);
		launchReq(time, client, server, size);
	}
	
	return(1);
}

int LogWebTrafPool::launchReq(int time, int cid, int sid, int size) {
	// Choose source and dest TCP agents for both source and destination
	TcpAgent* ctcp = this->picktcp();
	TcpAgent* stcp = this->picktcp();

	Agent* csnk = this->picksink();
	Agent* ssnk = this->picksink();

	Node* client = picksrc();
	Node* server = pickdst();

	int num_pkt = size / 1000 + 1;

	printf("%s launch-req %d %d %s %s %s %s %s %s %d %d",
			      this->name(), num_obj++, num_obj, 
			      client->name(), server->name(),
			      ctcp->name(), csnk->name(), 
			      stcp->name(), ssnk->name(), 
			      num_pkt, this);

	Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %s %s %d %d",
			      this->name(), num_obj++, num_obj, 
			      client->name(), server->name(),
			      ctcp->name(), csnk->name(), 
			      stcp->name(), ssnk->name(), 
			      num_pkt, this);
}

int LogWebTrafPool::command(int argc, const char*const* argv) {
	if (argc == 3) {
		if (strcmp(argv[1], "loadLog") == 0) {
			if (loadLog(argv[2]))
				return (TCL_OK);
			else
				return (TCL_ERROR);

		} else if (strcmp(argv[1], "doneObj") == 0) {
			return (TCL_OK);
		}
	}
	return WebTrafPool::command(argc, argv);
}
