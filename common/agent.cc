/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/agent.cc,v 1.37 1998/04/07 23:53:52 haldar Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <string.h>

#include "tclcl.h"
#include "agent.h"
#include "packet.h"
#include "ip.h"
#include "flags.h"
#include "address.h"


#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static class AgentClass : public TclClass {
public:
	AgentClass() : TclClass("Agent") {} 
	TclObject* create(int, const char*const*) {
		return (new Agent(-1));
	}
} class_agent;

int Agent::uidcnt_;		/* running unique id */

Agent::Agent(int pkttype) : 
	addr_(-1), dst_(-1), size_(0), type_(pkttype), fid_(-1),
	prio_(-1), flags_(0), defttl_(32), channel_(0), traceName_(NULL),
	oldValueList_(NULL)
{
	/*
	 * the following is a workaround to allow
	 * older scripts that use "class_" instead of
	 * flowid to work -K
	 */
	bind("class_", (int*)&fid_);

//	memset(pending_, 0, sizeof(pending_));
	// this is really an IP agent, so set up
	// for generating the appropriate IP fields...
	bind("addr_", (int*)&addr_);
	bind("dst_", (int*)&dst_);
	bind("fid_", (int*)&fid_);
	bind("prio_", (int*)&prio_);
	bind("flags_", (int*)&flags_);
	bind("ttl_", &defttl_);

	bind("off_ip_", &off_ip_);
}

Agent::~Agent()
{
	if (oldValueList_ != NULL) {
		OldValue *p;
		while (oldValueList_ != NULL) {
			oldValueList_ = oldValueList_->next_;
			delete p;
			p = oldValueList_; 
		}
	}
}

int Agent::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (strcmp(argv[1], "attach") == 0) {
		int mode;
		const char* id = argv[2];
		channel_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
		if (channel_ == 0) {
			tcl.resultf("trace: can't attach %s for writing", id);
			return (TCL_ERROR);
		}
		return (TCL_OK);
	} else if (strcmp(argv[1], "add-agent-trace") == 0) {
		// we need to write nam traces and set agent trace name
		if (channel_ == 0) {
			tcl.resultf("agent %s: no trace file attached", name_);
			return (TCL_OK);
		}
		addAgentTrace(argv[2]);
		return (TCL_OK);
	} else if (strcmp(argv[1], "delete-agent-trace") == 0) {
		if ((traceName_ == 0) || (channel_ == 0))
			return (TCL_OK);
		deleteAgentTrace();
		return (TCL_OK);
	} else if (strcmp(argv[1], "show-monitor") == 0) {
		if ((traceName_ == 0) || (channel_ == 0))
			return (TCL_OK);
		monitorAgentTrace();
		return (TCL_OK);
	} else if (strcmp(argv[1], "tracevar") == 0) {
		// wrapper of TclObject's trace command, because some tcl
		// agents (e.g. srm) uses it.
		const char *args[4];
		args[0] = argv[0];
		args[1] = "trace";
		args[2] = argv[2];
		if (argc > 3)
			args[3] = argv[3];
		return (Connector::command(argc, args));
	}
	return (Connector::command(argc, argv));
}

void Agent::flushAVar(TracedVar *v)
{
	char wrk[256], value[128];
	int n;

	// XXX we need to keep track of old values. What's the best way?
	v->value(value);
	if (strcmp(value, "") == 0) 
		// no value, because no writes has occurred to this var
		return;
	sprintf(wrk, "f -t %.17f -s %d -d %d -n %s -a %s -o %s -T v -x",
		Scheduler::instance().clock(),
		addr_ >> (Address::instance().NodeShift_[1]),
		dst_ >> (Address::instance().NodeShift_[1]),
		v->name(),
		traceName_,
		value);
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	(void)Tcl_Write(channel_, wrk, n+1);
}

void Agent::deleteAgentTrace()
{
	char wrk[256];
	int n;

	// XXX we don't know InstVar outside of Tcl! Is there any
	// tracedvars hidden in InstVar? If so, shall we have a tclclInt.h?
	TracedVar* var = tracedvar_;
	for ( ;  var != 0;  var = var->next_) 
		flushAVar(var);

	// we need to flush all var values to trace file, 
	// so nam can do backtracing
	sprintf(wrk, "a -t %.17f -s %d -d %d -n %s -x",
		Scheduler::instance().clock(),
		addr_ >> (Address::instance().NodeShift_[1]),
		dst_ >> (Address::instance().NodeShift_[1]),
		traceName_);
	if (traceName_ != NULL)
		delete[] traceName_;
	traceName_ = NULL;
}

OldValue* Agent::lookupOldValue(TracedVar *v)
{
	OldValue *p = oldValueList_;
	while ((p != NULL) && (p->var_ != v))
		p = p->next_;
	return p;
}

