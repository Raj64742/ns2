/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "pushback-queue.h"

#include "ip.h"
#include "pushback.h"
#include "rate-limit.h"

static class PushbackQueueClass : public TclClass {

public:
  PushbackQueueClass() : TclClass("Queue/RED/Pushback") {}

  TclObject * create(int argc, const char*const* argv) {
    if (argc==4) {
      printf("Missing Argument for Pushback Queue Constructor\n");
      exit(-1);
    }
    return (new PushbackQueue(argv[4]));
  }

} class_pushback_queue;


PushbackQueue::PushbackQueue(const char* const pba): pushbackID_(-1), src_(-1), dst_(-1), 
  qmon_(NULL) {
  
  pushback_ = (PushbackAgent *)TclObject::lookup(pba);
  if (pushback_ == NULL) {
    printf("Wrong Argument for Pushback Queue Constructor\n");
    exit(-1);
  }
  bind("pushbackID_", &pushbackID_);
  bind_bool("rate_limiting_", &rate_limiting_);
  
  timer_ = new PushbackQueueTimer(this);
  timer_->resched(SUSTAINED_CONGESTION_PERIOD);

  rateEstimator_=new RateEstimator();
  rlsList_ = new RateLimitSessionList();
  printf("pushback queue instantiated %d\n",pushback_->last_index_);
  
}

void
PushbackQueue::reportDrop(Packet *p) {
  if (debug_) 
    printf("PBQ:(%d:%d) rate limiting = %d\n", src_, dst_, rate_limiting_);
  
  if (rate_limiting_) 
    pushback_->reportDrop(pushbackID_, p);
  
}

int 
PushbackQueue::command(int argc, const char*const* argv)
{
  Tcl& tcl = Tcl::instance();
  if (argc == 3) {
    if (strcmp(argv[1], "set-monitor") == 0) {
      qmon_ = (EDQueueMonitor *)TclObject::lookup(argv[2]);
      if (qmon_ == NULL) {
	tcl.resultf("Got Invalid Queue Monitor\n");
	return TCL_ERROR;
      }
      return TCL_OK;
    }
  } else if (argc == 4) {
     if (strcmp(argv[1], "set-src-dst") == 0) {
       src_ = atoi(argv[2]);
       dst_ = atoi(argv[3]);
       if (src_ < 0 || dst_ < 0) {
	 tcl.resultf("Got Invalid Source or Destination\n");
	 return TCL_ERROR;
       }
       return TCL_OK;
     }
  } 
  
  return REDQueue::command(argc, argv);
}
 
void 
PushbackQueue::timeout(int from) {
  
  int barrivals = qmon_->barrivals() - qmon_->mon_ebdrops();
  int bdrops = qmon_->bdrops() - qmon_->mon_ebdrops();
  int bdeps = qmon_->bdepartures();
  Tcl& tcl = Tcl::instance();
  printf("PBQ:(%d:%d) (%g) arrs = %d  drops = %d deps = %d mon_drops = %d\n", 
	 src_, dst_, Scheduler::instance().clock(), 
	 barrivals*8, bdrops*8, bdeps*8, qmon_->mon_ebdrops()*8 );
  fflush(stdout);
  tcl.evalf("%s reset",qmon_->name());
  
  // an alternate way of calculating this is using the arrivals and drops from above, 
  // but the below is more accurate as RED avg queue takes time to come down and
  // hence drop rate goes down much slower.
  double dropRate= getDropRate();
  
  if (rate_limiting_ && dropRate >= SUSTAINED_CONGESTION_DROPRATE) {
    printf("PBQ:(%d:%d) (%g) Arr: %d (%g) Drops: %d (%g) BW: %g\n", src_, dst_, 
	   Scheduler::instance().clock(), barrivals, rateEstimator_->estRate_, 
	   bdrops, dropRate, link_->bandwidth());
    fflush(stdout);
    
    //this function call would 
    //  1) start a rate limiting session, 
    //  2) insert it in the queues rate limiting session list.
    //  3) will also set up appropriate timers.
    pushback_->identifyAggregate(pushbackID_, rateEstimator_->estRate_, link_->bandwidth());
  }

  //reset the drop history at the agent
  pushback_->resetDropLog(pushbackID_);

  timer_->resched(SUSTAINED_CONGESTION_PERIOD);
}
  
void 
PushbackQueue::enque(Packet *p) {

  hdr_cmn * hdr = HDR_CMN(p);

  if (debug_) 
    printf("In queue enque with ptype %d %d\n", hdr->ptype(), PT_PUSHBACK);

  if (hdr->ptype_ == PT_PUSHBACK) {
    printf("PBQ:(%d:%d). Got a pushback packet.\n",src_, dst_);
    q_->enqueHead(p);
    return;
  }

  int dropped = 0;
  //set lowDemand to 0 to switch off the low-demand feature.
  int qlen = qib_ ? bcount_ : q_->length();
  int lowDemand = (edv_.v_ave < edp_.th_min || qlen < 1);
  //  lowDemand = 0;
  
  //this would 
  // 1. check to see if a packet belongs to any of the aggregate being rate-limited
  // 2. if yes, log the packet and 
  // 3. drop it if necessary (based on rate-limiting dynamics).
  // 4. dropped = 1, if dropped.
  if (rlsList_->noSessions_) 
    dropped = rlsList_->filter(p, lowDemand);
  
  if (dropped) {
    qmon_->mon_edrop(p);
    
    //this is buggy. this drop is not recorded by any other monitor attached to the link.
    Packet::free(p);
    return;
  }
  
  //estimate rate only for enqued packets (insignificant bw of pushback messages).
  //also counts packet not dropped because of low demand (minor effects to overall demand calculations I believe).
  rateEstimator_->estimateRate(p);
  REDQueue::enque(p);

}


double
PushbackQueue::getRate() {
  return rateEstimator_->estRate_; 
}

double 
PushbackQueue::getBW() { 
  return link_->bandwidth(); 
}

double
PushbackQueue::getDropRate() {
  if (rateEstimator_->estRate_ < getBW()) {
    return 0;
  } else {
    return 1 - getBW()/rateEstimator_->estRate_;
  }
}
  
