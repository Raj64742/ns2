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
#include "webserver.h"

WebServer::WebServer(WebTrafPool *webpool) {
	web_pool_ = webpool;

	rate_ = 1;
	busy_ = 0;

	// mode_ = 0: FCFS, mode_ = 1: shortest task first serve
	mode_ = 0;

	// initialize the job queue
	head = tail = NULL;

	//cancel();
}

double WebServer::job_arrival(Agent *tcp, int job_size) {
	// Insert the new job to the job queue
	job_s *new_job = new(job_s);
	new_job->tcp = tcp;
	new_job->size = job_size;
	new_job->next = NULL;

	// always insert the new job to the tail.
	if (tail)
		tail->next = new_job;
	else
		head = new_job;
	tail = new_job;

	// Schedule the dequeue time when there's no job being processed
	if (!busy_) 
		schedule_next_job();
	
	return 0;
}


double WebServer::job_departure() {
	if (head) {
		Tcl::instance().evalf("%s send-message %s %d", web_pool_->name(), (head->tcp)->name(), head->size);
		
		// delete the first job
		job_s *p = head;
		if (head->next)
			head = head->next;
		else 
			head = tail = NULL;
		
		delete(p);
	}

	// Schedule next job
	schedule_next_job();
	return 0;
}

void WebServer::schedule_next_job() {
	job_s *p, *q;

	if (head) {
		// do shortest task first scheduling
		if (mode_ == 1) { 
			// find the shortest task
			p = q = head;
			while (q) {
				if (p->size > q->size)
					p = q;
				
				q = q->next;
			}
			
			// exchange the queue head with the shortest job
			Agent *tcp = p->tcp;
			int size = p->size;

			p->tcp = head->tcp;
			p->size = head->size;
			
			head->tcp = tcp;
			head->size = size;
		}
		
		// Schedule the processing timer
		double delay_ = head->size / rate_;
		resched(delay_);
		busy_ = 1;
	//	printf("%d, %f, %f\n", head->size, rate_, delay_);
	} else
		busy_ = 0;
}

// Processing finished
void WebServer::expire(Event *e) {
	job_departure();
}
