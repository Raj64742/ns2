/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/* tcp-abs.h
 * Copyright (C) 1999 by USC/ISI
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-abs.h,v 1.1 1999/05/13 23:59:59 polly Exp $ (LBL)";
 */

#include "agent.h"
#include "fsm.h"

class AbsTcpAgent;

class AbsTcpTimer : public TimerHandler {
public: 
        AbsTcpTimer(AbsTcpAgent *a) : TimerHandler() { a_ = a; }
protected:
        AbsTcpAgent *a_;
        void expire(Event *e);
};

class AbsTcpAgent : public Agent {
public:
        AbsTcpAgent();
        void timeout();
        void sendmsg(int pktcnt);
        void advanceby(int pktcnt);
	void start();
	void send_batch();
	void drop(int seqno);
	void finish();
	void recv(Packet* pkt, Handler*);
	void output(int seqno);
	inline int& flowid() { return fid_; }
        int command(int argc, const char*const* argv);
protected:
	double rtt_;
	FSMState* current_;
	int offset_;
	int seqno_lb_;                // seqno when finishing last batch
	int connection_size_;
        AbsTcpTimer timer_;
	int rescheduled_;
        void cancel_timer() {
                timer_.force_cancel();
        }
        void set_timer(double time) {
	  	timer_.resched(time);
	}

};


class AbsTcpTahoeAckAgent : public AbsTcpAgent {
public:
        AbsTcpTahoeAckAgent();
};

class AbsTcpRenoAckAgent : public AbsTcpAgent {
public:
        AbsTcpRenoAckAgent();
};

class AbsTcpTahoeDelAckAgent : public AbsTcpAgent {
public:
        AbsTcpTahoeDelAckAgent();
};

class AbsTcpRenoDelAckAgent : public AbsTcpAgent {
public:
        AbsTcpRenoDelAckAgent();
};

//AbsTcpSink
class AbsTcpSink : public Agent {
public:
        AbsTcpSink();
        void recv(Packet* pkt, Handler*);
protected:
};


class AbsDelAckSink;

class AbsDelayTimer : public TimerHandler {
public:
        AbsDelayTimer(AbsDelAckSink *a) : TimerHandler() { a_ = a; }
protected:
        void expire(Event *e);
        AbsDelAckSink *a_;
};

class AbsDelAckSink : public AbsTcpSink {
public:
        AbsDelAckSink();
        void recv(Packet* pkt, Handler*);
        void timeout();
protected:
        Packet* save_;          /* place to stash packet while delaying */
        double interval_;
        AbsDelayTimer delay_timer_;
};

// Dropper Class
class Dropper {
public:
	AbsTcpAgent* agent_;
        Dropper* next_;
};

class DropTargetAgent : public Connector {
public:
        DropTargetAgent();
        void recv(Packet* pkt, Handler*);
        void insert_tcp(AbsTcpAgent* tcp);
	static DropTargetAgent& instance() {
		return (*instance_);	       // general access to TahoeAckFSM
	}
protected:
	Dropper* dropper_list_;
	static DropTargetAgent* instance_;
};
