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
//
// Generate web traffic based on HTTP log
// Xuan Chen (xuanc@isi.edu)
//
#ifndef ns_logweb_h
#define ns_logweb_h

#include "webtraf.h"

// Data structure for next request
struct request_s {
	int time;
	int client;
	int server;
	int size;
};

class LogWebTrafPool;

// Data structure for the timer to send requests
class RequestTimer : public TimerHandler{
public:
	RequestTimer(LogWebTrafPool*);
	
	virtual void expire(Event *e);

private:
	LogWebTrafPool *lwp;
};

class LogWebTrafPool : public WebTrafPool {
public: 
	LogWebTrafPool(); 
	virtual ~LogWebTrafPool(); 

	int run();
protected:
	virtual int command(int argc, const char*const* argv);

private:
	// handler to the http log file
	FILE* fp;
	int num_obj;

	double start_t;
	RequestTimer *req_timer;
	request_s next_req;

	int loadLog(const char*);
	int start();
	int launchReq(int, int, int);
	int processLog();

	Node* picksrc(int);
	Node* pickdst(int);
};


#endif // ns_logweb_h
