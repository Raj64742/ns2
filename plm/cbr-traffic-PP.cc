/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 *This code is a contribution of Arnaud Legout, Institut Eurecom, France.
 *This code is highly inspired from the code of the CBR sources.
 *The following copyright is the original copyright included in the cbr_traffic.cc file.
 *
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

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* 
 * Constant bit rate traffic source.   Parameterized by interval, (optional)
 * random noise in the interval, and packet size.  
 */

class CBR_PP_Traffic : public TrafficGenerator {
 public:
	CBR_PP_Traffic();
	virtual double next_interval(int&);
	//HACK so that udp agent knows interpacket arrival time within a burst
	inline double interval() { return (interval_); }
 protected:
	virtual void start();
	void init();
	void timeout();
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	double random_;
	int seqno_;
	int maxpkts_;
	int PP_;
	int PBM_;         /*size of the packets bunch*/
};


static class CBR_PP_TrafficClass : public TclClass {
 public:
	CBR_PP_TrafficClass() : TclClass("Application/Traffic/CBR_PP") {}
	TclObject* create(int, const char*const*) {
		return (new CBR_PP_Traffic());
	}
} class_cbr_PP_traffic;

CBR_PP_Traffic::CBR_PP_Traffic() : seqno_(0)
{
	bind_bw("rate_", &rate_);
	bind("random_", &random_);
	bind("packetSize_", &size_);
	bind("maxpkts_", &maxpkts_);
	bind("PBM_", &PBM_);
}

void CBR_PP_Traffic::init()
{
        // compute inter-packet interval 
	 interval_ = PBM_*(double)(size_ << 3)/(double)rate_;
	//interval_ = 1e-100;
	PP_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_CBR);
}

void CBR_PP_Traffic::start()
{
        init();
        running_ = 1;
        timeout();
}

double CBR_PP_Traffic::next_interval(int& size)
{
	// Recompute interval in case rate_ or size_ has changes
	if (PP_ >= (PBM_ - 1)){		
		interval_ = PBM_*(double)(size_ << 3)/(double)rate_;
		PP_ = 0;
	}
	else {
		interval_ = 1e-100;
		PP_ += 1 ;
	}
	double t = interval_;
	if (random_==1)
		t += interval_ * Random::uniform(-0.5, 0.5);	
	if (random_==2)
		t += interval_ * Random::uniform(-0.000001, 0.000001);	
	size = size_;
	if (++seqno_ < maxpkts_)
		return(t);
	else
		return(-1); 
}

void CBR_PP_Traffic::timeout()
{
        if (! running_)
                return;

        /* send a packet */
        // The test tcl/ex/test-rcvr.tcl relies on the "NEW_BURST" flag being 
        // set at the start of any exponential burst ("talkspurt").  
        if (PP_ == 0) 
                agent_->sendmsg(size_, "NEW_BURST");
        else 
                agent_->sendmsg(size_);

        /* figure out when to send the next one */
        nextPkttime_ = next_interval(size_);
        /* schedule it */
        if (nextPkttime_ > 0)
                timer_.resched(nextPkttime_);
}

