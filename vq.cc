/* -*-  Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *  This product includes software developed by the Computer Systems
 *  Engineering Group at Lawrence Berkeley Laboratory.
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

/* Marking scheme proposed by Kunniyur and Srikant in "End-to-End 
   Congestion Control Schemes: Utility Functions, Random Losses and
   ECN Marks" published in the Proceedings of Infocom2000, Tel-Aviv, 
   Israel, March 2000.
 * Central Idea:
 * ------------

 * The scheme involves having a virtual queue at each link with the
 * same arrivals as the real queue but with the service rate scaled 
 * down by some factor (ecnlim_). If needed, we can also scale down the
 * buffer capacity by some constant (buflim_). When a packet is
 * dropped from the virtual queue, mark a packet at the front of the
 * real queue. The capacity of the virtual queue is adapted
 * periodically to guarantee loss-free service.

 * Implementation Details:
 * -----------------------

 * The update of the virtual capacity (capacity of the virtual queue)
 * is done using a token-bucket. For implentation details, please refer 
 * to the Sigcomm 2001 paper titled "Analysis and Design of an Adaptive
 * Virtual Queue (AVQ) algorithm for Active Queue Management (AQM). 

 * The virtual queue length can be taken in packets or in bytes.
 * If the packet sizes are not fixed, then it is recommended to use 
 * bytes instead of packets as units of length. If the variable (qib_)
 * is set, then the virtual queue is measured in bytes, else it is
 * measured in packets.  
 *	 - Srisankar 
 *     01/07/2001.

 * Intrdouced mean_pktsize_ and related changes to 
 * ensure corerct behavior when qib_ is set. also removed drop-front and mark-front. 
 * replaced with more generic functions similar to red.
 *   - Jitu
 *     06/19/2001
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /usr/src/mash/repository/vint/ns-2/drop-tail.cc,v 1.9 1998/06/27
 01:23:44 gnguyen Exp $ (LBL)";
#endif
#include "flags.h"
#include "delay.h"
#include "vq.h"
#include "math.h"


static class VqClass : public TclClass {
public:
	VqClass() : TclClass("Queue/Vq") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc==5) 
			return (new Vq(argv[4]));
		else
			return (new Vq("Drop"));
	}
} class_vq;

Vq::Vq(const char * trace) : link_(NULL), bcount_(0), EDTrace(NULL), tchan_(0){
	q_ = new PacketQueue;
	pq_ = q_;
	bind("ecnlim_", &ecnlim_); //  = ctilde/c ; Initial value set to 0.8
	bind("buflim_", &buflim_); /* Fraction of the original buffer */
			/* that the VQ buffer has. Default is 1.0 */
	bind_bool("queue_in_bytes_", &qib_);	 // boolean: q in bytes?
	bind("gamma_", &gamma_); // Defines the utilization. Default is 0.98
	bind_bool("markpkts_", &markpkts_); /* Whether to mark or drop?  */
					/* Default is drop */
	bind("mean_pktsize_", &mean_pktsize_);	// avg pkt size
	bind("curq_", &curq_);	        // current queue size

	vq_len = 0.0;
	prev_time = 0.0;
	vqprev_time = 0.0;
	mark_count = 0;
	IsPktdrop = 0; // Flag
	alpha2 = 0.15; // Determines how fast we adapt at the link
	Pktdrp = 0; /* Useful if we are dropping pkts instead of marking */
	firstpkt = 1;
}

