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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/pi.cc,v 1.2 2001/06/27 18:31:53 sfloyd Exp $ (LBL)";
#endif

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "pi.h"

static class PIClass : public TclClass {
public:
	PIClass() : TclClass("Queue/PI") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc==5) 
			return (new PIQueue(argv[4]));
		else
			return (new PIQueue("Drop"));
	}
} class_pi;


PIQueue::PIQueue(const char * trace) : link_(NULL), bcount_(0), 
  de_drop_(NULL), EDTrace(NULL), tchan_(0), first_reset_(1), CalcTimer(this)
{
	if (strlen(trace) >=20) {
		printf("trace type too long - allocate more space to traceType in red.h and recompile\n");
		exit(0);
	}
	strcpy(traceType, trace);

	bind_bool("bytes_", &edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &qib_);	    // boolean: q in bytes?
	bind("a_", &edp_.a);		
	bind("b_", &edp_.b);	   
	bind("w_", &edp_.w);		  
	bind("qref_", &edp_.qref);		  
	bind("mean_pktsize_", &edp_.mean_pktsize);  // avg pkt size
	bind_bool("setbit_", &edp_.setbit);	    // mark instead of drop
	bind("prob_", &edv_.v_prob);		    // dropping probability
	bind("curq_", &curq_);			    // current queue size
	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	reset();
}

void PIQueue::reset()
{
	double now = Scheduler::instance().clock();
	if (qib_ && first_reset_ == 1) {
		edp_.qref = edp_.qref*edp_.mean_pktsize;
	}
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.v_prob = 0;
	edv_.qold = 0;
	curq_ = 0;
	bcount_ = 0;
	calculate_p();
	Queue::reset();
}

/*
 * Receive a new packet arriving at the queue.
 * The average queue size is computed.  If the average size
 * exceeds the threshold, then the dropping probability is computed,
 * and the newly-arriving packet is dropped with that probability.
 * The packet is also dropped if the maximum queue size is exceeded.
 *
 * "Forced" drops mean a packet arrived when the underlying queue was
 * full or when the average q size exceeded maxthresh.
 * "Unforced" means a PI random drop.
 *
 * For forced drops, either the arriving packet is dropped or one in the
 * queue is dropped, depending on the setting of drop_tail_.
 * For unforced drops, the arriving packet is always the victim.
 */

void PIQueue::enque(Packet* pkt)
{
	/*
	 * count and count_bytes keeps a tally of arriving traffic
	 * that has not been dropped (i.e. how long, in terms of traffic,
	 * it has been since the last early drop)
	 */

	double now = Scheduler::instance().clock();
	hdr_cmn* ch = hdr_cmn::access(pkt);
	++edv_.count;
	edv_.count_bytes += ch->size();

	int droptype = DTYPE_NONE;

	int qlen = qib_ ? bcount_ : q_->length();
	curq_ = qlen;	// helps to trace queue during arrival, if enabled

	int qlim = qib_ ? (qlim_ * edp_.mean_pktsize) : qlim_;

	if (qlen >= qlim) {
		// see if we've exceeded the queue size
		droptype = DTYPE_FORCED;
		//printf ("# now=%f qlen=%d qref=%d FORCED drop\n", now, (int)curq_, (int)edp_.qref);
		//fflush(stdout);
	}
	else {
		if (drop_early(pkt, qlen)) {
			droptype = DTYPE_UNFORCED;
			//printf ("# now=%f qlen=%d qref=%d EARLY drop\n", now, (int)curq_, (int)edp_.qref);
			//fflush(stdout);
		}
	}

	if (droptype == DTYPE_UNFORCED) {
		/* pick packet for ECN, which is dropping in this case */
		Packet *pkt_to_drop = pickPacketForECN(pkt);
		/* 
		 * If the packet picked is different that the one that just arrived,
		 * add it to the queue and remove the chosen packet.
		 */
		if (pkt_to_drop != pkt) {
			q_->enque(pkt);
			bcount_ += ch->size();
			q_->remove(pkt_to_drop);
			bcount_ -= hdr_cmn::access(pkt_to_drop)->size();
			pkt = pkt_to_drop; /* XXX okay because pkt is not needed anymore */
		}

		// deliver to special "edrop" target, if defined
		if (de_drop_ != NULL) {
	
		//trace first if asked 
		// if no snoop object (de_drop_) is defined, 
		// this packet will not be traced as a special case.
			if (EDTrace != NULL) 
				((Trace *)EDTrace)->recvOnly(pkt);

			de_drop_->recv(pkt);
		}
		else {
			drop(pkt);
		}
	} else {
		/* forced drop, or not a drop: first enqueue pkt */
		q_->enque(pkt);
		bcount_ += ch->size();

		/* drop a packet if we were told to */
		if (droptype == DTYPE_FORCED) {
			/* drop random victim or last one */
			pkt = pickPacketToDrop();
			q_->remove(pkt);

			bcount_ -= hdr_cmn::access(pkt)->size();

			drop(pkt);
			edv_.count = 0;
			edv_.count_bytes = 0;
		}
	}
	return;
}

