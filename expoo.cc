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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/expoo.cc,v 1.6 1998/06/27 01:03:32 gnguyen Exp $ (Xerox)";
#endif

#include <stdlib.h>
 
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"


/* implement an on/off source with exponentially distributed on and
 * off times.  parameterized by average burst time, average idle time,
 * burst rate and packet size.
 */

class EXPOO_Source : public TrafficGenerator {
 public:
	EXPOO_Source();
	virtual double next_interval(int&);
	//HACK so that udp agent knows interpacket arrival time within a burst
	inline double interval() {
	return (interval_);
	}
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


static class EXPClass : public TclClass {
 public:
	EXPClass() : TclClass("Application/Traffic/Exponential") {}
	TclObject* create(int, const char*const*) {
		return (new EXPOO_Source());
	}
} class_expoo;

EXPOO_Source::EXPOO_Source() : burstlen_(0.0), Offtime_(0.0)
{
	bind_time("burst-time", &ontime_);
	bind_time("idle-time", Offtime_.avgp());
	bind_bw("rate", &rate_);
	bind("packet-size", &size_);
}

void EXPOO_Source::init()
{
        /* compute inter-packet interval during bursts based on
	 * packet size and burst rate.  then compute average number
	 * of packets in a burst.
	 */
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_.setavg(ontime_/interval_);
	rem_ = 0;
}

double EXPOO_Source::next_interval(int& size)
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

