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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/adc.h,v 1.5 1999/02/12 22:01:31 breslau Exp $
 */

#ifndef ns_adc_h
#define ns_adc_h

#include "estimator.h"

#define CLASS 10
class ADC : public NsObject {
public:
	ADC();
	int command(int,const char*const*);
	virtual int admit_flow(int cl,double r, int b)=0;
	virtual void rej_action(int,double,int){}; 
	virtual void teardown_action(int,double,int){}; 
	inline void recv(Packet*, Handler *){} 
	inline void setest(int cl,Estimator *est) {est_[cl]=est;
						 est_[cl]->setactype(type_);}
	double peak_rate(int cl,double r,int b) {return r+b/est_[cl]->period();}
	char *type() {return type_;}
protected:
	Estimator *est_[CLASS];
	double bandwidth_;
	char *type_;
	Tcl_Channel tchan_;
	int src_;
	int dst_;
	int backoff_;
	int dobump_;
};

#endif





