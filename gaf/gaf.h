// Copyright (c) 2000 by the University of Southern California
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
//

// Header file for grid-based adaptive fidelity algorithm 

#ifndef ns_gaf_h
#define ns_gaf_h

#include <assert.h>
#include "agent.h"
#include "node.h"

#define MAXQUEUE  1             // 1 = only my grid, 5 = all of my neighbos
#define GN_UPDATE_INTERVAL 10 	// how often to update neighbor list
#define GAF_STARTUP_JITTER 1.0	// secs to jitter start of periodic activity
#define GAF_NONSTART_JITTER 3.0
#define MIN_DISCOVERY_TIME 1    // min interval to send discovery
#define MAX_DISCOVERY_TIME 15   // max interval to send discovery
#define MIN_SELECT_TIME 5	// start to tell your neighbor that
#define MAX_SELECT_TIME 6	// you are the leader b/w, my need
				// to be set by apps. 
#define MAX_DISCOVERY   10      // send selection after 10 time try
#define MIN_LIFETIME    60
#define GAF_LEADER_JITTER 3
#define MIN_TURNOFFTIME 1

class GAFAgent;

typedef enum {
  GAF_DISCOVER, GAF_SELECT, GAF_DUTY
} GafMsgType;

typedef enum {
  GAF_FREE, GAF_LEADER, GAF_SLEEP
} GafNodeState;

/*
 * data structure for exchanging existence message and selection msg
 */

struct DiscoveryMsg {
        u_int32_t gid;	// grid id
        u_int32_t nid;  // node id
        u_int32_t state; // what is my state
        u_int32_t ttl;  // My time to live
        u_int32_t stime;  // I may stay on this grid for only stime

};


// gaf header

struct hdr_gaf {
	// needs to be extended

	int seqno_;
        GafMsgType type_;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_gaf* access(const Packet* p) {
		return (hdr_gaf*) p->access(offset_);
	}
};


// GAFTimer is used for discovery phase

class GAFDiscoverTimer : public TimerHandler {
public: 
	GAFDiscoverTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
	virtual void expire(Event *);
	GAFAgent *a_;
};

// GAFSelectTimer is for the slecting phase

class GAFSelectTimer : public TimerHandler {
public:
        GAFSelectTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
        virtual void expire(Event *);
        GAFAgent *a_;
};

// GAFDutyTimer is for duty cycle. It is the place of adaptive fidelity
// plays

class GAFDutyTimer : public TimerHandler {
public:
        GAFDutyTimer(GAFAgent *a) : TimerHandler(), a_(a) { }
protected:
        inline void expire(Event *);
        GAFAgent *a_;
};

class GAFAgent : public Agent {
public:
	GAFAgent(nsaddr_t id);
	virtual void recv(Packet *, Handler *);
	void timeout(GafMsgType);
	//void select_timeout(int);

	u_int32_t nodeid() {return nid_;}
	double myttl();
		
protected:
	int command(int argc, const char*const*argv);
	

	void node_on();
	void node_off();
	void duty_timeout();
	void send_discovery();
	void makeUpDiscoveryMsg(Packet *p);
	void processDiscoveryMsg(Packet *p);
	void schedule_wakeup(struct DiscoveryMsg);
	double beacon_; /* beacon period */
	void setGAFstate(GafNodeState);
	int randomflag_;
	GAFDiscoverTimer timer_;
	GAFSelectTimer stimer_;
	GAFDutyTimer dtimer_;
	int seqno_;
	int gid_;	// group id of this node
	int nid_;	// the node id of this node belongs to.
	Node *thisnode; // the node object where this agent resides
	int maxttl_;	// life of a node in the neighbor list
	
	GafNodeState	state_;
	int leader_settime_;
	int adapt_mobility_;  // control the use of 
	                      // GAF-3: load balance with aggressive sleeping
	                      // GAF-4:  load 3 + mobility adaption
};

/* assisting getting broadcast msg */

class GAFPartner : public Connector {
public:
        GAFPartner();
        void recv(Packet *p, Handler *h);
	
protected:
	int command(int argc, const char*const*argv);
        ns_addr_t here_;
	int gafagent_;
        int mask_;
        int shift_;
};



#endif