int Vq::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
  if (argc == 3) {
	  if (strcmp(argv[1], "link") == 0) {
		  LinkDelay* del = (LinkDelay*)TclObject::lookup(argv[2]);
		  if (del == 0) {
			  return(TCL_ERROR);
		  }
		  // set capacity now
			link_ = del;
		  c_ = del->bandwidth();
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
		// attach a file for variable tracing
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			tchan_ = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (tchan_ == 0) {
				tcl.resultf("Vq: trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
  }
  return Queue::command(argc, argv);
}

void Vq::enque(Packet* p)
{
	int qlim = qib_ ? (qlim_ * mean_pktsize_) : qlim_;
	double now = Scheduler::instance().clock();
	hdr_cmn* ch = hdr_cmn::access(p);

	if(firstpkt){
		/* Changing c_ so that it is measured in packets per second */
		if (qib_) c_ = c_ / (8.0);
		else c_ = c_ / (8.0 * mean_pktsize_);
		firstpkt = 0;
		ctilde = c_ * ecnlim_;
		prev_time = Scheduler::instance().clock();
		vqprev_time = Scheduler::instance().clock();
	}

	q_->enque(p); 
	bcount_ = bcount_ + ch->size();
	int qlen = qib_ ? bcount_ : q_->length();

	/*Whenever a packet arrives, the actual length of the virtual queue is determined */

	vq_len = vq_len - (ctilde * (now - vqprev_time));
	vqprev_time = now;
	if(vq_len < 0.0) vq_len = 0.0;
	vq_len = vq_len + qib_ * ch->size() + (1 - qib_);

	if (qlen >= qlim) {
		q_->remove(p);
		drop(p);
		bcount_ = bcount_ - ch->size();
		Pktdrp = 1;	
		vq_len = vq_len - qib_ * ch->size() - (1.0 - qib_);
		if(vq_len < 0.0) vq_len = 0.0;
	}
	else {
		if(vq_len > (buflim_ * qlim)){ // Mark or drop packets accordingly
			if(markpkts_ == 1){ // Mark packets  
				Packet *pp = pickPacketForECN(p);
				hdr_flags* hdr = hdr_flags::access(pp);
				// hdr_cmn* chpp = hdr_cmn::access(pp);
				hdr->ce() = 1; // For TCP Flows
			}
			else{ // If the router AQM policy is to drop packets
				Packet *pp = pickPacketToDrop();
				hdr_cmn* chpp = hdr_cmn::access(pp);
				q_->remove(pp);
				drop(pp);
				bcount_ = bcount_ - chpp->size ();
				Pktdrp = 1; // Do not update tilde(C)
			}
			/* Update the VQ length */
			vq_len = vq_len - qib_ * ch->size() - (1.0 - qib_);
			if(vq_len < 0.0) vq_len = 0.0;
		}
	}
	/* Adaptation of the virtual capacity, tilde(C) */
	if(Pktdrp == 0){ 
		ctilde = ctilde + alpha2*gamma_*c_*(now - prev_time) - alpha2*(1.0 - qib_) - alpha2*qib_*ch->size();
		if(ctilde > c_) ctilde = c_;
		if(ctilde <0.0) ctilde = 0.0;
		prev_time = now;
	}
	else{ // No adaptation and reset Pktdrp
		Pktdrp = 0;
	}
	qlen = qib_ ? bcount_ : q_->length();
	curq_ = qlen; 
/*
printf ("now=%.4f qlim=%d qlen=%d vq_len=%f buflim_*qlim=%.3f ctilde=%f gamma=%f\n\n", 
now, qlim, qlen, vq_len, (buflim_ * qlim), ctilde,gamma_);
*/
}

Packet* Vq::deque() 
{
	Packet *p;
	p = q_->deque();
	if (p != 0) {
		bcount_ -= hdr_cmn::access(p)->size();
	} 
	curq_ = qib_ ? bcount_ : q_->length();
	return (p);
}

/*
 * Pick packet for early congestion notification (ECN). This packet is then
 * marked or dropped. Having a separate function do this is convenient for
 * supporting derived classes that use the standard PI algorithm to compute
 * average queue size but use a different algorithm for choosing the packet for 
 * ECN notification.
 */
Packet* Vq::pickPacketForECN(Packet* pkt)
{
	return pkt; /* pick the packet that just arrived */
}

/*
 * Pick packet to drop. Same as above. 
 */
Packet* Vq::pickPacketToDrop() 
{
	int victim;
	victim = q_->length() - 1;
	return(q_->lookup(victim)); 
}


/*
 * Routine called by TracedVar facility when variables change values.
 * Currently used to trace value of 
 * the instantaneous queue size seen by arriving packets.
 * Note that the tracing of each var must be enabled in tcl to work.
 */

void Vq::trace(TracedVar* v)
{
	char wrk[500], *p;

	if ((p = strstr(v->name(), "curq")) == NULL) {
		fprintf(stderr, "Vq:unknown trace var %s\n", v->name());
		return;
	}

	if (tchan_) {
		int n;
		double t = Scheduler::instance().clock();
		// XXX: be compatible with nsv1 RED trace entries
		if (*p == 'c') {
			sprintf(wrk, "Q %g %d", t, int(*((TracedInt*) v)));
		} else {
			sprintf(wrk, "%c %g %g", *p, t, double(*((TracedDouble*) v)));
		}
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(tchan_, wrk, n+1);
	}
	return; 
}
