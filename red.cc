/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 *
 *
 * Here is one set of parameters from one of Sally's simulations
 * (this is from tcpsim, the older simulator):
 * 
 * ed [ q_weight=0.002 thresh=5 linterm=30 maxthresh=15
 *         mean_pktsize=500 dropmech=random-drop queue-size=60
 *         plot-file=none bytes=false doubleq=false dqthresh=50 
 *	   wait=true ]
 * 
 * 1/"linterm" is the max probability of dropping a packet. 
 * There are different options that make the code
 * more messy that it would otherwise be.  For example,
 * "doubleq" and "dqthresh" are for a queue that gives priority to
 *   small (control) packets, 
 * "bytes" indicates whether the queue should be measured in bytes 
 *   or in packets, 
 * "dropmech" indicates whether the drop function should be random-drop 
 *   or drop-tail when/if the queue overflows, and 
 *   the commented-out Holt-Winters method for computing the average queue 
 *   size can be ignored.
 * "wait" indicates whether the gateway should wait between dropping
 *   packets.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/red.cc,v 1.57 2001/06/15 00:18:13 sfloyd Exp $ (LBL)";
#endif

#include <math.h>
#include <sys/types.h>
#include "config.h"
#include "template.h"
#include "random.h"
#include "flags.h"
#include "delay.h"
#include "red.h"

static class REDClass : public TclClass {
public:
	REDClass() : TclClass("Queue/RED") {}
	TclObject* create(int argc, const char*const* argv) {
		//printf("creating RED Queue. argc = %d\n", argc);
		
		//mod to enable RED to take arguments
		if (argc==5) 
			return (new REDQueue(argv[4]));
		else
			return (new REDQueue("Drop"));
	}
} class_red;

/* Strangely this didn't work. 
 * Seg faulted for child classes.
REDQueue::REDQueue() { 
	REDQueue("Drop");
}
*/

/*
 * modified to enable instantiation with special Trace objects - ratul
 */
REDQueue::REDQueue(const char * trace) : link_(NULL), bcount_(0), de_drop_(NULL), EDTrace(NULL),
	tchan_(0), idle_(1), first_reset_(1)
{
	//	printf("Making trace type %s\n", trace);
	if (strlen(trace) >=20) {
		printf("trace type too long - allocate more space to traceType in red.h and recompile\n");
		exit(0);
	}
	strcpy(traceType, trace);
	bind_bool("bytes_", &edp_.bytes);	    // boolean: use bytes?
	bind_bool("queue_in_bytes_", &qib_);	    // boolean: q in bytes?
	//	_RENAMED("queue-in-bytes_", "queue_in_bytes_");

	bind("thresh_", &edp_.th_min);		    // minthresh
	bind("maxthresh_", &edp_.th_max);	    // maxthresh
	bind("mean_pktsize_", &edp_.mean_pktsize);  // avg pkt size
	bind("q_weight_", &edp_.q_w);		    // for EWMA
	bind("adaptive_", &edp_.adaptive);          // 1 for adaptive red
	bind("alpha_", &edp_.alpha); 	  	    // adaptive red param
	bind("beta_", &edp_.beta);                  // adaptive red param
	bind("interval_", &edp_.interval);	    // adaptive red param
	bind_bool("wait_", &edp_.wait);
	bind("linterm_", &edp_.max_p_inv);
	bind_bool("setbit_", &edp_.setbit);	    // mark instead of drop
	bind_bool("gentle_", &edp_.gentle);         // increase the packet
						    // drop prob. slowly
						    // when ave queue
						    // exceeds maxthresh

	bind_bool("summarystats_", &edp_.summarystats);
	bind_bool("drop_tail_", &drop_tail_);	    // drop last pkt
	//	_RENAMED("drop-tail_", "drop_tail_");

	bind_bool("drop_front_", &drop_front_);	    // drop first pkt
	//	_RENAMED("drop-front_", "drop_front_");
	
	bind_bool("drop_rand_", &drop_rand_);	    // drop pkt at random
	//	_RENAMED("drop-rand_", "drop_rand_");

	bind_bool("ns1_compat_", &ns1_compat_);	    // ns-1 compatibility
	//	_RENAMED("ns1-compat_", "ns1_compat_");

	bind("ave_", &edv_.v_ave);		    // average queue sie
	bind("prob1_", &edv_.v_prob1);		    // dropping probability
	bind("curq_", &curq_);			    // current queue size
	
	q_ = new PacketQueue();			    // underlying queue
	pq_ = q_;
	reset();
#ifdef notdef
	print_edp();
	print_edv();
#endif
	
}

