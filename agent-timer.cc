
/*
 * agent-timer.cc
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


#include "agent.h"


void
AgentTimerHandler::cancel()
{
	if (!pending_)
		abort();
	pending_ = 0;
	_cancel();
}

void
AgentTimerHandler::sched(double delay)
{
	if (pending_)
		abort();
	pending_ = 1;
	_sched(delay);
}

void
AgentTimerHandler::resched(double delay)
{
	if (pending_)
		_cancel();
	_sched(delay);
}

void
AgentTimerHandler::handle(Event *e)
{
	/*
	 * must be overridden
	 *
	 * handlers should either call cancel() or resched().
	 */
	abort();  // must be overridden

}
