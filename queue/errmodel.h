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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/queue/errmodel.h,v 1.22 1998/03/11 04:38:02 gnguyen Exp $ (UCB)
 */

#ifndef ns_errmodel_h
#define ns_errmodel_h

#include "connector.h"
#include "ranvar.h"


enum ErrorUnit { EU_PKT=0, EU_BYTE, EU_TIME };
#define EU_NAMES "pkt", "byte", "time"
#define STR2EU(s) (!strcmp(s,"byte") ? EU_BYTE : (!strcmp(s,"time") ? EU_TIME : EU_PKT))

#define EM_GOOD 1
#define EM_BAD  2

/* 
 * Basic object for error models.  This can be used unchanged by error 
 * models that are characterized by a single parameter, the rate of errors 
 * (or equivalently, the mean duration/spacing between errors).  Currently,
 * this includes the uniform and exponentially-distributed models.
 */
class ErrorModel : public Connector {
  public:
	ErrorModel();
	virtual void recv(Packet *, Handler *);
	virtual void reset();
	virtual void copy(ErrorModel *);
	virtual int corrupt(Packet *);
	virtual int CorruptPkt(Packet *);
	virtual int CorruptTime(Packet *);
	virtual int CorruptByte(Packet *);
	inline double rate() { return rate_; }

  protected:
	virtual int command(int argc, const char*const* argv);
	ErrorUnit unit_;	// error unit in pkts, bytes, or time
	RandomVariable *ranvar_;// the underlying random variate generator
	double rate_;		/* mean pkts between errors (for EU_PKT), or
				 * mean bytes between errors (for EU_BYTE), or 
				 * mean time between errors (for EU_TIME). */
	double errorLen_;	// current length of error
	int enable_;		// true if this error module is turned on
	int firstTime_;		// to not corrupt first packet in byte model
	Event intr_;		// set callback to queue
};


class TwoStateErrorModel : public ErrorModel {
  public:
	TwoStateErrorModel() { unit_ = EU_TIME; };
	virtual int CorruptTime(Packet *);
	int command(int argc, const char*const* argv);
  protected:
	RandomVariable *ranvar_[2];
	int state_;
};

class MultiStateErrorModel : public ErrorModel {
public:
	virtual int corrupt(Packet *);
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


/* Error model for srm experiments */
class SRMErrorModel : public ErrorModel {
public:
	SRMErrorModel();
	virtual int corrupt(Packet*);

protected:
	int command(int argc, const char*const* argv);
        int pkt_type_;
        int drop_cycle_;
	int drop_offset_;
        int off_srm_;
};

#endif
