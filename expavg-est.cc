/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

#include <math.h>
#include "estimator.h"

class ExpAvg_Est : public Estimator {
public:
	ExpAvg_Est() {bind("w_",&w_);};
protected:
	void estimate();
	double w_;
};


void ExpAvg_Est::estimate()
{
	avload_=(1-w_)*avload_+w_*meas_mod_->bitcnt()/period_;
	//printf("%f %f %f\n",Scheduler::instance().clock(),avload_,meas_mod_->bitcnt()/period_);
	fflush(stdout);
	meas_mod_->resetbitcnt(); 
}

static class ExpAvg_EstClass : public TclClass {
public:
	ExpAvg_EstClass() : TclClass ("Est/ExpAvg") {}
	TclObject* create(int,const char*const*) {
		return (new ExpAvg_Est());
	}
}class_expavg_est;