void REDQueue::reset()
{
	/*
	 * If queue is measured in bytes, scale min/max thresh
	 * by the size of an average packet (which is specified by user).
	 */
	
	if (edp_.th_max == 0) 
		edp_.th_max = 3 * edp_.th_min;
        if (qib_ && first_reset_ == 1) {
                //printf ("edp_.th_min: %5.3f \n", edp_.th_min);
                edp_.th_min *= edp_.mean_pktsize;  
                edp_.th_max *= edp_.mean_pktsize;
                //printf ("edp_.th_min: %5.3f \n", edp_.th_min);
                first_reset_ = 0;
        }

	/*
	 * Compute the "packet time constant" if we know the
	 * link bandwidth.  The ptc is the max number of (avg sized)
	 * pkts per second which can be placed on the link.
	 * The link bw is given in bits/sec, so scale mean psize
	 * accordingly.
	 */
	 
	if (link_)
		edp_.ptc = link_->bandwidth() /
			(8. * edp_.mean_pktsize);

	edv_.v_ave = 0.0;
	edv_.v_true_ave = 0.0;
	edv_.v_total_time = 0.0;
	edv_.v_slope = 0.0;
	edv_.count = 0;
	edv_.count_bytes = 0;
	edv_.old = 0;
	edv_.v_a = 1 / (edp_.th_max - edp_.th_min);
	edv_.cur_max_p = 1.0 / edp_.max_p_inv;
	edv_.v_b = - edp_.th_min / (edp_.th_max - edp_.th_min);
	edv_.lastset = 0.0;
	if (edp_.gentle) {
		edv_.v_c = ( 1.0 - edv_.cur_max_p ) / edp_.th_max;
		edv_.v_d = 2 * edv_.cur_max_p - 1.0;
	}

	idle_ = 1;
	if (&Scheduler::instance() != NULL)
		idletime_ = Scheduler::instance().clock();
	else
		idletime_ = 0.0; /* sched not instantiated yet */
	
	if (debug_) 
		printf("Doing a queue reset\n");
	Queue::reset();
	if (debug_) 
		printf("Done queue reset\n");

	bcount_ = 0;
}

/*
 * Compute the average queue size.
 * Nqueued can be bytes or packets.
 */
double REDQueue::estimator(int nqueued, int m, double ave, double q_w)
{
	double new_ave, old_ave;

	new_ave = ave;
	while (--m >= 1) {
		old_ave = new_ave;
		new_ave *= 1.0 - q_w;
	}
	old_ave = new_ave;
	new_ave *= 1.0 - q_w;
	new_ave += q_w * nqueued;
	
	double now = Scheduler::instance().clock();
	if (edp_.adaptive == 1 && now > edv_.lastset + edp_.interval ) {
		double part = 0.4*(edp_.th_max - edp_.th_min);
		// AIMD rule to keep target Q~1/2(th_min+th_max)
		int change = 0;
		if ( new_ave < edp_.th_min + part && edv_.cur_max_p > 0.01 ) {
			// we increase the average queue size, so decrease max_p
			edv_.cur_max_p = edv_.cur_max_p * edp_.beta;
			edv_.lastset = now;
		} else if (new_ave > edp_.th_max - part && 0.5 > edv_.cur_max_p ) {
			// we decrease the average queue size, so increase max_p
			edv_.cur_max_p = edv_.cur_max_p + edp_.alpha;
			edv_.lastset = now;
		} 
	}
	return new_ave;
}

/*
 * Return the next packet in the queue for transmission.
 */
