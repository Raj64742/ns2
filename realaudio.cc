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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/realaudio.cc,v 1.1 2000/09/27 18:13:04 kclan Exp $ (USC/ISI)";
#endif
 
#include <sys/time.h>
#include "random.h"
#include "trafgen.h"
#include "ranvar.h"

class RA_Traffic : public TrafficGenerator {
 public:
	RA_Traffic();
	virtual double next_interval(int&);
 protected:
	void init();
	double ontime_;  /* average length of burst (sec) */
	double offtime_; /* average idle period (sec) */
	double rate_;    /* send rate during burst (bps) */
	double interval_; /* inter-packet time at burst rate */
	double burstlen_; /* average # packets/burst */
	unsigned int rem_; /* number of packets remaining in current burst */
        EmpiricalRandomVariable Offtime_ ;
//      EmpiricalRandomVariable Ontime_ ;
};


static class RATrafficClass : public TclClass {
 public:
	RATrafficClass() : TclClass("Application/Traffic/RealAudio") {}
 	TclObject* create(int, const char*const*) {
		return (new RA_Traffic());
	}
} class_ra_traffic;

RA_Traffic::RA_Traffic()
{
	bind_time("burst_time_", &ontime_);
	bind_time("idle_time_", &offtime_);
	bind_bw("rate_", &rate_);
	bind("packetSize_", &size_);
}

void RA_Traffic::init()
{
        int res = Offtime_.loadCDF("offtimecdf");
//      int res1 = Ontime_.loadCDF("ontimecdf");
        timeval tv;
	gettimeofday(&tv, 0);
	srand48(tv.tv_sec) ;

	interval_ = ontime_ ;
	burstlen_ = (double) ( rate_ * (ontime_ + offtime_))/ (double)(size_ << 3);
	rem_ = 0;
	if (agent_)
		agent_->set_pkttype(PT_REALAUDIO);
}

double RA_Traffic::next_interval(int& size)
{
//	double t = Ontime_.value() ;
	double o = Offtime_.value() ; //off-time is taken from CDF
//      double ran = drand48();

	double t = ontime_ ; //fixed on-time
//	double o = offtime_ ;

//	o = o * ran ;

	if (rem_ == 0) {
		/* compute number of packets in next burst */
		rem_ = int(burstlen_ + .5);

		//recalculate the number of packet sent during ON-period if
		// off-time is mutliple of 1.8s
		if (o > offtime_ ) {
	           double b = 
		   // (double) ( rate_ * (ontime_ + o))/ (double)(size_ << 3);
		   (double) ( rate_ * (t + o))/ (double)(size_ << 3);
		   rem_ = int(b);
		}

//		if (ran > 0.8 ) rem_++ ;
//		if (ran < 0.2 ) rem_-- ;

		/* make sure we got at least 1 */
		if (rem_<= 0)
			rem_ = 1;	
		/* start of an idle period, compute idle time */
		t += o ;
	}	
	rem_--;

	size = size_;
	return(t);

}
