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

#ifndef ns_tbf_h
#define ns_tbf_h

#include "connector.h"
#include "timer-handler.h"

class TBF;

class TBF_Timer : public TimerHandler {
public:
	TBF_Timer(TBF *t) : TimerHandler() { tbf_ = t;}
	
protected:
	virtual void expire(Event *e);
	TBF *tbf_;
};


class TBF : public Connector {
public:
	TBF();
	~TBF();
	void timeout(int);
protected:
	void recv(Packet *, Handler *);
	double getupdatedtokens();
	double tokens_; //acumulated tokens
	double rate_; //token bucket rate
	int bucket_; //bucket depth
	int qlen_;
	double lastupdatetime_;
	PacketQueue *q_;
	TBF_Timer tbf_timer_;
	int init_;
};

#endif