Packet* REDQueue::deque()
{
	Packet *p;
	p = q_->deque();
	if (p != 0) {
		idle_ = 0;
		bcount_ -= hdr_cmn::access(p)->size();
	} else {
		idle_ = 1;
		// deque() may invoked by Queue::reset at init
		// time (before the scheduler is instantiated).
		// deal with this case
		if (&Scheduler::instance() != NULL)
			idletime_ = Scheduler::instance().clock();
		else
			idletime_ = 0.0;
	}
	return (p);
}

/*
 * Calculate the drop probability.
 */
double
REDQueue::calculate_p_new(double v_ave, double th_max, int gentle, double v_a, 
	double v_b, double v_c, double v_d, double max_p)
{
	double p;
	if (gentle && v_ave >= th_max) {
		// p ranges from max_p to 1 as the average queue
		// size ranges from th_max to twice th_max 
		p = v_c * v_ave + v_d;
	} else {
		// p ranges from 0 to max_p as the average queue
		// size ranges from th_min to th_max 
		p = v_a * v_ave + v_b;
		p *= max_p;
	}
	if (p > 1.0)
		p = 1.0;
	return p;
}

/*
 * Calculate the drop probability.
 * This is being kept for backwards compatibility.
 */
double
REDQueue::calculate_p(double v_ave, double th_max, int gentle, double v_a, 
	double v_b, double v_c, double v_d, double max_p_inv)
{
	double p = calculate_p_new(v_ave, th_max, gentle, v_a,
		v_b, v_c, v_d, 1.0 / max_p_inv);
	return p;
}

/*
 * Make uniform instead of geometric interdrop periods.
 */
double
REDQueue::modify_p(double p, int count, int count_bytes, int bytes, 
   int mean_pktsize, int wait, int size)
{
	double count1 = (double) count;
	if (bytes)
		count1 = (double) (count_bytes/mean_pktsize);
	if (wait) {
		if (count1 * p < 1.0)
			p = 0.0;
		else if (count1 * p < 2.0)
			p /= (2 - count1 * p);
		else
			p = 1.0;
	} else {
		if (count1 * p < 1.0)
			p /= (1.0 - count1 * p);
		else
			p = 1.0;
	}
	if (bytes && p < 1.0) {
		p = p * size / mean_pktsize;
	}
	if (p > 1.0)
		p = 1.0;
 	return p;
}

/*
 * 
 */

/*
 * should the packet be dropped/marked due to a probabilistic drop?
 */
