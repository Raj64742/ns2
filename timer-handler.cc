/*
 * timer-handler.cc
 * Copyright (C) 1997 by USC/ISI
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/timer-handler.cc,v 1.2 1997/07/23 05:16:51 kfall Exp $ (USC/ISI)";
#endif

#include <stdlib.h>  // abort()
#include "timer-handler.h"

void
TimerHandler::cancel()
{
	if (status_ != TIMER_PENDING)
		abort();
	_cancel();
	status_ = TIMER_IDLE;
}

void
TimerHandler::sched(double delay)
{
	if (status_ != TIMER_IDLE)
		abort();
	_sched(delay);
	status_ = TIMER_PENDING;
}

void
TimerHandler::resched(double delay)
{
	if (status_ == TIMER_PENDING)
		_cancel();
	_sched(delay);
	status_ = TIMER_PENDING;
}

void
TimerHandler::handle(Event *e)
{
	if (status_ != TIMER_PENDING)   // sanity check
		abort();
	status_ = TIMER_HANDLING;
	expire(e);
	// if it wasn't rescheduled, it's done
	if (status_ == TIMER_HANDLING)
		status_ = TIMER_IDLE;
}
