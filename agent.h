/*
 * Copyright (c) 1993-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/agent.h,v 1.14 1997/09/12 01:31:22 haoboy Exp $ (LBL)
 */

#ifndef ns_agent_h
#define ns_agent_h

#include "config.h"
#include "packet.h"
#include "connector.h"
#include "timer-handler.h"
#include <tracedvar.h>
#include <assert.h>

#define TIMER_IDLE 0
#define TIMER_PENDING 1

/* 
 * Note that timers are now implemented using timer-handler.{cc,h}
 */

#define TRACEVAR_MAXVALUELENGTH 128

// store old value of traced vars
// work only for TracedVarTcl
struct OldValue {
	TracedVar *var_;
	char val_[TRACEVAR_MAXVALUELENGTH];
	struct OldValue *next_;
};

class Agent : public Connector {
 public:
	Agent(int pktType);
	virtual ~Agent();
	virtual void timeout(int tno);
 protected:
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);
	Packet* allocpkt() const;
	Packet* allocpkt(int) const;
        Tcl_Channel channel_;

	nsaddr_t addr_;		/* address of this agent */
	nsaddr_t dst_;		/* destination address for pkt flow */
	int size_;		/* fixed packet size */
	int type_;		/* type to place in packet header */
	int fid_;		/* for IPv6 flow id field */
	int prio_;		/* for IPv6 prio field */
	int flags_;		/* for experiments (see ip.h) */
	int defttl_;		/* default ttl for outgoing pkts */

#ifdef notdef
int seqno_;		/* current seqno */
int class_;		/* class to place in packet header */
#endif

	static int uidcnt_;
	int off_ip_;

	char *traceName_;        // name used in agent traces
	OldValue *oldValueList_; 
	virtual void trace(TracedVar *v);
	void deleteAgentTrace();
	void addAgentTrace(const char *name);
	OldValue* lookupOldValue(TracedVar *v);
	void insertOldValue(TracedVar *v, const char *value);
 private:
	void flushAVar(TracedVar *v);
};

// Local Variables:
// mode:c++
// End:

#endif
