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

//Time Window estimation

#include "estimator.h"
#include <stdlib.h>

class TimeWindow_Est : public Estimator {
public:
	TimeWindow_Est() :scnt(1),maxp(0){bind("T_",&T_);};
	inline void change_avload(double incr) { avload_ += incr; if (incr >0) scnt=0;}
protected:
	void estimate();
	double maxp;//maximum of previous interval
	int T_;
	int scnt;
};

void TimeWindow_Est::estimate() {
	measload_ = meas_mod_->bitcnt()/period_;
	if (meas_mod_->bitcnt()/period_ >avload_)
		avload_=meas_mod_->bitcnt()/period_;
	if (maxp < meas_mod_->bitcnt()/period_)
		maxp=meas_mod_->bitcnt()/period_;

	if (scnt == T_)  
		{
			scnt-=T_;
			avload_=maxp;
			maxp=0;
		}
	//printf("%f %f %f\n",Scheduler::instance().clock(),avload_,meas_mod_->bitcnt()/period_);
	
	fflush(stdout);
	meas_mod_->resetbitcnt();
	scnt++;
}

static class TimeWindow_EstClass : public TclClass {
public:
	TimeWindow_EstClass() : TclClass ("Est/TimeWindow") {}
	TclObject* create(int,const char*const*) {
		return (new TimeWindow_Est());
	}
}class_timewindow_est;

