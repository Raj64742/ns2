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
 */

#ifndef ns_sa_h
#define ns_sa_h

#include "resv.h"
#include "agent.h"
#include "timer-handler.h"
#include "trafgen.h"
#include "rtp.h"
#include "random.h"

class SA_Agent;

class SA_Timer : public TimerHandler {
public:
        SA_Timer(SA_Agent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        SA_Agent *a_;
};


class SA_Agent : public Agent {
public:
	SA_Agent();
	~SA_Agent();
	int command(int, const char*const*);
	virtual void timeout(int);
	virtual void sendmsg(int nbytes, const char *flags = 0);
protected:
	void start(); 
	void stop(); 
	void sendreq();
	void sendteardown();
	void recv(Packet *, Handler *);
        void stoponidle(const char *);
	virtual void sendpkt();
	double rate_;
	int bucket_;
	NsObject* ctrl_target_;
        TrafficGenerator *trafgen_;
        int rtd_;  /* ready-to-die: waiting for last burst to end */
	char* callback_;

	SA_Timer sa_timer_;
	double nextPkttime_;
	int running_;
	int seqno_;
	int size_;
};


#endif
