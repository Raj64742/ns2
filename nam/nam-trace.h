/*
 * Copyright (c) 1991,1993 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/nam/Attic/nam-trace.h,v 1.1 1997/03/29 04:38:04 mccanne Exp $ (LBL)
 */

#ifndef nam_trace_h
#define nam_trace_h

#include "Tcl.h"

/*
 * 'packet' events (hop, enqueue, dequeue & drop) all have the same
 * format:  the src & dst node and a description of the packet.
 */
struct PacketAttr {
	int size;
	int attr;
        int id;
	char type[8];
	char convid[32];
};

struct PacketEvent {
	int src;
	int dst;
	PacketAttr pkt;
};

struct VarEvent {
	/* Var event stuff */
	int str;
};

struct TraceEvent {
	double time;		/* event time */
	long offset;		/* XXX trace file offset */
	int lineno;		/* XXX trace file line no. */
	int tt;			/* type: h,+,-,d */
	union {
		PacketEvent pe;
		VarEvent ve;
	};
	char image[256];
};

class TraceHandler : public TclObject {
 public:
	virtual void update(double) = 0;
	virtual void reset(double) = 0;
	virtual void handle(const TraceEvent&, double) = 0;
};

struct TraceHandlerList {
	TraceHandler* th;
	TraceHandlerList* next;
};

class NamTrace : public TclObject {
 public:
	NamTrace(const char *);
	int command(int argc, const char*const* argv);
	int ReadEvent(double, TraceEvent&);
	void scan();
	void rewind(long);
	double nextTime() { return (pending_.time); }
	double Maxtime() { return (maxtime_); }
	double Mintime() { return (mintime_); }
	int valid() { return (file_ != 0); }

	/*NamTrace* next_;*/
 private:
	void addHandler(TraceHandler*);
	void settime(double now, int timeSliderClicked);
	void findLastLine(int);

	TraceHandlerList* handlers_;
	int lineno_;
	double maxtime_;
	double mintime_;
	double now_;
	FILE *file_;
	TraceEvent pending_;
	char fileName_[256];
};

#endif

