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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/red-pd.cc,v 1.1 2000/11/17 22:12:09 ratul Exp $ (LBL)";
#endif

// EXPERIMENTAL CODE - CAN BE UNSTABLE

#include "red-pd.h"


static class ReDPDClass : public TclClass {
public:
	ReDPDClass() : TclClass("Queue/RED/PD") {}
	TclObject* create(int argc, const char*const* argv) {
		printf("creating REDPD %d\n", argc);
		if (argc==4) {
			return (new RedPDQueue("Drop", "Drop"));
		}
		else  {
			char args[100];
			strcpy(args, argv[4]);
			//strtok used for compatibility reasons
			char * arg1 = strtok(args," ");
			char * arg2 = strtok(NULL," ");
			printf("got arguements :%s:  :%s:\n",arg1, arg2);
			if (arg2 == NULL) {
				printf("calling null arg2\n");
				return (new RedPDQueue(arg1, "Drop"));
			}
			else {
				return (new RedPDQueue(arg1, arg2));
			}
		}
		
	}
} red_pd_class;

static class RedPDFlowClass : public TclClass {
 public:
	RedPDFlowClass() : TclClass("QueueMonitor/ED/Flow/RedPD") {}
	TclObject* create(int, const char*const*) {
		return (new RedPDFlow);
	}
} red_pd_flow_class;

RedPDQueue::RedPDQueue(const char * medtype, const char * edtype): REDQueue(edtype),
	auto_(0), global_target_(0), targetBW_(0), noMonitored_(0), 
	unresponsive_penalty_(2), P_testFRp_(-1), flowMonitor_(NULL), MEDTrace(NULL) {

	//printf("In RedPD constructor with %s %s\n", medtype, edtype);
	if (strlen(medtype) >=20) {
		printf("RedPD : Too Long a trace type. Change the field length in red-pd.h and recompile\n");
		exit(0);
	}
	strcpy(medTraceType, medtype);
	
	off_ip_ = hdr_ip::offset();
	
	bind_bool("auto_", &auto_);
	bind_bool("global_target_", &global_target_);
	bind_bw("targetBW_", &targetBW_);
	bind("noMonitored_", &noMonitored_);
	bind("unresponsive_penalty_", &unresponsive_penalty_);
	bind("P_testFRp_", &P_testFRp_);
}


void RedPDQueue::reset() {

	REDQueue::reset();
	
	//probably should also reset the attached flow monitor and all the flows in it.
}

/*
 * Receive a new packet arriving at the queue.
 *    Check if the incoming flow belongs to a flow being monitored
 *    If YES, 
 *         drop the packet with probability associated with this flow.
 *         if the packet survives, put it in the regular RED queue
 *    if NO,
 *        put it in the regular RED queue 
 */

void RedPDQueue::enque(Packet* pkt) {
	
	double P_monFlow=0;

	//	hdr_ip* iph = (hdr_ip*)pkt->access(off_ip_);    
	//	int fid = iph->flowid();
	//	int src_ = iph->saddr();
	
	if (flowMonitor_ == NULL) {
		printf("RedPD: ERROR: FlowMonitor Not Found --\n");
		abort();
	}	
	
	RedPDFlow * flow = (RedPDFlow *) flowMonitor_->find(pkt);
	
	if (flow == NULL) {
		printf("RedPD: ERROR: Flow Not Found\n");
		abort();
	}
	
// 	if (debug_) {
// 		printf("flow - %s %d", flow->name(), flow->monitored_);
// 		if (flow->monitored()) 
// 			printf("RedPD: Got a monitored flow :)\n");
// 		else
// 			printf("RedPD: Unmonitored flow :(\n");
// 	}

	if (flow->monitored()) {

		//update the current estimate 
		//if automatic arrival rate estimation is taking place.
		if (flow->auto_) {
			flow->currentBW_ = flow->estRate_;
		}
		
		//calculate drop probability - use the global target if global_target_ is set
		if (global_target_) { 
			if (flow->unresponsive()) {
				P_monFlow = getP_monFlow(flow->currentBW_, 
							 targetBW_/unresponsive_penalty_);
			} else { 
				P_monFlow = getP_monFlow(flow->currentBW_, targetBW_);
			} 
		} else { 
			P_monFlow = getP_monFlow(flow->currentBW_, flow->targetBW_);
		}
		
		if (P_monFlow != 0) {
			flow->lastDropTime_ = Scheduler::instance().clock();
			double mod_p = modify_p(P_monFlow, flow->count, 0, 0, 0, 0, 0);

			P_monFlow = mod_p;
			double u = Random::uniform();
			
			//don't drop a packet if ave q size is small and the flow is responsive.
			//drop packet from the unresponsive flow if probability says so.
			int qlen = qib_ ? bcount_ : q_->length();
			if ( (u <= P_monFlow && 2*edv_.v_ave >= edp_.th_min && 
			      qlen > 1 && !flow->unresponsive_) || 
			     (u <= P_monFlow) ) {
								
				//first trace the monitored early drop
				 if (MEDTrace!= NULL) 
					((Trace *)MEDTrace)->recvOnly(pkt);
								
				flowMonitor_->mon_edrop(pkt);
				
				//there is a bug here, this packet drop does not go to
				// any other flow monitor attached to the link. 
				//departures and arrivals still go there if you wanna calculate.
				Packet::free(pkt);

				flow->count = 0;
				return;
			}
			else {
				flow->count++;
			}
		}
	}
	
	//if not dropped or a non-monitored packet - send it to the RED queue 
	// - but before see if testFRp mode is on
	if (P_testFRp_ != -1) {
		if (debug_) 
			printf("FRp_ mode ON with %g\n",P_testFRp_); 
		double u = Random::uniform();
		if (u <= P_testFRp_) {
			drop(pkt);
			return;
		}
	}
	
	REDQueue::enque(pkt);
}