double PIQueue::calculate_p()
{
	double now = Scheduler::instance().clock();
	double p;
	int qlen = qib_ ? bcount_ : q_->length();

	p=edp_.a*(qlen-edp_.qref)-edp_.b*(edv_.qold-edp_.qref)+edv_.v_prob;

	if (p < 0) p = 0;
	if (p > 1) p = 1;

	/*
	printf ("# now=%f qlen=%d qref=%d a=%f b=%f oldp=%f newp=%f w=%f\n", 
  					now, (int)qlen, (int)edp_.qref, (double)edp_.a, (double)edp_.b, 
  						(double)edv_.v_prob, p, (double)edp_.w);
	fflush(stdout);
	*/

	edv_.v_prob = p;
	edv_.qold = qlen;

	CalcTimer.resched(1.0/edp_.w);
	return p;
}

int PIQueue::drop_early(Packet* pkt, int qlen)
{
	double now = Scheduler::instance().clock();
	hdr_cmn* ch = hdr_cmn::access(pkt);
	double p = edv_.v_prob; 

	if (edp_.bytes) {
		p = p*ch->size()/edp_.mean_pktsize;
		if (p > 1) p = 1; 
	}

	// drop probability is computed, pick random number and act
	double u = Random::uniform();
	//printf ("# now=%f qref=%d p=%f u=%f\n", now, (int)edp_.qref, p, u);
	//fflush(stdout);
	if (u <= p) {
		// DROP or MARK
		edv_.count = 0;
		edv_.count_bytes = 0;
		hdr_flags* hf = hdr_flags::access(pickPacketForECN(pkt));
		if (edp_.setbit && hf->ect()) {
			hf->ce() = 1; 	// mark Congestion Experienced bit
			return (0);	// no drop
		} else {
			return (1);	// drop
		}
	}
	return (0);			// no DROP/mark
}

/*
 * Pick packet for early congestion notification (ECN). This packet is then
 * marked or dropped. Having a separate function do this is convenient for
 * supporting derived classes that use the standard PI algorithm to compute
 * average queue size but use a different algorithm for choosing the packet for 
 * ECN notification.
 */
Packet*
PIQueue::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

/*
 * Pick packet to drop. Same as above. 
 */
Packet* PIQueue::pickPacketToDrop() 
{
	int victim;
	victim = q_->length() - 1;
	return(q_->lookup(victim)); 
}

Packet* PIQueue::deque()
{
	Packet *p;
	p = q_->deque();
	if (p != 0) {
		bcount_ -= hdr_cmn::access(p)->size();
	} 
	curq_ = qib_ ? bcount_ : q_->length(); // helps to trace queue during arrival, if enabled
	return (p);
}

int PIQueue::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			if (de_drop_ != NULL)
				tcl.resultf("%s", de_drop_->name());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "edrop-trace") == 0) {
			if (EDTrace != NULL) {
				tcl.resultf("%s", EDTrace->name());
			}
			else {
				tcl.resultf("0");
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "trace-type") == 0) {
			tcl.resultf("%s", traceType);
			return (TCL_OK);
		}
	} 
	else if (argc == 3) {
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("PI: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		// tell PI about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("PI: no LinkDelay object %s", argv[2]);
				return(TCL_ERROR);
			}
			link_ = del;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "early-drop-target") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			de_drop_ = p;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "edrop-trace") == 0) {
			NsObject * t  = (NsObject *)TclObject::lookup(argv[2]);
			if (t == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			EDTrace = t;
			return (TCL_OK);
		}
		if (!strcmp(argv[1], "packetqueue-attach")) {
			delete q_;
			if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
				return (TCL_ERROR);
			else {
				pq_ = q_;
				return (TCL_OK);
			}
		}
	}
	return (Queue::command(argc, argv));
}

/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace values of avg queue size, drop probability,
 * and the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void
PIQueue::trace(TracedVar* v)
{
	char wrk[500], *p;

	if (((p = strstr(v->name(), "ave")) == NULL) &&
	    ((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL)) {
		fprintf(stderr, "PI:unknown trace var %s\n",
			v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 PI trace entries
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t,
				double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}

void PICalcTimer::expire(Event *)
{
	a_->calculate_p();
}


