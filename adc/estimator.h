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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/estimator.h,v 1.5 1998/08/22 02:41:02 haoboy Exp $
 */

//Estimator unit estimates average load every period interval of time

#ifndef ns_estimator_h
#define ns_estimator_h

#include "connector.h"
#include "measuremod.h"
#include "timer-handler.h"

class Estimator;

class Estimator_Timer : public TimerHandler {
public:
	Estimator_Timer(Estimator *est) : TimerHandler() { est_ = est;}
	
protected:
	virtual void expire(Event *e);
	Estimator *est_;
};

class Estimator : public NsObject {
public:
	Estimator();
	inline double avload() { return double(avload_);};

	inline virtual void change_avload(double incr) { avload_ += incr;}
	int command(int argc, const char*const* argv); 
	virtual void timeout(int);
	inline void recv(Packet *,Handler *){}
	void start();
	void stop();
	void setmeasmod(MeasureMod *);
	void setactype(const char*);
	inline double &period(){ return period_;}
	void trace(TracedVar* v);
protected:
	MeasureMod *meas_mod_;
	TracedDouble avload_;
	double period_; 
	virtual void estimate()=0;
	Estimator_Timer est_timer_;
	TracedDouble measload_;
	Tcl_Channel tchan_;
	int src_;
	int dst_;
	double omeasload_;
	double oavload_;
	char *actype_;
};

#endif
