/*
 * Copyright (c) 1991-1997 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/tcp-sink.h,v 1.3 1997/07/23 03:13:36 kfall Exp $ (LBL)
 */

#ifndef ns_tcpsink_h
#define ns_tcpsink_h

#include <math.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "agent.h"
#include "flags.h"

/* max window size */
#define MWS 1024
#define MWM (MWS-1)
/* For Tahoe TCP, the "window" parameter, representing the receiver's
 * advertised window, should be less than MWM.  For Reno TCP, the
 * "window" parameter should be less than MWM/2.
 */

class Acker {
public:
	Acker();
	virtual ~Acker() {}
	void update(int seqno);
	inline int Seqno() const { return (next_ - 1); }
	virtual void append_ack(hdr_cmn*, hdr_tcp*, int oldSeqno) const;
	void reset();
protected:
	int next_;		/* next packet expected  */
	int maxseen_;		/* max packet number seen */
	int seen_[MWS];		/* array of packets seen  */
};

class TcpSink : public Agent {
public:
	TcpSink(Acker*);
	void recv(Packet* pkt, Handler*);
	void reset();
protected:
	void ack(Packet*);
	virtual void add_to_ack(Packet* pkt);  
	Acker* acker_;
	int off_tcp_;
};

#define DELAY_TIMER 0

class DelAckSink : public TcpSink {
public:
	DelAckSink(Acker* acker);
	void recv(Packet* pkt, Handler*);
protected:
	virtual void timeout(int tno);
	Packet* save_;		/* place to stash packet while delaying */
	double interval_;
};

#endif
