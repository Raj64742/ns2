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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/pareto.cc,v 1.3 1998/06/09 21:53:21 breslau Exp $ (Xerox)";
#endif
 
#include "random.h"
#include "trafgen.h"

/* implement an on/off source with average on and off times taken
 * from a pareto distribution.  (enough of these sources multiplexed
 * produces aggregate traffic that is LRD).  It is parameterized
 * by the average burst time, average idle time, burst rate, and
 * pareto shape parameter and packet size.
 */

class POO_Source : public TrafficGenerator {
 public:
	POO_Source();
	virtual double next_interval(int&);
	int on()  { return on_ ; }
 protected:
	void init();
	double ontime_;  /* average length of burst (sec) */
	double offtime_; /* average idle period (sec) */
	double rate_;    /* send rate during burst (bps) */
	double interval_; /* inter-packet time at burst rate */
	double burstlen_; /* average # packets/burst */
	double shape_;    /* pareto shape parameter */
	unsigned int rem_; /* number of packets remaining in current burst */
	double p1_;       /* parameter for pareto distribution to compute
			   * number of packets in burst.
		           */
	double p2_;       /* parameter for pareto distribution to compute
		     	   * length of idle period.
		     	   */
	int on_;          /* denotes whether in the on or off state */
};


static class POOClass : public TclClass {
 public:
	POOClass() : TclClass("Traffic/Pareto") {}
 	TclObject* create(int, const char*const*) {
		return (new POO_Source());
	}
} class_poo;

POO_Source::POO_Source()
{
	bind_time("burst-time", &ontime_);
	bind_time("idle-time", &offtime_);
	bind_bw("rate", &rate_);
	bind("shape", &shape_);
	bind("packet-size", &size_);
}

void POO_Source::init()
{
	interval_ = (double)(size_ << 3)/(double)rate_;
	burstlen_ = ontime_/interval_;
	rem_ = 0;
	on_ = 0;
	p1_ = burstlen_ * (shape_ - 1.0)/shape_;
	p2_ = offtime_ * (shape_ - 1.0)/shape_;
}

double POO_Source::next_interval(int& size)
{
	double t = interval_;

	on_ = 1;
	if (rem_ == 0) {
		/* compute number of packets in next burst */
		rem_ = int(Random::pareto(p1_, shape_) + .5);
		/* make sure we got at least 1 */
		if (rem_ == 0)
			rem_ = 1;	
		/* start of an idle period, compute idle time */
		t += Random::pareto(p2_, shape_);
		on_ = 0;
	}	
	rem_--;

	size = size_;
	return(t);

}
