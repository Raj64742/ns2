#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>
 
// #define DEBUG
//#include <debug.h>
#include <arp.h>
#include <new-ll.h>
#include <new-mac.h>
#include <mac-timers.h>
#include <new-mac-802_11.h> 

/*
 * Force timers to expire on slottime boundries.
 */
// #define USE_SLOT_TIME

#define ROUND_TIME()	\
	{								\
		assert(slottime);					\
		double rmd = remainder(s.clock() + rtime, slottime);	\
		if(rmd > 0.0)						\
			rtime += (slottime - rmd);			\
		else							\
			rtime += (-rmd);				\
	}


/* ======================================================================
   Timers
   ====================================================================== */

void
MacTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}

void
MacTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);

	if(paused_ == 0)
		s.cancel(&intr);

	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
}

/* ======================================================================
   Defer Timer
   ====================================================================== */
void
DeferTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = time;
#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}


void    
DeferTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->deferHandler();
}

/* ======================================================================
   NAV Timer
   ====================================================================== */
void    
NavTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->navHandler();
}


/* ======================================================================
   Receive Timer
   ====================================================================== */
void    
RxTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->recvHandler();
}


/* ======================================================================
   Send Timer
   ====================================================================== */
void    
TxTimer::handle(Event *)
{       
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->sendHandler();
}


/* ======================================================================
   Interface Timer
   ====================================================================== */
void
IFTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;

	mac->txHandler();
}


/* ======================================================================
   Backoff Timer
   ====================================================================== */
void
BackoffTimer::handle(Event *)
{
	busy_ = 0;
	paused_ = 0;
	stime = 0.0;
	rtime = 0.0;
	difs_wait = 0.0;

	mac->backoffHandler();
}

void
BackoffTimer::start(int cw, int idle)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	paused_ = 0;
	stime = s.clock();
	rtime = (Random::random() % cw) * mac->phymib_->SlotTime;
#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	difs_wait = 0.0;

	if(idle == 0)
		paused_ = 1;
	else {
		assert(rtime >= 0.0);
		s.schedule(this, &intr, rtime);
	}
}


void
BackoffTimer::pause()
{
	Scheduler &s = Scheduler::instance();
	int slots = (int) ((s.clock() - (stime + difs_wait)) / mac->phymib_->SlotTime);
	if(slots < 0)
		slots = 0;
	assert(busy_ && ! paused_);

	paused_ = 1;
	rtime -= (slots * mac->phymib_->SlotTime);
	assert(rtime >= 0.0);

	difs_wait = 0.0;

	s.cancel(&intr);
}


void
BackoffTimer::resume(double difs)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ && paused_);

	paused_ = 0;
	stime = s.clock();

	/*
	 * The media should be idle for DIFS time before we start
	 * decrementing the counter, so I add difs time in here.
	 */
	difs_wait = difs;

#ifdef USE_SLOT_TIME
	ROUND_TIME();
#endif
	assert(rtime + difs_wait >= 0.0);
	s.schedule(this, &intr, rtime + difs_wait);
}


