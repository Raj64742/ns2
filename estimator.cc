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

#include  "estimator.h"

Estimator::Estimator() : meas_mod_(0),avload_(0),est_timer_(this)
{
	bind("period_",&period_);
}

int Estimator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"load-est") == 0) {
			tcl.resultf("%.3f",avload_);
			return(TCL_OK);
		} else if (strcmp(argv[1],"link-utlzn") == 0) {
			tcl.resultf("%.3f",meas_mod_->bitcnt()/period_);
			return(TCL_OK);
		}
	}
	return NsObject::command(argc,argv);
}

void Estimator::setmeasmod (MeasureMod *measmod)
{
	meas_mod_=measmod;
}

void Estimator::start()
{
	avload_=0;
	est_timer_.resched(period_);
}

void Estimator::stop()
{
	est_timer_.cancel();
}

void Estimator::timeout(int)
{
	estimate();
	est_timer_.resched(period_);
}

void Estimator_Timer::expire(Event *e) 
{
	est_->timeout(0);
}
