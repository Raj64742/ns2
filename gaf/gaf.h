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
#define GN_UPDATE_INTERVAL 20 	// how often to update neighbor list
#define GAF_STARTUP_JITTER 1.0	// secs to jitter start of periodic activity
#define GAF_BROADCAST_JITTER 0.1 // jitter for all broadcast packet
#define MIN_DISCOVERY_TIME 1    // min interval to send discovery
#define MAX_DISCOVERY_TIME 15   // max interval to send discovery
#define MIN_SELECT_TIME 5	// start to tell your neighbor that
#define MAX_SELECT_TIME 6	// you are the leader b/w, my need
				// to be set by apps. 
#define MAX_DISCOVERY   10      // send selection after 10 time try
class GAFAgent;

/*
 * data structure for storing neighborhood infomation
 * You may find a similar structure at energy-model.h
 * But it is not good to put such things there.
 */

struct gaf_neighbor_item {
       u_int32_t nid;        		// node id
       int       ttl;    		// time-to-live
       gaf_neighbor_item *next; 	// pointer to next item
};

struct gaf_neighbor {
       u_int32_t  gid;   // grid id
       gaf_neighbor_item *head; 
};

typedef enum {
  GAF_DISCOVER, GAF_SELECT, GAF_DUTY
} GafMsgType;

typedef enum {
	GAF_TRAFFIC, GAF_LEADER, GAF_FREE, GAF_SLEEP
} GafNodeState;

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

class GafNeighborshipHandler : public Handler {
public:
        GafNeighborshipHandler(GAFAgent *gaf) {
                gaf_ = gaf;
        }
        virtual void start();
        virtual void handle(Event *e);
protected:
        GAFAgent *gaf_;
        Event  intr;
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
	void scanGn();
		
protected:
	int command(int argc, const char*const*argv);
	void setGn(double,double);
	void node_on();
	void node_off();
	void duty_timeout();
	void send_selection();
	void makeSelectionMsg(Packet *p);
	void send_discovery(int dst = -1);
	void makeUpExistenceMsg(Packet *p);
	void processSelectionMsg(Packet *p);
	void processExistenceMsg(Packet *p);
	void insertGn(gaf_neighbor *gridp, u_int32_t nid);
	void releaseGn(gaf_neighbor *gridp);
	void releaseGnItem(gaf_neighbor *gridp, u_int32_t nid);
	void printGn();
	double beacon_; /* beacon period */
	int randomflag_;
	GAFDiscoverTimer timer_;
	GAFSelectTimer stimer_;
	GAFDutyTimer dtimer_;
	int seqno_;
	int gid_;	// group id of this node
	int nid_;	// the node id of this node belongs to.
	Node *thisnode; // the node object where this agent resides
	struct gaf_neighbor gn_[5]; // keep neighbor info around me
	                            // gn_[0] = my grid; 1-4: T,B,L,R
	GafNeighborshipHandler *gnhandler_;
	int maxttl_;	// life of a node in the neighbor list
	
	GafNodeState	state_;
	int total_exist_cnt_;
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

/*
 * data structure for exchanging existence message and selection msg
 */

struct ExistenceMsg {
        u_int32_t gid;	// grid id
        u_int32_t nid;  // node id
};

struct SelectionMsg {
        u_int32_t gid;  // grid id
        u_int32_t nid;  // node id
        u_int32_t ttl;  // estimated live life (if keeping from now on
};




#endif
