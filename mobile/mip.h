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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mobile/mip.h,v 1.7 2000/09/01 03:04:06 haoboy Exp $

/*
 * Copyright (c) Sun Microsystems, Inc. 1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS
 * SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

/* Ported by Ya Xu to official ns release  Jan. 1999 */


#ifndef ns_mip_h
#define ns_mip_h

#include <assert.h>
#include "agent.h"
#include "classifier-addr.h"

#define MIP_TIMER_SIMPLE	0
#define MIP_TIMER_AGTLIST	1

struct hdr_ipinip {
	hdr_ip hdr_;
	struct hdr_ipinip *next_;	// XXX multiple encapsulation OK

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_ipinip* access(const Packet* p) {
		return (hdr_ipinip*) p->access(offset_);
	}
};

typedef enum {
	MIPT_REG_REQUEST, MIPT_REG_REPLY, MIPT_ADS, MIPT_SOL
} MipRegType;

struct hdr_mip {
	int haddr_;
	int ha_;
	int coa_;
	MipRegType type_;
	double lifetime_;
	int seqno_;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_mip* access(const Packet* p) {
		return (hdr_mip*) p->access(offset_);
	}
};

class MIPEncapsulator : public Connector {
public:
	MIPEncapsulator();
	void recv(Packet *p, Handler *h);
protected:
	ns_addr_t here_;
	int mask_;
	int shift_;
	int defttl_;
};

class MIPDecapsulator : public AddressClassifier {
public:
	MIPDecapsulator();
	void recv(Packet* p, Handler* h);
};

class SimpleTimer : public TimerHandler {
public: 
	SimpleTimer(Agent *a) : TimerHandler() { a_ = a; }
protected:
	inline void expire(Event *) { a_->timeout(MIP_TIMER_SIMPLE); }
	Agent *a_;
};

class MIPBSAgent : public Agent {
public:
	MIPBSAgent();
	virtual void recv(Packet *, Handler *);
	void timeout(int);
protected:
	int command(int argc, const char*const*argv);
	void send_ads(int dst = -1, NsObject *target = NULL);
	void sendOutBCastPkt(Packet *p);
	double beacon_; /* beacon period */
	NsObject *bcast_target_; /* where to send out ads */
	NsObject *ragent_;     /* where to send reg-replies to MH */
	SimpleTimer timer_;
	int mask_;
	int shift_;
#ifndef notdef
	int seqno_;		/* current ad seqno */
#endif
	double adlftm_;	/* ads lifetime */
};

class MIPMHAgent;

class AgtListTimer : public TimerHandler {
public: 
	AgtListTimer(MIPMHAgent *a) : TimerHandler() { a_ = a; }
protected:
	void expire(Event *e);
	MIPMHAgent *a_;
};

typedef struct _agentList {
	int node_;
	double expire_time_;
	double lifetime_;
	struct _agentList *next_;
} AgentList;

class MIPMHAgent : public Agent {
public:
	MIPMHAgent();
	void recv(Packet *, Handler *);
	void timeout(int);
protected:
	int command(int argc, const char*const*argv);
	void reg();
	void send_sols();
	void sendOutBCastPkt(Packet *p);
	int ha_;
	int coa_;
	double reg_rtx_;
	double beacon_;
	NsObject *bcast_target_; /* where to send out solicitations */
	AgentList *agts_;
	SimpleTimer rtx_timer_;
	AgtListTimer agtlist_timer_;
	int mask_;
	int shift_;
#ifndef notdef
	int seqno_;		/* current registration seqno */
#endif
	double reglftm_;	/* registration lifetime */
	double adlftm_;		/* current ads lifetime */
	MobileNode *node_;      /* ptr to my mobilenode,if appl. */

};

#endif
