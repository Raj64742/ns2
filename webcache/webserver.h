//
// Copyright (c) 1999 by the University of Southern California
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
// Incorporation Polly's web traffic module into the PagePool framework
//
// Simple web server implementation
// Two server scheduling policies supported: fcfs and stf
// Xuan Chen (xuanc@isi.edu)
//

#ifndef ns_webserver_h
#define ns_webserver_h

#include "webtraf.h"

// Data structure for incoming jobs (requests)
struct job_s {
	Agent *tcp;
	int size;
	job_s *next;
};

class WebTrafPool;

// Data structure for web server
class WebServer : public TimerHandler{
public:
	WebServer(WebTrafPool*);
	
	// Server properties
	Node *node;
	double rate_;
	int busy_;
	double mode_;
	WebTrafPool *web_pool_;

	// Job queue
	job_s *head, *tail;

	virtual void expire(Event *e);

	// Job arrival and departure
	double job_arrival(Agent*, int);
	double job_departure();

	void schedule_next_job();
};
#endif //ns_webserver_h
