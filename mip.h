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
};

class MIPEncapsulator : public Connector {
public:
	MIPEncapsulator();
	void recv(Packet *p, Handler *h);
protected:
	nsaddr_t addr_;
	int mask_;
	int shift_;
	int off_ip_;
	int off_ipinip_;
	int defttl_;
};

class MIPDecapsulator : public AddressClassifier {
public:
	MIPDecapsulator();
	void recv(Packet* p, Handler* h);
protected:
	int off_ipinip_;		/* XXX to be removed */
	int off_ip_;
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
	void recv(Packet *, Handler *);
	void timeout(int);
protected:
	int command(int argc, const char*const*argv);
	void send_ads(int dst = -1, NsObject *target = NULL);
	double beacon_; /* beacon period */
	NsObject *bcast_target_; /* where to send out ads */
	SimpleTimer timer_;
	int mask_;
	int shift_;
#ifndef notdef
	int seqno_;		/* current ad seqno */
#endif
	double adlftm_;	/* ads lifetime */
	int off_mip_;
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
	int off_mip_;
};

#endif