void Agent::insertOldValue(TracedVar *v, const char *value)
{
	OldValue *p = new OldValue;
	assert(p != NULL);
	strncpy(p->val_, value, min(strlen(value)+1, TRACEVAR_MAXVALUELENGTH));
	p->var_ = v;
	p->next_ = NULL;
	if (oldValueList_ == NULL) 
		oldValueList_ = p;
	else {
		p->next_ = oldValueList_;
		oldValueList_ = p;
	}
}

// callback from traced variable updates
void Agent::trace(TracedVar* v) 
{
	if (channel_ == 0)
		return;
	char wrk[256], value[128];
	int n;

	// XXX we need to keep track of old values. What's the best way?
	v->value(value);

	// XXX hack: how do I know ns has not started yet?
	// if there's nothing in value, return
	if (value[0] == 0) 
		return;

	OldValue *ov = lookupOldValue(v);
	if (ov != NULL) {
		sprintf(wrk, "f -t %.17f -s %d -d %d -n %s -a %s -v %s -o %s -T v",
			Scheduler::instance().clock(),
			addr_ >> (Address::instance().NodeShift_[1]),
			dst_ >> (Address::instance().NodeShift_[1]),
			v->name(),
			traceName_,
			value,
			ov->val_);
		strncpy(ov->val_, 
			value,
			min(strlen(value)+1, TRACEVAR_MAXVALUELENGTH));
	} else {
		// if there is value, insert it into old value list
		sprintf(wrk, "f -t %.17f -s %d -d %d -n %s -a %s -v %s -T v",
			Scheduler::instance().clock(),
			addr_ >> (Address::instance().NodeShift_[1]),
			dst_ >> (Address::instance().NodeShift_[1]),
			v->name(),
			traceName_,
			value);
		insertOldValue(v, value);
	}
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	(void)Tcl_Write(channel_, wrk, n+1);
}

void Agent::monitorAgentTrace()
{
	char wrk[256];
	int n;
	double curTime = (&Scheduler::instance() == NULL ? 0 : 
			  Scheduler::instance().clock());
	
	sprintf(wrk, "v -t %.17f monitor_agent %d %s",
		curTime,
		addr_ >> (Address::instance().NodeShift_[1]),
		traceName_);
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
}

void Agent::addAgentTrace(const char *name)
{
	char wrk[256];
	int n;
	double curTime = (&Scheduler::instance() == NULL ? 0 : 
			  Scheduler::instance().clock());
	
	sprintf(wrk, "a -t %.17f -s %d -d %d -n %s",
		curTime,
		addr_ >> (Address::instance().NodeShift_[1]),
		dst_ >> (Address::instance().NodeShift_[1]),
			 name);
	n = strlen(wrk);
	wrk[n] = '\n';
	wrk[n+1] = 0;
	if (channel_)
		(void)Tcl_Write(channel_, wrk, n+1);
	// keep agent trace name
	if (traceName_ != NULL)
		delete[] traceName_;
	traceName_ = new char[strlen(name)+1];
	strcpy(traceName_, name);
}

void Agent::timeout(int)
{
}

void Agent::recv(Packet* p, Handler*)
{
	/*
	 * didn't expect packet (or we're a null agent?)
	 */
	Packet::free(p);
}

/*
 * allocate a packet and fill in all the generic fields
 */
Packet* Agent::allocpkt() const
{
	Packet* p = Packet::alloc();

	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	ch->uid() = uidcnt_++;
	ch->ptype() = type_;
	ch->size() = size_;
	ch->timestamp() = Scheduler::instance().clock();
	ch->iface() = -2;	/* XXX arbitrary */
	ch->ref_count() = 0;	/* reference count */
	ch->error() = 0;	/* pkt not corrupt to start with */

	hdr_ip* iph = (hdr_ip*)p->access(off_ip_);
	iph->src() = addr_;
	iph->dst() = dst_;
	iph->flowid() = fid_;
	iph->prio() = prio_;
	iph->ttl() = defttl_;

	hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
	hf->ecn_capable_ = 0;
	hf->ecn_ = 0;
	hf->eln_ = 0;
	hf->ecn_to_echo_ = 0;
	hf->fs_ = 0;
	hf->no_ts_ = 0;
	hf->pri_ = 0;
	hf->usr1_ = 0;
	hf->usr2_ = 0;

	return (p);
}

/* allocate a packet and fill in all the generic fields and allocate
 * a buffer of n bytes for data
 */
Packet* Agent::allocpkt(int n) const
{
        Packet* p = allocpkt();

	if (n > 0)
	        p->allocdata(n);

	return(p);
}
