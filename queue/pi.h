/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2001  International Computer Science Institute
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
 *      This product includes software developed by ACIRI, the AT&T
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Based on PI conrtoller proposed by Hollot et. al.
 * C. Hollot, V. Misra, D. Towsley and W. Gong. On Designing Improved Controllers for AQM Routers
 * Supporting TCP Flows , INFOCOMM 2001. 
 */

#ifndef ns_pi_h
#define ns_pi_h

#include "queue.h"
#include "trace.h"
#include "timer-handler.h"

#define	DTYPE_NONE	0	/* ok, no drop */
#define	DTYPE_FORCED	1	/* a "forced" drop */
#define	DTYPE_UNFORCED	2	/* an "unforced" (random) drop */


/*
 * Early drop parameters, supplied by user
 */
struct edp {
	/*
	 * User supplied.
	 */
	int mean_pktsize;	/* avg pkt size, linked into Tcl */
	int bytes;		/* true if queue in bytes, false if packets */
	int setbit;		/* true to set congestion indication bit */
	double a, b;		 /* parameters to pi controller */
	double w;				/* sampling frequency (# of times per second) */ 
	double qref;		/* desired queue size */
};

/*
 * Early drop variables, maintained by PI
 */
struct edv {
	TracedDouble v_prob;	/* prob. of packet drop before "count". */
	int count;		/* # of packets since last drop */
	int count_bytes;	/* # of bytes since last drop */
	int qold;
	edv() : v_prob(0.0), count(0), count_bytes(0) { }
};

class LinkDelay;
class PIQueue ; 

class PICalcTimer : public TimerHandler {
public:
		PICalcTimer(PIQueue *a) : TimerHandler() { a_ = a; }
		virtual void expire(Event *e);
protected:
		PIQueue *a_;
};


class PIQueue : public Queue {
 
 friend PICalcTimer;
 public:	
	/*	PIQueue();*/
	PIQueue(const char * = "Drop");
 protected:
	int command(int argc, const char*const* argv);
	void enque(Packet* pkt);
	virtual Packet *pickPacketForECN(Packet* pkt);
	virtual Packet *pickPacketToDrop();
	Packet* deque();
	void reset();
	int drop_early(Packet* pkt, int qlen);
 	double calculate_p();
	PICalcTimer CalcTimer;

	LinkDelay* link_;	/* outgoing link */
	int fifo_;		/* fifo queue? */
  PacketQueue *q_; 	/* underlying (usually) FIFO queue */
		
	int bcount_;	/* byte count */
	int qib_;	/* bool: queue measured in bytes? */
	NsObject* de_drop_;	/* drop_early target */

	//added to be able to trace EDrop Objects - ratul
	//the other events - forced drop, enque and deque are traced by a different mechanism.
	NsObject * EDTrace;    //early drop trace
	char traceType[20];    //the preferred type for early drop trace. 
	                       //better be less than 19 chars long
	Tcl_Channel tchan_;	/* place to write trace records */
	TracedInt curq_;	/* current qlen seen by arrivals */
	void trace(TracedVar*);	/* routine to write trace records */

	edp edp_;	/* early-drop params */
	edv edv_;		/* early-drop variables */

	int first_reset_;       /* first time reset() is called */

};

#endif