int
REDQueue::drop_early(Packet* pkt)
{
	hdr_cmn* ch = hdr_cmn::access(pkt);

	edv_.v_prob1 = calculate_p_new(edv_.v_ave, edp_.th_max, edp_.gentle, 
  	  edv_.v_a, edv_.v_b, edv_.v_c, edv_.v_d, edv_.cur_max_p);
	edv_.v_prob = modify_p(edv_.v_prob1, edv_.count, edv_.count_bytes,
	  edp_.bytes, edp_.mean_pktsize, edp_.wait, ch->size());

	// drop probability is computed, pick random number and act
	double u = Random::uniform();
	if (u <= edv_.v_prob) {
		// DROP or MARK
		edv_.count = 0;
		edv_.count_bytes = 0;
		hdr_flags* hf = hdr_flags::access(pickPacketForECN(pkt));
		if (edp_.setbit && hf->ect() && edv_.v_ave < edp_.th_max) {
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
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the packet for 
 * ECN notification.
 */
Packet*
REDQueue::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

/*
 * Pick packet to drop. Having a separate function do this is convenient for
 * supporting derived classes that use the standard RED algorithm to compute
 * average queue size but use a different algorithm for choosing the victim.
 */
Packet*
REDQueue::pickPacketToDrop() 
{
	int victim;

	if (drop_front_)
		victim = min(1, q_->length()-1);
	else if (drop_rand_)
		victim = Random::integer(q_->length());
	else			/* default is drop_tail_ */
		victim = q_->length() - 1;

	return(q_->lookup(victim)); 
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
 * "Unforced" means a RED random drop.
 *
 * For forced drops, either the arriving packet is dropped or one in the
 * queue is dropped, depending on the setting of drop_tail_.
 * For unforced drops, the arriving packet is always the victim.
 */

#define	DTYPE_NONE	0	/* ok, no drop */
#define	DTYPE_FORCED	1	/* a "forced" drop */
#define	DTYPE_UNFORCED	2	/* an "unforced" (random) drop */

void REDQueue::enque(Packet* pkt)
{

	/*
	 * if we were idle, we pretend that m packets arrived during
	 * the idle period.  m is set to be the ptc times the amount
	 * of time we've been idle for
	 */

	double now = Scheduler::instance().clock();
	int m = 0;
	if (idle_) {
		// A packet that arrives to an idle queue will never
		//  be dropped.
		double now = Scheduler::instance().clock();
		/* To account for the period when the queue was empty. */
		idle_ = 0;
		m = int(edp_.ptc * (now - idletime_));
	}

	/*
	 * Run the estimator with either 1 new packet arrival, or with
	 * the scaled version above [scaled by m due to idle time]
	 * (bcount_ maintains the byte count in the underlying queue).
	 * If the underlying queue is able to delete packets without
	 * us knowing, then bcount_ will not be maintained properly!
	 */
	edv_.v_ave = estimator(qib_ ? bcount_ : q_->length(), m + 1, edv_.v_ave, edp_.q_w);
	//printf("v_ave: %6.4f (%13.12f) q: %d)\n", 
	//	double(edv_.v_ave), double(edv_.v_ave), q_->length());
	//run_estimator(qib_ ? bcount_ : q_->length(), m + 1);
	if (edp_.summarystats) {
		double oldave = edv_.v_true_ave;
		double oldtime = edv_.v_total_time;
		double newtime = now - edv_.v_total_time;
		int newsize;
		if (qib_) newsize = bcount_; else newsize = q_->length();
		edv_.v_true_ave = (oldtime * oldave + newtime * newsize)/now;
		edv_.v_total_time = now;
	}
	/* compute true average queue size for summary stats */


	/*
	 * count and count_bytes keeps a tally of arriving traffic
	 * that has not been dropped (i.e. how long, in terms of traffic,
	 * it has been since the last early drop)
	 */

	hdr_cmn* ch = hdr_cmn::access(pkt);
	++edv_.count;
	edv_.count_bytes += ch->size();

	/*
	 * DROP LOGIC:
	 *	q = current q size, ~q = averaged q size
	 *	1> if ~q > maxthresh, this is a FORCED drop
	 *	2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
	 *	3> if (q+1) > hard q limit, this is a FORCED drop
	 */

	register double qavg = edv_.v_ave;
	int droptype = DTYPE_NONE;
	int qlen = qib_ ? bcount_ : q_->length();
	int qlim = qib_ ? (qlim_ * edp_.mean_pktsize) : qlim_;

	curq_ = qlen;	// helps to trace queue during arrival, if enabled

	if (qavg >= edp_.th_min && qlen > 1) {
		if ((!edp_.gentle && qavg >= edp_.th_max) ||
			(edp_.gentle && qavg >= 2 * edp_.th_max)) {
			droptype = DTYPE_FORCED;
		} else if (edv_.old == 0) {
			/* 
			 * The average queue size has just crossed the
			 * threshold from below to above "minthresh", or
			 * from above "minthresh" with an empty queue to
			 * above "minthresh" with a nonempty queue.
			 */
			edv_.count = 1;
			edv_.count_bytes = ch->size();
			edv_.old = 1;
		} else if (drop_early(pkt)) {
			droptype = DTYPE_UNFORCED;
		}
	} else {
		/* No packets are being dropped.  */
		edv_.v_prob = 0.0;
		edv_.old = 0;		
	}
	if (qlen >= qlim) {
		// see if we've exceeded the queue size
		droptype = DTYPE_FORCED;
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

			reportDrop(pkt);
			de_drop_->recv(pkt);
		}
		else {
			reportDrop(pkt);
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
			reportDrop(pkt);
			drop(pkt);
			if (!ns1_compat_) {
				// bug-fix from Philip Liu, <phill@ece.ubc.ca>
				edv_.count = 0;
				edv_.count_bytes = 0;
			}
		}
	}
	return;
}

int REDQueue::command(int argc, const char*const* argv)
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
				if (debug_) 
					printf("edrop trace exists according to RED\n");
			}
			else {
				if (debug_)
					printf("edrop trace doesn't exist according to RED\n");
				tcl.resultf("0");
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "trace-type") == 0) {
			tcl.resultf("%s", traceType);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "printstats") == 0) {
			print_summarystats();
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
				tcl.resultf("RED: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		// tell RED about link stats
		if (strcmp(argv[1], "link") == 0) {
			LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
			if (del == 0) {
				tcl.resultf("RED: no LinkDelay object %s",
					argv[2]);
				return(TCL_ERROR);
			}
			// set ptc now
			link_ = del;
			edp_.ptc = link_->bandwidth() /
				(8. * edp_.mean_pktsize);

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
			if (debug_) 
				printf("Ok, Here\n");
			NsObject * t  = (NsObject *)TclObject::lookup(argv[2]);
			if (debug_)  
				printf("Ok, Here too\n");
			if (t == 0) {
				tcl.resultf("no object %s", argv[2]);
				return (TCL_ERROR);
			}
			EDTrace = t;
			if (debug_)  
				printf("Ok, Here too too too %d\n", ((Trace *)EDTrace)->type_);
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
REDQueue::trace(TracedVar* v)
{
	char wrk[500], *p;

	if (((p = strstr(v->name(), "ave")) == NULL) &&
	    ((p = strstr(v->name(), "prob")) == NULL) &&
	    ((p = strstr(v->name(), "curq")) == NULL)) {
		fprintf(stderr, "RED:unknown trace var %s\n",
			v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
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

/* for debugging help */
void REDQueue::print_edp()
{
	printf("mean_pktsz: %d\n", edp_.mean_pktsize); 
	printf("bytes: %d, wait: %d, setbit: %d\n",
		edp_.bytes, edp_.wait, edp_.setbit);
	printf("minth: %f, maxth: %f\n", edp_.th_min, edp_.th_max);
	printf("max_p: %f, qw: %f, ptc: %f\n",
		edv_.cur_max_p, edp_.q_w, edp_.ptc);
	printf("qlim: %d, idletime: %f\n", qlim_, idletime_);
	printf("=========\n");
}

void REDQueue::print_edv()
{
	printf("v_a: %f, v_b: %f\n", edv_.v_a, edv_.v_b);
}

void REDQueue::print_summarystats()
{
	double now = Scheduler::instance().clock();
	printf("True average queue: %5.3f time: %5.3f\n", edv_.v_true_ave, edv_.v_total_time);
}

/************************************************************/
/*
 * This procedure is obsolete, and only included for backward compatibility.
 * The new procedure is REDQueue::estimator
 */ 
/*
 * Compute the average queue size.
 * The code contains two alternate methods for this, the plain EWMA
 * and the Holt-Winters method.
 * nqueued can be bytes or packets
 */
void REDQueue::run_estimator(int nqueued, int m)
{
	double f, f_sl, f_old;

	f = edv_.v_ave;
	f_sl = edv_.v_slope;
#define RED_EWMA
#ifdef RED_EWMA
	while (--m >= 1) {
		f_old = f;
		f *= 1.0 - edp_.q_w;
	}
	f_old = f;
	f *= 1.0 - edp_.q_w;
	f += edp_.q_w * nqueued;
#endif
#ifdef RED_HOLT_WINTERS
	while (--m >= 1) {
		f_old = f;
		f += f_sl;
		f *= 1.0 - edp_.q_w;
		f_sl *= 1.0 - 0.5 * edp_.q_w;
		f_sl += 0.5 * edp_.q_w * (f - f_old);
	}
	f_old = f;
	f += f_sl;
	f *= 1.0 - edp_.q_w;
	f += edp_.q_w * nqueued;
	f_sl *= 1.0 - 0.5 * edp_.q_w;
	f_sl += 0.5 * edp_.q_w * (f - f_old);
#endif
	edv_.v_ave = f;
	edv_.v_slope = f_sl;
}

