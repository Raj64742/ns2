/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/tools/expoo.cc,v 1.14 2004/10/07 17:56:04 haldar Exp $ (Xerox)";
#endif

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* implement an on/off source with exponentially distributed on and
 * off times.  parameterized by average burst time, average idle time,
 * burst rate and packet size.
 */

class EXPOO_Traffic : public TrafficGenerator {
 public:
	EXPOO_Traffic();
	virtual double next_interval(int&);
        virtual void timeout();
	// Added by Debojyoti Dutta October 12th 2000
	int command(int argc, const char*const* argv);
 protected:
	void init();
	double ontime_;   /* average length of burst (sec) */
	double offtime_;  /* average length of idle time (sec) */
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	unsigned int rem_; /* number of packets left in current burst */

	/* new stuff using RandomVariable */
	ExponentialRandomVariable burstlen_;
	ExponentialRandomVariable Offtime_;

};


static class EXPTrafficClass : public TclClass {
 public:
	EXPTrafficClass() : TclClass("Application/Traffic/Exponential") {}
	TclObject* create(int, const char*const*) {
		return (new EXPOO_Traffic());
	}
} class_expoo_traffic;

// Added by Debojyoti Dutta October 12th 2000
// This is a new command that allows us to use 
// our own RNG object for random number generation
// when generating application traffic

int EXPOO_Traffic::command(int argc, const char*const* argv){
        
        if(argc==3){
                if (strcmp(argv[1], "use-rng") == 0) {
                        burstlen_.seed((char *)argv[2]);
                        Offtime_.seed((char *)argv[2]);
                        return (TCL_OK);
                }
        }
        return Application::command(argc,argv);
}


EXPOO_Traffic::EXPOO_Traffic() : burstlen_(0.0), Offtime_(0.0)
{
	bind_time("burst_time_", &ontime_);
	bind_time("idle_time_", Offtime_.avgp());
	bind_bw("rate_", &rate_);
	bind("packetSize_", &size_);
}

void EXPOO_Traffic::init()
{
        /* compute inter-packet interval during bursts based on
	 * packet size and burst rate.  then compute average number
	 * of packets in a burst.
	 */
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_.setavg(ontime_/interval_);
	rem_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_EXP);
}

double EXPOO_Traffic::next_interval(int& size)
{
	double t = interval_;

	if (rem_ == 0) {
		/* compute number of packets in next burst */
		rem_ = int(burstlen_.value() + .5);
		/* make sure we got at least 1 */
		if (rem_ == 0)
			rem_ = 1;
		/* start of an idle period, compute idle time */
		t += Offtime_.value();
	}	
	rem_--;

	size = size_;
	return(t);
}

void EXPOO_Traffic::timeout()
{
	if (! running_)
		return;

	/* send a packet */
	// The test tcl/ex/test-rcvr.tcl relies on the "NEW_BURST" flag being 
	// set at the start of any exponential burst ("talkspurt").  
	if (nextPkttime_ != interval_ || nextPkttime_ == -1) 
		agent_->sendmsg(size_, "NEW_BURST");
	else 
		agent_->sendmsg(size_);
	/* figure out when to send the next one */
	nextPkttime_ = next_interval(size_);
	/* schedule it */
	if (nextPkttime_ > 0)
		timer_.resched(nextPkttime_);
}



