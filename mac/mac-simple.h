//
// Copyright (c) 2003 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.

#ifndef ns_mac_simple_h
#define ns_mac_simple_h

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "address.h"
#include "ip.h"

class MacSimpleWaitTimer;
class MacSimpleSendTimer;
class MacSimpleRecvTimer;

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
class EventTrace;


class MacSimple : public Mac {
	//Added by Sushmita to support backoff
	friend class BackoffTimer;
public:
	MacSimple();
	void recv(Packet *p, Handler *h);
	void send(Packet *p, Handler *h);

	void waitHandler(void);
	void sendHandler(void);
	void recvHandler(void);
	double txtime(Packet *p);

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	void trace_event(char *, Packet *);
	int command(int, const char*const*);
	EventTrace *et_;

private:
	Packet *	pktRx_;
	Packet *	pktTx_;
        MacState        rx_state_;      // incoming state (MAC_RECV or MAC_IDLE)
	MacState        tx_state_;      // outgoing state
        int             tx_active_;
	Handler * 	txHandler_;
	MacSimpleWaitTimer *waitTimer;
	MacSimpleSendTimer *sendTimer;
	MacSimpleRecvTimer *recvTimer;

	int busy_ ;


};

class MacSimpleTimer: public Handler {
public:
	MacSimpleTimer(MacSimple* m) : mac(m) {
	  busy_ = 0;
	}
	virtual void handle(Event *e) = 0;
	virtual void start(double time);
	virtual void stop(void);
	inline int busy(void) { return busy_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}

protected:
	MacSimple	*mac;
	int		busy_;
	Event		intr;
	double		stime;
	double		rtime;
	double		slottime;
};

// Timer to use for delaying the sending of packets
class MacSimpleWaitTimer: public MacSimpleTimer {
public: MacSimpleWaitTimer(MacSimple *m) : MacSimpleTimer(m) {}
	void handle(Event *e);
};

//  Timer to use for finishing sending of packets
class MacSimpleSendTimer: public MacSimpleTimer {
public:
	MacSimpleSendTimer(MacSimple *m) : MacSimpleTimer(m) {}
	void handle(Event *e);
};

// Timer to use for finishing reception of packets
class MacSimpleRecvTimer: public MacSimpleTimer {
public:
	MacSimpleRecvTimer(MacSimple *m) : MacSimpleTimer(m) {}
	void handle(Event *e);
};



#endif
