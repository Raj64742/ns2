/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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


#include "xcp.h"

static unsigned int next_router = 0;

static class XCPClass : public TclClass {
public:
	XCPClass() : TclClass("Queue/XCP") {}
	TclObject* create(int, const char*const*) {
		return (new XCPWrapQ);
	}
} class_xcp_queue;

XCPWrapQ::XCPWrapQ():xcpq_(NULL)
{
	bind("maxVirQ_", &maxVirQ_);
	bind("spread_bytes_", &spread_bytes_);
	routerId_ = next_router++;

	// XXX Temporary fix XXX
	// set flag to 1 when supporting both tcp and xcp flows; 
	// temporary fix for allocating link BW between xcp and 
	// tcp queues until dynamic queue weights come into effect. 
	// This fix should then go away.
	
	Tcl& tcl = Tcl::instance();
	tcl.evalf("Queue/XCP set tcp_xcp_on_");
	if (strcmp(tcl.result(), "0") != 0)  
		tcp_xcp_on_ = 1;              //tcp_xcp_on flag is set
}
 
void XCPWrapQ::setVirtualQueues() {

	for (int n=0; n < maxVirQ_; n++) {
		xcpq_[n]->routerId(this,routerId_); 
		wrrTemp_[n] = 0;
	}
  
	qToDq_ = 0;
	queueWeight_[XCPQ] = 0.5;
	queueWeight_[TCPQ] = 0.5;
	queueWeight_[OTHERQ] = 0;
  
	// setup timers for xcp queue only
	xcpq_[XCPQ]->setupTimers();
	xcpq_[XCPQ]->spread_bytes(spread_bytes_);

}


int XCPWrapQ::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		// for Dina's parking-lot experiment data
		if (strcmp(argv[1], "queue-read-drops") == 0) {
			tcl.resultf("%g",xcpq_[XCPQ]->totalDrops());
			return (TCL_OK);
		} 
	}

	if (argc >= 3) {	
		if (strcmp(argv[1], "set-virQ") == 0) {
			xcpq_ = new XCPQueue*[maxVirQ_]; 
			for (int n=0,c=2; n < maxVirQ_; n++) {
				xcpq_[n] = (XCPQueue *)(TclObject::lookup(argv[c++]));
				if (xcpq_[n] == NULL) {
					tcl.add_errorf("Wrong xcp virtual queues %s\n",argv[c-1]);
					return TCL_ERROR;
				}
			}
			setVirtualQueues();
			return TCL_OK;
		}

		else if (strcmp(argv[1], "set-link-capacity") == 0) {
			double link_capacity_bitps = strtod(argv[2], 0);
			if (link_capacity_bitps < 0.0) {
				printf("Error: BW < 0"); 
				exit(1);
			}
			if (tcp_xcp_on_) // divide between xcp and tcp queues
				link_capacity_bitps = link_capacity_bitps/2.0;
			
			for (int n=0; n < maxVirQ_; n++) {
				xcpq_[n]->setBW(link_capacity_bitps/8.0); 
				xcpq_[n]->limit(limit());
				xcpq_[n]->config();
			}
			return TCL_OK;
		}
    
		else if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			Tcl_Channel queue_trace_file = Tcl_GetChannel(tcl.interp(), (char*)id, &mode);
			if (queue_trace_file == 0) {
				tcl.resultf("queue.cc: trace-drops: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			for (int n=0; n < maxVirQ_; n++)
				xcpq_[n]->setChannel(queue_trace_file);
			return (TCL_OK);
		}
    
		else if (strcmp(argv[1], "queue-sample-everyrtt") == 0) {
			double e_rtt = strtod(argv[2],0);
			printf(" timer at %f \n",e_rtt);
			xcpq_[XCPQ]->setEffectiveRtt(e_rtt);
			return (TCL_OK);
		}
    
		else if (strcmp(argv[1], "num-mice") == 0) {
			int nm = atoi(argv[2]);
			for (int n=0; n < maxVirQ_; n++)
				xcpq_[n]->setNumMice(nm);
			return (TCL_OK);
		}
	}
	return (Queue::command(argc, argv));
}


void XCPWrapQ::recv(Packet* p, Handler* h)
{
	mark(p);
	Queue::recv(p, h);
}

void XCPWrapQ::addQueueWeights(int queueNum, int weight) {
	if (queueNum < maxVirQ_)
		queueWeight_[queueNum] = weight;
	else {
		fprintf(stderr, "Queue number is out of range.\n");
	}
}

int XCPWrapQ::queueToDeque()
{
	int i = 0;
  
	if (wrrTemp_[qToDq_] <= 0) {
		qToDq_ = ((qToDq_ + 1) % maxVirQ_);
		wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
	} else {
		wrrTemp_[qToDq_] = wrrTemp_[qToDq_] -1;
	}
	while ((i < maxVirQ_) && (xcpq_[qToDq_]->length() == 0)) {
		wrrTemp_[qToDq_] = 0;
		qToDq_ = ((qToDq_ + 1) % maxVirQ_);
		wrrTemp_[qToDq_] = queueWeight_[qToDq_] - 1;
		i++;
	}
	return (qToDq_);
}

int XCPWrapQ::queueToEnque(int cp)
{
	int n;
	switch (cp) {
	case CP_XCP:
		return (n = XCPQ);
    
	case CP_TCP:
		return (n = TCPQ);
    
	case CP_OTHER:
		return (n = OTHERQ);

	default:
		fprintf(stderr, "Unknown codepoint %d\n", cp);
		exit(1);
	}
}


// Extracts the code point marking from packet header.
int XCPWrapQ::getCodePt(Packet *p) {

	hdr_ip* iph = hdr_ip::access(p);
	return(iph->prio());
}

Packet* XCPWrapQ::deque()
{
	Packet *p = NULL;
	int n = queueToDeque();
	
// Deque a packet from the underlying queue:
	p = xcpq_[n]->deque();
	return(p);
}


void XCPWrapQ::enque(Packet* pkt)
{
	int codePt;
  
	codePt = getCodePt(pkt);
	int n = queueToEnque(codePt);
  
	xcpq_[n]->enque(pkt);
}


void XCPWrapQ::mark(Packet *p) {
  
	int codePt;
	hdr_cmn* cmnh = hdr_cmn::access(p);
	hdr_xcp *xh = hdr_xcp::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	
	if ((codePt = iph->prio_) > 0)
		return;
	
	else {
		if (xh->xcp_enabled_ != hdr_xcp::XCP_DISABLED)
			codePt = CP_XCP;
		else 
			if (cmnh->ptype() == PT_TCP || cmnh->ptype() == PT_ACK)
				codePt = CP_TCP;
			else // for others
				codePt = CP_OTHER;
    
		iph->prio_ = codePt;
	}
}
