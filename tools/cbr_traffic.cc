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

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* 
 * Constant bit rate traffic source.   Parameterized by interval, (optional)
 * random noise in the interval, and packet size.  
 */

class CBR_Source : public TrafficGenerator {
 public:
	CBR_Source();
	virtual double next_interval(int&);
	//HACK so that udp agent knows interpacket arrival time within a burst
	inline double interval() { return (interval_); }
 protected:
	void init();
	double rate_;     /* send rate during on time (bps) */
	double interval_; /* packet inter-arrival time during burst (sec) */
	double random_;
};


static class CBRTrafficClass : public TclClass {
 public:
	CBRTrafficClass() : TclClass("Application/Traffic/CBR") {}
	TclObject* create(int, const char*const*) {
		return (new CBR_Source());
	}
} class_cbr_traffic;

CBR_Source::CBR_Source() 
{
	bind_bw("rate", &rate_);
	bind("random_", &random_);
	bind("packet-size", &size_);
}

void CBR_Source::init()
{
        /* compute inter-packet interval */
	interval_ = (double)(size_ << 3)/(double)rate_;
	printf("interval %f\n", interval_);
}

double CBR_Source::next_interval(int& size)
{
	double t = interval_;
	if (random_)
		t += interval_ * Random::uniform(-0.5, 0.5);	
	size = size_;
	return(t);
}

