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

#define NO_DELAY 0
#define FCFS_DELAY 1
#define STF_DELAY 2

// Data structure for incoming jobs (requests)
struct job_s {
  int obj_id;
  Node *clnt;
  Agent *tcp;
  Agent *snk;
  int size;
  void *data;
  job_s *next;
};

class WebTrafPool;

// Data structure for web server
class WebServer : public TimerHandler{
 private:
       void WebServer_init(WebTrafPool *);

 public:
	WebServer(WebTrafPool*pool_) { WebServer_init(pool_); };
	WebServer() { WebServer_init(NULL); };
	
	// Assign node to server
	void set_node(Node *);
	// Return server's node
	Node* get_node();
	// Return server's node id
	int get_nid();

	// Set server processing rate
	void set_rate(double);
	// Set server function mode
	void set_mode(int);
	// Set the limit for job queue
	void set_queue_limit(int);

	// Handling incoming job
	double job_arrival(int, Node *, Agent *, Agent *, int, void *);

 private:
	// The web page pool associated with this server
	WebTrafPool *web_pool_;

	// The node associated with this server
	Node *node;

	// Server processing rate KB/s
	double rate_;
	
	// Flag for web server:
	// 0: there's no server processing delay
	// 1: server processing delay from FCFS scheduling policy
	// 2: server processing delay from STF scheduling policy
	int mode_;

	// Job queue
	job_s *head, *tail;
	int queue_size_;
	int queue_limit_;

	virtual void expire(Event *e);

	double job_departure();
	
	// Flag for server status
	int busy_;
	void schedule_next_job();
};
#endif //ns_webserver_h
