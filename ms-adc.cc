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
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ms-adc.cc,v 1.3 1998/05/08 00:30:34 bajaj Exp $";
#endif

//Measured Sum Admission control

#include "adc.h"
#include <stdlib.h>

class MS_ADC : public ADC {
public:
	MS_ADC();
	void rej_action(int, double,int);
protected:
	int admit_flow(int,double,int);
	double utilization_;
};

MS_ADC::MS_ADC() 
{
	bind("utilization_",&utilization_);
	type_ = new char[5];
	strcpy(type_, "MSAC");
}

void MS_ADC::rej_action(int cl,double p,int r)
{
	est_[cl]->change_avload(-p);
}


int MS_ADC::admit_flow(int cl,double r,int b)
{
	if (r+est_[cl]->avload() < utilization_* bandwidth_) {
		est_[cl]->change_avload(r);
		return 1;
	}
	return 0;
}


static class MS_ADCClass : public TclClass {
public:
	MS_ADCClass() : TclClass("ADC/MS") {}
	TclObject* create(int,const char*const*) {
		return (new MS_ADC());
	}
}class_ms_adc;
