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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/param-adc.cc,v 1.1 1998/05/04 18:29:27 breslau Exp $";
#endif


/* Parameter-based admission control.  Admission decisions are
 * based on the sum of reserved rates.
 */

#include "adc.h"
#include <stdlib.h>

class Param_ADC : public ADC {
public:
	Param_ADC();
	void teardown_action(int,double,int);
	void rej_action(int,double,int);
	void trace(TracedVar* v);
protected:
	int admit_flow(int,double,int);
	double utilization_;
	TracedDouble resv_rate_;
	double oresv_rate_;
};

Param_ADC::Param_ADC() : resv_rate_(0), oresv_rate_(0)
{
	bind("utilization_",&utilization_);
	type_ = new char[5];
	strcpy(type_, "PBAC");

	resv_rate_.tracer(this);
	resv_rate_.name("\"Reserved Rate\"");
}

void Param_ADC::rej_action(int cl,double p,int r)
{
	resv_rate_-=p;
}


void Param_ADC::teardown_action(int cl,double p,int r)
{
	resv_rate_-=p;
}

int Param_ADC::admit_flow(int cl,double r,int b)
{
	if (resv_rate_ + r <= utilization_ * bandwidth_) {
		resv_rate_ +=r;
		return 1;
	}
	return 0;
}

static class Param_ADCClass : public TclClass {
public:
	Param_ADCClass() : TclClass("ADC/Param") {}
	TclObject* create(int,const char*const*) {
		return (new Param_ADC());
	}
}class_param_adc;

void Param_ADC::trace(TracedVar* v)
{
        char wrk[500];
	double *p, newval;

	/* check for right variable */
	if (strcmp(v->name(), "\"Reserved Rate\"") == 0) {
	        p = &oresv_rate_;
	}
	else {
	        fprintf(stderr, "PBAC: unknown trace var %s\n", v->name());
		return;
	}

	newval = double(*((TracedDouble*)v));

        if (tchan_) {
	        int n;
	        double t = Scheduler::instance().clock();
		/* f -t 0.0 -s 1 -a SA -T v -n Num -v 0 -o 0 */
		sprintf(wrk, "f -t %g -s %d -a %s:%d-%d -T v -n %s -v %g -o %g",
			t, src_, type_, src_, dst_, v->name(), newval, *p);
		n = strlen(wrk);
		wrk[n] = '\n';
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
		
	}

	*p = newval;

	return;



}

