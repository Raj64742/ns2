
/*
 * agent-timer.h
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

#ifndef agent_timer_h
#define agent_timer_h

/*
 * Abstract base class to deal with Agent-style timers.
 *
 * To define a new, subclass this function and define handle:
 *
 * class MyTimer : private AgentTimerHandler {
 * public:
 *	MyTimer(MyAgentClass *a) : AgentTimerHandler() { a_ = a; }
 *	virtual void handle(Event *e);
 * protected:
 *	MyAgentClass *a_;
 * };
 *
 * Then define handle:
 *
 * void
 * MyTimer::handle(Event *e)
 * {
 * 	// do the work, then call resched(newdelay) or handled()
 * }
 *
 * Often times MyTimer will be a friend of MyAgentClass.
 *
 * See tcp-rbp.{cc,h} for an example.
 */
class AgentTimerHandler : public Handler {
public:
	AgentTimerHandler() : pending_(0) { }

	void sched(double delay);    // cannot be pending
	void resched(double delay);  // may be pending
	void cancel();               // must be pending
	int pending() { return pending_; };

	virtual void handle(Event *e);  // must be filled in by client
		// handle should conclude by calling
		// either resched() or handled()
	void handled() { pending_ = 0; }

protected:
	int pending_;
	Event event_;

private:
	inline void _sched(double delay) {
		(void)Scheduler::instance().schedule(this, &event_, delay);
	}
	inline void _cancel() {
		(void)Scheduler::instance().cancel(&event_);
	}
};

#endif /* agent_timer_h */
