/*
 * Copyright (c) 2001 University of Southern California.
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
 **
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This implements the Quick Start Agent at each of network element "Agent/QSAgent"
 * qsagent.cc
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include "object.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"
#include "connector.h"
#include "delay.h"
#include "queue.h"
#include "scheduler.h"
#include "random.h"

#include "hdr_qs.h"
#include "qsagent.h"
#include <fstream>

static class QSAgentClass : public TclClass {
public:
	QSAgentClass() : TclClass("Agent/QSAgent") {}
	TclObject* create(int, const char*const*) {
		return (new QSAgent);
	}
} class_QSAgent;

QSAgent::QSAgent():Agent(PT_TCP), old_classifier_(NULL), qs_enabled_(1), 
    qs_timer_(this) 
{

	prev_int_aggr_ = 0;
	aggr_approval_ = 0;

	bind("qs_enabled_", &qs_enabled_);
	bind("old_classifier_", &old_classifier_);
	bind("state_delay_", &state_delay_);
	bind("alloc_rate_", &alloc_rate_);
	bind("max_rate_", &max_rate_);
	bind("mss_", &mss_);

	qs_timer_.resched(state_delay_);
  
}

QSAgent::~QSAgent()
{
}

int QSAgent::command(int argc, const char*const* argv)
{
	return (Agent::command(argc,argv));
}

void QSAgent::recv(Packet* packet, Handler*)
{
	int src, dst, nhop, app_rate;
	double avail_bw, util;
	Classifier * pkt_target;
	Tcl& tcl = Tcl::instance();
	char qname[64], lname[64];

	hdr_cmn *cmh = hdr_cmn::access(packet);
	hdr_qs *qsh =  hdr_qs::access(packet);
	hdr_ip *iph = hdr_ip::access(packet);

	if (old_classifier_)  
		pkt_target = (Classifier *)TclObject::lookup(old_classifier_->name());

	if (qs_enabled_) {
		/*if ((qsh->flag() == QS_REQUEST || qsh->flag() == QS_RESPONSE) && qsh->rate() > 0) 
		printf("%d: got flag = %d, ttl1 = %d, ttl2 = %d, rate = %d\n", 
		addr(), qsh->flag(), iph->ttl(), qsh->ttl(), qsh->rate());*/
		if (qsh->flag() == QS_REQUEST && qsh->rate() > 0 && iph->daddr() != addr()) {
			sprintf (qname, "[Simulator instance] get-queue %d %d", addr(), iph->daddr()); 
			tcl.evalc (qname);
			Queue * queue = (Queue *) TclObject::lookup(tcl.result());

			sprintf (lname, "[Simulator instance] get-link %d %d", addr(), iph->daddr()); 
			tcl.evalc (lname);
			LinkDelay * link = (LinkDelay *) TclObject::lookup(tcl.result());

			if (link != NULL && queue != NULL) {
				util = queue->utilization();
				avail_bw = link->bandwidth() / 8 * (1 - util) / mss_;
				avail_bw -= (prev_int_aggr_ + aggr_approval_);
				avail_bw *= alloc_rate_;
				printf("%d: requested = %d, available = %f\n", addr(), qsh->rate(), avail_bw);
				app_rate = (avail_bw < (double)qsh->rate()) ? (int) avail_bw : qsh->rate();
				app_rate = (app_rate < max_rate_) ? app_rate : max_rate_;
				if (app_rate > 0) {    
					aggr_approval_ += app_rate; // add approved to current bucket
					qsh->ttl() -= 1;
					qsh->rate() = app_rate; //update rate
				}
				else {
					qsh->rate() = 0; //disable quick start, not enough bandwidth
					qsh->flag() = QS_DISABLE;
				}
			}
      }
  }

#if 0   
  if (qs_enabled_) {
      /*if ((qsh->flag() == QS_REQUEST || qsh->flag() == QS_RESPONSE) && qsh->rate() > 0) 
          printf("%d: got flag = %d, ttl1 = %d, ttl2 = %d, rate = %d\n", 
              addr(), qsh->flag(), iph->ttl(), qsh->ttl(), qsh->rate());*/
      if (qsh->flag() == QS_REQUEST && qsh->rate() > 0 && iph->daddr() != addr()) {
          Connector * head = (Connector *) pkt_target->find(packet);
          if (head) {
              //printf("%d: got head %s %p\n", addr(), head->name(), head);
              Connector * enqT = (Connector *) head->target();
              if (enqT) {
                  //printf("%d: got enqT %s %p\n", addr(), enqT->name(), enqT);
                  Queue * queue = (Queue *) enqT->target();
                  if (queue) {
                      //printf("%d: got queue %s %p\n", addr(), queue->name(), queue);
                      Connector * deqT = (Connector *) queue->target();
                      if (deqT) {
                          //printf("%d: got deqT %s %p\n", addr(), deqT->name(), deqT);
                          LinkDelay * link = (LinkDelay *) deqT->target();
                          if (link) {
							  util = queue->utilization();
                              avail_bw = link->bandwidth() / 8 * (1 - util) / mss_;
                              avail_bw -= (prev_int_aggr_ + aggr_approval_);
                              avail_bw *= alloc_rate_;
                              printf("%d: requested = %d, available = %f\n", addr(), qsh->rate(), avail_bw);
                              app_rate = (avail_bw < (double)qsh->rate()) ? (int) avail_bw : qsh->rate();
                              app_rate = (app_rate < max_rate_) ? app_rate : max_rate_;
                              if (app_rate > 0) {    
                                  aggr_approval_ += app_rate; // add approved to current bucket
                                  qsh->ttl() -= 1;
                                  qsh->rate() = app_rate; //update rate
                              }
                              else {
                                  qsh->rate() = 0; //disable quick start, not enough bandwidth
                                  qsh->flag() = QS_DISABLE;
                              }
                          }
                      }
                  }
              }
          }
      }
  }
#endif

	if (pkt_target) 
		pkt_target->recv(packet, 0);
	else {
		printf("%d, don't know what to do with the packet\n", addr()); // should never get here.
		Packet::free(packet);
	}

	return;
  
}

void QSTimer::expire(Event *e) {
	
	qs_handle_->prev_int_aggr_ = qs_handle_->aggr_approval_;
	qs_handle_->aggr_approval_ = 0;

	this->resched(qs_handle_->state_delay_);

}
