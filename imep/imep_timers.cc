/* 
   imep_timers.cc
   $Id: imep_timers.cc,v 1.1 1999/08/03 04:12:38 yaxu Exp $
   */

#include <imep/imep.h>

#include <packet.h>
#include <ip.h>

#define CURRENT_TIME	Scheduler::instance().clock()

// ======================================================================
// ======================================================================

void
imepTimer::handle(Event *)
{
	busy_ = 0;
	agent->handlerTimer(type_);
}

void
imepTimer::start(double time)
{
	assert(busy_ == 0);
	busy_ = 1;
	Scheduler::instance().schedule(this, &intr, time);
}

void
imepTimer::cancel()
{
	assert(busy_ == 1);
	busy_ = 0;
	Scheduler::instance().cancel(&intr);
}

double
imepTimer::timeLeft()
{
        assert(busy_ == 1);
	return intr.time_ - CURRENT_TIME;
}
