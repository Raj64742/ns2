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
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/hb-adc.cc,v 1.4 1998/06/11 04:54:08 breslau Exp $";
#endif

//Hoeffding Bounds Admission Control

#include "adc.h"
#include <stdlib.h>
#include <math.h>

class HB_ADC : public ADC {
public:
	HB_ADC();
	void teardown_action(int,double,int);
	void rej_action(int,double,int);
protected:
	int admit_flow(int,double,int);
	int rejected_; 
	double epsilon_;
	double sump2_;
};

HB_ADC::HB_ADC() : rejected_(0), sump2_(0)
{
	bind("epsilon_", &epsilon_);
	type_ = new char[3];
	strcpy(type_, "HB");
}


int HB_ADC::admit_flow(int cl,double r,int b)
{
	//get peak rate this class of flow
	double p=peak_rate(cl,r,b);
	if (backoff_) {
		if (rejected_)
			return 0;
	}	
	//printf("Peak rate: %f Avload: %f Rem %f %f %f\n",p,est_[cl]->avload(),sqrt(log(1/epsilon_)*sump2_/2),log(1/epsilon_),sump2_);
	
	if ((p+est_[cl]->avload()+sqrt(log(1/epsilon_)*sump2_/2)) <= bandwidth_) {
		sump2_+= p*p;
		est_[cl]->change_avload(p);
		return 1;
	}
	else {
		rejected_=1;
		return 0;
	}
}


void HB_ADC::rej_action(int cl,double r,int b)
{
  double p=peak_rate(cl,r,b);
  sump2_ -= p*p;
}


void HB_ADC::teardown_action(int cl,double r,int b)
{
	rejected_=0;
	double p=peak_rate(cl,r,b);
	sump2_ -= p*p;
}

static class HB_ADCClass : public TclClass {
public:
	HB_ADCClass() : TclClass("ADC/HB") {}
	TclObject* create(int,const char*const*) {
		return (new HB_ADC());
	}
}class_hb_adc;
