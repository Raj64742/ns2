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
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/acto-adc.cc,v 1.6 1999/02/12 22:01:30 breslau Exp $";
#endif


//Acceptance region Tangent at Origin Admission Control

#include "adc.h"
#include <stdlib.h>
#include <math.h>

class ACTO_ADC : public ADC {
public:
	ACTO_ADC();
	void teardown_action(int,double,int);
protected:
	int admit_flow(int,double,int);
	int rejected_; 
	double s_;
};

ACTO_ADC::ACTO_ADC() : rejected_(0)
{
	bind("s_", &s_);
	type_ = new char[5];
	strcpy(type_, "ACTO");

}


int ACTO_ADC::admit_flow(int cl,double r,int b)
{
	double p=peak_rate(cl,r,b);
	if (backoff_) {
		if (rejected_)
			return 0;
	}
	
	if (exp(p*s_)*est_[cl]->avload() <= bandwidth_) {
		if (dobump_) {
			est_[cl]->change_avload(p);
		}
		return 1;
	}
	else {
		rejected_=1;
		return 0;
	}
}

void ACTO_ADC::teardown_action(int /*cl*/,double /*r*/,int /*b*/)
{
	rejected_=0;
}

static class ACTO_ADCClass : public TclClass {
	public :
	ACTO_ADCClass() : TclClass("ADC/ACTO") {}
	TclObject* create(int,const char*const*) {
		return (new ACTO_ADC());
	}
}class_acto_adc;