int RedPDQueue::command(int argc, const char*const* argv) {

	Tcl& tcl = Tcl::instance();
	if (argc==2) {
		if (strcmp(argv[1], "mon-edrop-trace") == 0) {
			if (MEDTrace != NULL) {
				tcl.resultf("%s", MEDTrace->name());
				printf("Exists according to RedPD\n");
			}
			else {
				printf("Doesn't exist according to RedPD\n");
				tcl.resultf("0");
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mon-trace-type") == 0) {
			tcl.resultf("%s",medTraceType);
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		//$queue attach-flowmon $flowMon
		if (strcmp(argv[1], "attach-flowmon") == 0) {
			
			flowMonitor_ = (FlowMon *) TclObject::lookup(argv[2]);
			if (flowMonitor_ == NULL) {
				if (debug_) printf("Error Creating Flowmonitor\n");
                                return (TCL_ERROR);
			}
			if (debug_) 
				printf("RedPD: Flow Monitor Set to %s\n", flowMonitor_->name());
	
			de_drop_ = (NsObject *) flowMonitor_;
                        return (TCL_OK);
		}
		//$queue showme $flow
		//prints the monitoring status of the flow
		else if (strcmp(argv[1], "showme") == 0) {

			RedPDFlow * flow = (RedPDFlow *) TclObject::lookup(argv[2]);
			printf("showing now : %s = %d\n", flow->name(), flow->monitored_);
			
			return (TCL_OK);
		}
		//$queue mon-edrop-trace $trace
		//attaches the trace object to the queue
		else if (strcmp(argv[1], "mon-edrop-trace") == 0) {
			
			MEDTrace = (NsObject *) TclObject::lookup(argv[2]);
			
                        if (MEDTrace == NULL) {
				if (debug_) printf("Error Attaching Trace\n");
                                return (TCL_ERROR);
			}
			if (debug_) 
				printf("RedPD: MEDTrace Set to %s\n", flowMonitor_->name());
		
			return (TCL_OK);
		}
		//$queue unmonitor-flow $flow
		else if (strcmp(argv[1], "unmonitor-flow") == 0) {
			RedPDFlow * flow = (RedPDFlow *) TclObject::lookup(argv[2]);
			
			if (flow->monitored_ != 1) {
				tcl.resultf("Cannot unmonitor an unmonitored flow: %d\n", flow->flowid());
				return(TCL_ERROR);
			}

			flow->monitored_ = 0;
			flow->unresponsive_ = 0;
			flow->monitorStartTime_ = 0;
			flow->lastDropTime_ = 0;
			flow->unresponsiveStartTime_ = 0;

			noMonitored_--;

			if ( noMonitored_ < 0 ) {
				tcl.resultf("noMonitored gone below ZERO\n");
				return TCL_ERROR;
			}

			return TCL_OK;
		}
		//$queue unresponsive-flow $flow
		//declare a flow unresponsive
		else if (strcmp(argv[1], "unresponsive-flow") == 0) {
			RedPDFlow * flow = (RedPDFlow *) TclObject::lookup(argv[2]);
			
			if (flow->monitored_ != 1) {
				tcl.resultf("Cannot make an unmonitored flow unresponsive: %d\n", 
					    flow->flowid());
				return(TCL_ERROR);
			}

			if (flow->unresponsive_ != 1) {
				flow->unresponsive_ = 1;
				flow->unresponsiveStartTime_ = Scheduler::instance().clock();
			}

			if (flow->auto_) {
				flow->estimate_rate_=1;
			}
			
			return TCL_OK;
		}
		//$queue responsive-flow $flow
		else if (strcmp(argv[1], "responsive-flow") == 0) {
			RedPDFlow * flow = (RedPDFlow *) TclObject::lookup(argv[2]);
			
			if (flow->unresponsive_ != 1) {
				tcl.resultf("Cannot make a responsive flow responsive: %d\n", 
					    flow->flowid());
				return(TCL_ERROR);
			}
			flow->unresponsive_ = 0;
			flow->unresponsiveStartTime_ = 0;
			
			return TCL_OK;
		}
	}
	else if (argc == 4) {
		//$queue monitor-flow $flow $prob
		//monitor a flow with probability $prob
		if (strcmp(argv[1], "monitor-flow") == 0) {
			//this is a round about way of doing things, but ... historical
			//monitoring a flow with probability p, is same as 
			//monitoring it with targetBW 1-p and currentBW 1. 
			tcl.evalf("%s monitor-flow %s %g 1",name(), argv[2], 1 - atof(argv[3]));
		}
	}
	else if (argc == 5) {
		//$queue monitor-flow $flow $targetBW $currentBW 
		if (strcmp(argv[1], "monitor-flow") == 0) {
			RedPDFlow * flow = (RedPDFlow *) TclObject::lookup(argv[2]);
			
			tcl.evalf("%s set targetBW_ %s", flow->name(), argv[3]);
			tcl.evalf("%s set currentBW_ %s", flow->name(), argv[4]);
			
			if (flow->monitored_ != 1) {
				flow->monitored_=1;
				noMonitored_ ++;
				flow->monitorStartTime_ = Scheduler::instance().clock();
			}
			
			//if auto_ is ON initialize the rate estimation with the current bandwidth
			if (auto_) {
				flow->estimate_rate_=1;
				flow->estRate_ = flow->currentBW_;
			}
			
			return (TCL_OK);
		}
	}
	
	return (REDQueue::command(argc, argv));
}

