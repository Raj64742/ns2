/*
 * Copyright (c) 2001 University of Southern California.
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
 **
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * This defines the Class for the Quick Start agent. "Agent/QSAgent"
 * 
 * qsagent.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#ifndef _QSAGENT_H
#define _QSAGENT_H

#include <stdarg.h>

#include "object.h"
#include "agent.h"
#include "packet.h"
#include "hdr_qs.h"
#include "timer-handler.h"
#include "lib/bsd-list.h"
#include "queue.h"
#include "delay.h"

class QSAgent;

class QSTimer: public TimerHandler {
public:
	QSTimer(QSAgent * qs_handle) : TimerHandler() { qs_handle_ = qs_handle; }
	virtual void expire(Event * e);

protected:
	QSAgent * qs_handle_;
};

class Agent;
class QSAgent : public Agent{
public:

	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler* callback = 0);

	TclObject * old_classifier_;

	int qs_enabled_;
	int algorithm_; // QS request processing algorithm

	double state_delay_;
	double alloc_rate_;
	double threshold_;
	int max_rate_;
	int mss_;

	/*
	 * 1: Bps = rate * 10 KB
	 * 2: Bps = 2 ^ rate * 1024
	 */
	static int rate_function_;

	QSTimer qs_timer_;

	QSAgent();
	~QSAgent();

	double aggr_approval_;
	double prev_int_aggr_;

protected:
	double process(LinkDelay *link, Queue *queue, double ratereq);
};

#endif // _QSAGENT_H
