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

void
WebServer::WebServer_init(WebTrafPool *webpool) {
  web_pool_ = webpool;
  
  // clean busy flag
  busy_ = 0;
  
  // Initialize function flag:
  // 0: there's no server processing delay
  // 1: server processing delay from FCFS scheduling policy
  // 2: server processing delay from STF scheduling policy
  set_mode(0);
  
  // Initialize server processing raste
  set_rate(1);
  
  // initialize the job queue
  head = tail = NULL;
  queue_size_ = 0;
  queue_limit_ = 0;

  //cancel();
}

// Set server processing rate
void WebServer::set_rate(double s_rate) {
  rate_ = s_rate;
}

// Set server function mode
void WebServer::set_mode(int s_mode) {
  mode_ = s_mode;
}

// Set the limit for job queue
void WebServer::set_queue_limit(int limit) {
  queue_limit_ = limit;
}


// Return server's node id
int WebServer::get_nid() {
  return(node->nodeid());
}

// Assign node to server
void WebServer::set_node(Node *n) {
  node = n;
}
// Return server's node
Node* WebServer::get_node() {
  return(node);
}

double WebServer::job_arrival(int obj_id, Node *clnt, Agent *tcp, Agent *snk, int size, void *data) {
  // There's no server processing delay
  if (! mode_) {
    web_pool_->launchResp(obj_id, node, clnt, tcp, snk, size, data);

    return 1;
  }

  //printf("%d %d\n", queue_limit_, queue_size_);
  if (!queue_limit_ || queue_size_ < queue_limit_) {
    // Insert the new job to the job queue
    job_s *new_job = new(job_s);
    new_job->obj_id = obj_id;
    new_job->clnt = clnt;
    new_job->tcp = tcp;
    new_job->snk = snk;
    new_job->size = size;
    new_job->data = data;
    new_job->next = NULL; 

    // always insert the new job to the tail.
    if (tail)
      tail->next = new_job;
    else
      head = new_job;
    tail = new_job;

    queue_size_++;
  } else {
    // drop the incoming job
    //printf("server drop job\n");
    return 0;
  }

  // Schedule the dequeue time when there's no job being processed
  if (!busy_) 
    schedule_next_job();
  
  return 1;
}


double WebServer::job_departure() {
  if (head) {
    web_pool_->launchResp(head->obj_id, node, head->clnt, head->tcp, head->snk, head->size, head->data);
    
    // delete the first job
    job_s *p = head;
    if (head->next)
      head = head->next;
    else 
      head = tail = NULL;
    
    delete(p);
    queue_size_--;
  }
  
  // Schedule next job
  schedule_next_job();
  return 0;
}

void WebServer::schedule_next_job() {
  job_s *p, *q;
  
  if (head) {
    // do shortest task first scheduling
    if (mode_ == STF_DELAY) { 
      // find the shortest task
      p = q = head;
      while (q) {
	if (p->size > q->size)
	  p = q;
	
	q = q->next;
      }
      
      // exchange the queue head with the shortest job
      int obj_id = p->obj_id;
      Node *clnt = p->clnt;
      int size = p->size;
      void *data = p->data;
      
      p->obj_id = head->obj_id;
      p->clnt = head->clnt;
      p->size = head->size;
      p->data = head->data;

      head->obj_id = obj_id;
      head->clnt = clnt;
      head->size = size;
      head->data = data;
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
