/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research
 *	Group at the University of California Berkeley.
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
 * Contributed by the Daedalus Research Group, UC Berkeley 
 * (http://daedalus.cs.berkeley.edu)
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/errmodel.h,v 1.34.2.1 1998/08/10 19:49:26 yuriy Exp $ (UCB)
 */

#ifndef ns_errmodel_h
#define ns_errmodel_h

#include "connector.h"
#include "ranvar.h"


enum ErrorUnit { EU_TIME=0, EU_BYTE, EU_PKT };
#define EU_NAMES "time", "byte", "pkt"
#define STR2EU(s) (!strcmp(s,"time") ? EU_TIME : (!strcmp(s,"byte") ? EU_BYTE : EU_PKT))

#define EM_GOOD	1
#define EM_BAD	2

/* 
 * Basic object for error models.  This can be used unchanged by error 
 * models that are characterized by a single parameter, the rate of errors 
 * (or equivalently, the mean duration/spacing between errors).  Currently,
 * this includes the uniform and exponentially-distributed models.
 */
class ErrorModel : public Connector {
public:
	ErrorModel();
	virtual void recv(Packet*, Handler*);
	virtual void reset();
	virtual int corrupt(Packet*);
	virtual int CorruptPkt(Packet*);
	virtual int CorruptTime(Packet*);
	virtual int CorruptByte(Packet*);
	inline double rate() { return rate_; }
	inline ErrorUnit unit() { return unit_; }

protected:
	virtual int command(int argc, const char*const* argv);
	double PktLength(Packet*);

	int enable_;		// true if this error module is turned on
	int markecn_;		// mark ecn instead of dropping on corruption?
	int firstTime_;		// to not corrupt first packet in byte model
	ErrorUnit unit_;	// error unit in pkts, bytes, or time
	double rate_;		// uniform error rate in pkt or byte
	double bandwidth_;	// bandwidth of the link
	RandomVariable *ranvar_;// the underlying random variate generator
	Event intr_;		// set callback to queue
};


class TwoStateErrorModel : public ErrorModel {
public:
	TwoStateErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
	int state_;		// state: 0=error-free, 1=error
	double remainLen_;	// remaining length of the current state
	RandomVariable *ranvar_[2]; // ranvar staying length for each state
};


class MultiStateErrorModel : public ErrorModel {
public:
	MultiStateErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
	ErrorModel* em_;	// current error model to use
};


/*
 * periodic packet drops (drop every nth packet we see)
 * this can be conveniently combined with a flow-based classifier
 * to achieve drops in particular flows
 */
class PeriodicErrorModel : public ErrorModel {
public:
	PeriodicErrorModel();
	int corrupt(Packet*);
protected:
	int cnt_;
	double period_;
	double offset_;
	double burstlen_;
	double last_time_;
	double first_time_;
};

/*
 * List error model: specify which packets to drop in tcl
 */
class ListErrorModel : public ErrorModel {
public:
	ListErrorModel() : cnt_(0), droplist_(NULL),
		dropcnt_(0), cur_(0) { }
	~ListErrorModel() { if (droplist_) delete droplist_; }
	int corrupt(Packet*);
	int command(int argc, const char*const* argv);
protected:
	int parse_droplist(int argc, const char *const*);
	static int nextval(const char*&p);
	static int intcomp(const void*, const void*);		// for qsort
	int cnt_;	/* cnt of pkts/bytes we've seen */
	int* droplist_;	/* array of pkt/byte #s to affect */
	int dropcnt_;	/* # entries in droplist_ total */
	int cur_;	/* current index into droplist_ */
};

/* For Selective packet drop */
class SelectErrorModel : public ErrorModel {
public:
	SelectErrorModel();
	virtual int corrupt(Packet*);
protected:
	int command(int argc, const char*const* argv);
	int pkt_type_;
	int drop_cycle_;
	int drop_offset_;
};


class Classifier;

class ErrorModule : public Connector {
public:
	ErrorModule() : classifier_(0) {}
protected:
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
	Classifier* classifier_;
};

/* error model that reads a loss trace (instead of a math/computed model) */
class TraceErrorModel : public ErrorModel {
public:
	TraceErrorModel();
	virtual int match(Packet* p);
	virtual int corrupt(Packet* p);
	virtual void recv(Packet* pkt, Handler* h);
protected:
	double loss_;
	double good_;
};

/* error model for multicast routing,... now inherits from trace.. later
may make them separate and use pointer/containment.. etc */
class MrouteErrorModel : public TraceErrorModel {
public:
	MrouteErrorModel();
	virtual int match(Packet* p);
	inline int maxtype() { return sizeof(msg_type); }
protected:
	int command(int argc, const char*const* argv);
	int off_mcast_ctrl_; /* don't forget to bind this to tcl */
	char msg_type[15]; /* to which to copy the message code (e.g.
			    *  "prune","join"). It's size is the same
			    * as type_ in mcast_ctrl.h [also returned by maxtype.]
			    */
};


#endif
