/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
#ifndef lint
static const char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/pointsample-est.cc,v 1.3 1998/06/27 01:24:19 gnguyen Exp $";
#endif

#include "estimator.h"
#include <stdlib.h>
#include <math.h>

class PointSample_Est : public Estimator {
public:
	PointSample_Est() {};
protected:
	void estimate();
};


void PointSample_Est::estimate()
{
	avload_=meas_mod_->bitcnt()/period_;
	//printf("%f %f\n",Scheduler::instance().clock(),avload_);
	fflush(stdout);
	meas_mod_->resetbitcnt(); 
}

static class PointSample_EstClass : public TclClass {
public:
	PointSample_EstClass() : TclClass ("Est/PointSample") {}
	TclObject* create(int,const char*const*) {
		return (new PointSample_Est());
	}
}class_pointsample_est;
