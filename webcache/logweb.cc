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

// Timer to send requests
RequestTimer::RequestTimer(LogWebTrafPool* pool) {
	lwp = pool;
};

void RequestTimer::expire(Event *e) {
	//if (e) 
	//	Packet::free((Packet *)e);

	lwp->run();
}


static class LogWebTrafPoolClass : public TclClass {
public:
        LogWebTrafPoolClass() : TclClass("PagePool/WebTraf/Log") {}
        TclObject* create(int, const char*const*) {
		return (new LogWebTrafPool());
	}
} class_logwebtrafpool;

LogWebTrafPool::LogWebTrafPool() {
	num_obj = 0;
	// initialize next request
	next_req.time = 0;
	next_req.client = 0;
	next_req.server = 0;
	next_req.size = 0;

	// initialize request timer
	req_timer = new RequestTimer(this);
	start_t = 0;
}

LogWebTrafPool::~LogWebTrafPool() {
	if (fp)
		fclose(fp);
	if (req_timer) delete req_timer;
}

int LogWebTrafPool::loadLog(const char* filename) {
	fp = fopen(filename, "r");
	if (fp == 0)
		return(0);
	
	return(1);
}

int LogWebTrafPool::start() {
	start_t = Scheduler::instance().clock();
	processLog();
	return(1);
}

int LogWebTrafPool::processLog() {
	int time, client, server, size;

	if (!feof(fp)) {
		fscanf(fp, "%d %d %d %d\n", &time, &client, &server, &size);
		// save information for next request
		next_req.time = time;
		next_req.client = client;
		next_req.server = server;
		next_req.size = size;	

		double now = Scheduler::instance().clock();
		double delay = time + start_t - now;
		req_timer->resched(delay);

		return(1);

	} else 
		return(0);
}

int LogWebTrafPool::run() {
	launchReq(next_req.client, next_req.server, next_req.size);
	processLog();
	return(1);
}

Node* LogWebTrafPool::picksrc(int id) {
	int n = id % nClient_;
	assert((n >= 0) && (n < nClient_));
	return client_[n];
}

Node* LogWebTrafPool::pickdst(int id) {
	int n = id % nServer_;
	assert((n >= 0) && (n < nServer_));
	return server_[n].get_node();
}

int LogWebTrafPool::launchReq(int cid, int sid, int size) {
	TcpAgent *tcp;
	Agent *snk;
	
	// Allocation new TCP connections for both directions
	pick_ep(&tcp, &snk);

	// pick client and server nodes
	Node* client = picksrc(cid);
	Node* server = pickdst(sid);

	int num_pkt = size / 1000 + 1;

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %d %d", 
			      name(), num_obj++, num_obj,
			      client->name(), server->name(),
			      tcp->name(), snk->name(), num_pkt, NULL);

	return(1);
}

int LogWebTrafPool::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			if (start())
				return (TCL_OK);
			else
				return (TCL_ERROR);
			
		}
		
	} else if (argc == 3) {
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
