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
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/estimator.cc,v 1.7 1999/03/13 03:52:47 haoboy Exp $";
#endif

#include "estimator.h"

Estimator::Estimator() : meas_mod_(0),avload_(0.0),est_timer_(this), measload_(0.0), tchan_(0), omeasload_(0), oavload_(0)
{
	bind("period_",&period_);
	bind("src_", &src_);
	bind("dst_", &dst_);

	avload_.tracer(this);
	avload_.name("\"Estimated Util.\"");
	measload_.tracer(this);
	measload_.name("\"Measured Util.\"");
}

int Estimator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1],"load-est") == 0) {
			tcl.resultf("%.3f",double(avload_));
			return(TCL_OK);
		} else if (strcmp(argv[1],"link-utlzn") == 0) {
			tcl.resultf("%.3f",meas_mod_->bitcnt()/period_);
			return(TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("Estimator: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "setbuf") == 0) {
			/* some sub classes actually do something here */
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
	measload_ = 0;
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

void Estimator_Timer::expire(Event* /*e*/) 
{
	est_->timeout(0);
}

void Estimator::trace(TracedVar* v)
{
	char wrk[500];
	double *p, newval;

	/* check for right variable */
	if (strcmp(v->name(), "\"Estimated Util.\"") == 0) {
		p = &oavload_;
	}
	else if (strcmp(v->name(), "\"Measured Util.\"") == 0) {
		p = &omeasload_;
	}
	else {
		fprintf(stderr, "Estimator: unknown trace var %s\n", v->name());
		return;
	}

	newval = double(*((TracedDouble*)v));

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		/* f -t 0.0 -s 1 -a SA -T v -n Num -v 0 -o 0 */
		sprintf(wrk, "f -t %g -s %d -a %s:%d-%d -T v -n %s -v %g -o %g",
			t, src_, actype_, src_, dst_, v->name(), newval, *p);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
		
	}

	*p = newval;

	return;



}

void Estimator::setactype(const char* type)
{
	actype_ = new char[strlen(type)+1];
	strcpy(actype_, type);
	return;
}
