/*
 * Copyright (c) 2000 Nortel Networks
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
 *      This product includes software developed by Nortel Networks.
 * 4. The name of the Nortel Networks may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORTEL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NORTEL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 */

#include <stdio.h>
#include "ip.h"
#include "dsred.h"
#include "delay.h"
#include "random.h"
#include "flags.h"
#include "tcp.h"
#include "dsredq.h"


/*------------------------------------------------------------------------------
dsREDClass declaration. 
    Links the new class in the TCL heirarchy.  See "Notes And Documentation for 
ns-2."
------------------------------------------------------------------------------*/
static class dsREDClass : public TclClass {
public:
	dsREDClass() : TclClass("Queue/dsRED") {}
	TclObject* create(int, const char*const*) {
		return (new dsREDQueue);
	}
} class_dsred;


/*------------------------------------------------------------------------------
dsREDQueue() Constructor.
    Initializes the queue.  Note that the default value assigned to numQueues 
in tcl/lib/ns-default.tcl must be no greater than MAX_QUEUES (the physical 
queue array size).
------------------------------------------------------------------------------*/
dsREDQueue::dsREDQueue() : de_drop_(NULL), link_(NULL)   {
	bind("numQueues_", &numQueues_);
	bind_bool("ecn_", &ecn_);
	int i;

	numPrec = MAX_PREC;
   schedMode = schedModeRR;

   for(i=0;i<MAX_QUEUES;i++){
		queueMaxRate[i] = 0;
      queueWeight[i]=1;
   }
		
   queuesDone = MAX_QUEUES;
	phbEntries = 0;		// Number of entries in PHB table
	
	reset();
}


/*------------------------------------------------------------------------------
void edrop(Packet* pkt)
    This method is used so that flowmonitor can monitor early drops.
------------------------------------------------------------------------------*/
void dsREDQueue::reset() {
	int i;

	qToDq = 0;		// q to be dequed, initialized to 0	

   for(i=0;i<MAX_QUEUES;i++){
		queueAvgRate[i] = 0.0;
		queueArrTime[i] = 0.0;
      slicecount[i]=0;
      pktcount[i]=0;
      wirrTemp[i]=0;
      wirrqDone[i]=0;
   }

	stats.drops = 0;
	stats.edrops = 0;
	stats.pkts = 0;

	for(i=0;i<MAX_CP;i++){
		stats.drops_CP[i]=0;
		stats.edrops_CP[i]=0;
		stats.pkts_CP[i]=0;
	}

	for (i = 0; i < MAX_QUEUES; i++)
		redq_[i].qlim = limit();
	
	// Compute the "packet time constant" if we know the
	// link bandwidth.  The ptc is the max number of (avg sized)
	// pkts per second which can be placed on the link.
	if (link_)
		for (int i = 0; i < MAX_QUEUES; i++)
			redq_[i].setPTC(link_->bandwidth());
	
	Queue::reset();
}


/*------------------------------------------------------------------------------
void edrop(Packet* pkt)
    This method is used so that flowmonitor can monitor early drops.
------------------------------------------------------------------------------*/
void dsREDQueue::edrop(Packet* p)
{

	if (de_drop_ != 0){
		de_drop_->recv(p);
	}
	else {
		drop(p);
	}
}


/*------------------------------------------------------------------------------
void applyTSWMeter(Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
------------------------------------------------------------------------------*/
void dsREDQueue::applyTSWMeter(Packet *pkt) {
	double now, bytesInTSW, newBytes;
	hdr_cmn* hdr = hdr_cmn::access(pkt);
	double winLen = 1.0;

	bytesInTSW = queueAvgRate[qToDq] * winLen;
	newBytes = bytesInTSW + (double) hdr->size();
	now = Scheduler::instance().clock();
	queueAvgRate[qToDq] = newBytes / (now - queueArrTime[qToDq] + winLen);
	queueArrTime[qToDq] = now;
	
}


/*------------------------------------------------------------------------------
void enque(Packet* pkt) 
    The following method outlines the enquing mechanism for a Diffserv router.
This method is not used by the inheriting classes; it only serves as an 
outline.
------------------------------------------------------------------------------*/
void dsREDQueue::enque(Packet* pkt) {
   int codePt, queue, prec;
   hdr_ip* iph = hdr_ip::access(pkt);
   codePt = iph->prio();	//extracting the marking done by the edge router
   int ecn = 0;

   //looking up queue and prec numbers for that codept
   lookupPHBTable(codePt, &queue, &prec);	

   // code added for ECN support
   //hdr_flags* hf = (hdr_flags*)(pkt->access(off_flags_));
   // Changed for the latest version instead of 2.1b6
   hdr_flags* hf = hdr_flags::access(pkt);

   if (ecn_ && hf->ect()) ecn = 1;

	stats.pkts_CP[codePt]++;
	stats.pkts++;

   switch(redq_[queue].enque(pkt, prec, ecn)) {
      case PKT_ENQUEUED:
         break;
      case PKT_DROPPED:
         stats.drops_CP[codePt]++;
	 stats.drops++;
         drop(pkt);
         break;
      case PKT_EDROPPED:
	 stats.edrops_CP[codePt]++;
	 stats.edrops++;
         edrop(pkt);
         break;
      case PKT_MARKED:
         hf->ce() = 1; 	// mark Congestion Experienced bit		
         break;			
      default:
         break;
   }
}


/*------------------------------------------------------------------------------
Packet* deque() 
    This method implements the dequing mechanism for a Diffserv router.
------------------------------------------------------------------------------*/
Packet* dsREDQueue::deque() {
	Packet *p;
	int queue, prec;
   hdr_ip* iph;
   int fid;

	// Select queue to deque in a round robin manner:
	selectQueueToDeque();

	// Dequeue a packet from the underlying queue:
	p = redq_[qToDq].deque();

	if (p != 0) { 
      iph= hdr_ip::access(p);
      fid = iph->flowid()/32;
      pktcount[qToDq]+=1;

		if (schedMode==schedModePRI && queueMaxRate[qToDq]) applyTSWMeter(p);

      /* There was a packet to be dequed;
         find the precedence level (or virtual queue)
         to which this packet was attached:
      */
      lookupPHBTable(getCodePt(p), &queue, &prec);

      // update state variables for that "virtual" queue
      redq_[qToDq].updateREDStateVar(prec);
	}

	// Return the dequed packet:	
	return(p);
}


/*------------------------------------------------------------------------------
int getCodePt(Packet *p) 
    This method, when given a packet, extracts the code point marking from its 
header.
------------------------------------------------------------------------------*/
int dsREDQueue::getCodePt(Packet *p) {
	hdr_ip* iph = hdr_ip::access(p);
	return(iph->prio());
}


/*------------------------------------------------------------------------------
void selectQueueToDeque(void) 
    This method determines the next queue in line to be dequed.
------------------------------------------------------------------------------*/
void dsREDQueue::selectQueueToDeque() {
   // If the queue to be dequed has no elements, look for the next queue in line:
   int i = 0;

   if(schedMode==schedModeRR){
		qToDq = ((qToDq + 1) % numQueues_);
      while ((i < numQueues_) && (redq_[qToDq].getRealLength() == 0)) {
			qToDq = ((qToDq + 1) % numQueues_);			
         i++;
      }
   }
   else if (schedMode==schedModeWRR) {
      if(wirrTemp[qToDq]<=0){
  	      qToDq = ((qToDq + 1) % numQueues_);
     	   wirrTemp[qToDq] = queueWeight[qToDq] - 1;
		} else {
			wirrTemp[qToDq] = wirrTemp[qToDq] -1;
		}			
      while ((i < numQueues_) && (redq_[qToDq].getRealLength() == 0)) {
			wirrTemp[qToDq] = 0;
  	      qToDq = ((qToDq + 1) % numQueues_);
     	   wirrTemp[qToDq] = queueWeight[qToDq] - 1;
			i++;
		}

   }
   else if (schedMode==schedModeWIRR) {
      qToDq = ((qToDq + 1) % numQueues_);
		while ((i<numQueues_) && ((redq_[qToDq].getRealLength()==0) || (wirrqDone[qToDq]))) {
			if (!wirrqDone[qToDq]) {
				queuesDone++;
				wirrqDone[qToDq]=1;
			}
  	      qToDq = ((qToDq + 1) % numQueues_);
			i++;
		}
      if (wirrTemp[qToDq] == 1) {
  	       queuesDone +=1;
          wirrqDone[qToDq]=1;
      }
  	   wirrTemp[qToDq]-=1;
      if(queuesDone >= numQueues_) {
          queuesDone = 0;
          for(i=0;i<numQueues_;i++) {
  	          wirrTemp[i] = queueWeight[i];
     	       wirrqDone[i]=0;
          }   	
      }
   }
	else if (schedMode==schedModePRI) {
		qToDq = 0;
		while ((i < numQueues_) && ((redq_[qToDq].getRealLength() == 0) ||
			((queueAvgRate[qToDq] > queueMaxRate[qToDq]) && queueMaxRate[qToDq]))) {
			i++;
			qToDq = i;
		}
		if (i == numQueues_) {
			i = qToDq = 0;			
			while ((i < numQueues_) && (redq_[qToDq].getRealLength() == 0)) {
				qToDq = ((qToDq + 1) % numQueues_);
				i++;
			}
		}
	}
}	


/*------------------------------------------------------------------------------
void lookupPHBTable(int codePt, int* queue, int* prec)
    Assigns the queue and prec parameters values corresponding to a given code 
point.  The code point is assumed to be present in the PHB table.  If it is 
not, an error message is outputted and queue and prec are undefined.
------------------------------------------------------------------------------*/
void dsREDQueue::lookupPHBTable(int codePt, int* queue, int* prec) {
   for (int i = 0; i < phbEntries; i++) {
      if (phb_[i].codePt_ == codePt) {
         *queue = phb_[i].queue_;
         *prec = phb_[i].prec_;
         return;
      }
   }
   printf("ERROR: No match found for code point %d in PHB Table.\n", codePt);
}


/*------------------------------------------------------------------------------
void addPHBEntry(int codePt, int queue, int prec)
    Add a PHB table entry.  (Each entry maps a code point to a queue-precedence
pair.)
------------------------------------------------------------------------------*/
void dsREDQueue::addPHBEntry(int codePt, int queue, int prec) {
	if (phbEntries == MAX_CP) {
      printf("ERROR: PHB Table size limit exceeded.\n");
	} else {
		phb_[phbEntries].codePt_ = codePt;
		phb_[phbEntries].queue_ = queue;
		phb_[phbEntries].prec_ = prec;
		stats.valid_CP[codePt] = 1;
		phbEntries++;
	}
}


/*------------------------------------------------------------------------------
void addPHBEntry(int codePt, int queue, int prec)
    Add a PHB table entry.  (Each entry maps a code point to a queue-precedence
pair.)
------------------------------------------------------------------------------*/
double dsREDQueue::getStat(int argc, const char*const* argv) {

	if (argc == 3) {
		if (strcmp(argv[2], "drops") == 0)
         return (stats.drops*1.0);
		if (strcmp(argv[2], "edrops") == 0)
         return (stats.edrops*1.0);
		if (strcmp(argv[2], "pkts") == 0)
         return (stats.pkts*1.0);
   }
	if (argc == 4) {
		if (strcmp(argv[2], "drops") == 0)
         return (stats.drops_CP[atoi(argv[3])]*1.0);
		if (strcmp(argv[2], "edrops") == 0)
         return (stats.edrops_CP[atoi(argv[3])]*1.0);
		if (strcmp(argv[2], "pkts") == 0)
         return (stats.pkts_CP[atoi(argv[3])]*1.0);
	}
	return -1.0;
}


/*------------------------------------------------------------------------------
void setNumPrec(int prec) 
    Sets the current number of drop precendences.  The number of precedences is
the number of virtual queues per physical queue.
------------------------------------------------------------------------------*/
void dsREDQueue::setNumPrec(int prec) {
	int i;

	if (prec > MAX_PREC) {
		printf("ERROR: Cannot declare more than %d prcedence levels (as defined by MAX_PREC)\n",MAX_PREC);
	} else {
		numPrec = prec;

		for (i = 0; i < MAX_QUEUES; i++)
			redq_[i].numPrec = numPrec;
	}
}

/*------------------------------------------------------------------------------
void setMREDMode(const char* mode)
   sets up the average queue accounting mode.
----------------------------------------------------------------------------*/
void dsREDQueue::setMREDMode(const char* mode, const char* queue) {
	int i;
	mredModeType tempMode;

	if (strcmp(mode, "RIO-C") == 0)
   	tempMode = rio_c;
	else if (strcmp(mode, "RIO-D") == 0)
		tempMode = rio_d;
	else if (strcmp(mode, "WRED") == 0)
		tempMode = wred;
	else if (strcmp(mode, "DROP") == 0)
		tempMode = dropTail;
	else {
		printf("Error: MRED mode %s does not exist\n",mode);
      return;
   }

	if (!queue)
		for (i = 0; i < MAX_QUEUES; i++)
   	   redq_[i].mredMode = tempMode;
	else
		redq_[atoi(queue)].mredMode = tempMode;

}


/*------------------------------------------------------------------------------
void printPHBTable()
    Prints the PHB Table, with one entry per line.
------------------------------------------------------------------------------*/
void dsREDQueue::printPHBTable() {
   printf("PHB Table:\n");
   for (int i = 0; i < phbEntries; i++)
      printf("Code Point %d is associated with Queue %d, Precedence %d\n", phb_[i].codePt_, phb_[i].queue_, phb_[i].prec_);
   printf("\n");
}


/*------------------------------------------------------------------------------
void printStats()
    An output method that may be altered to assist debugging.
------------------------------------------------------------------------------*/
void dsREDQueue::printStats() {
	printf("\nPackets Statistics\n");
	printf("=======================================\n");
	printf(" CP  TotPkts   TxPkts   ldrops   edrops\n");
	printf(" --  -------   ------   ------   ------\n");
	printf("All %8ld %8ld %8ld %8ld\n",stats.pkts,stats.pkts-stats.drops-stats.edrops,stats.drops,stats.edrops);
	for (int i = 0; i < MAX_CP; i++)
		if (stats.pkts_CP[i] != 0)
			printf("%3d %8ld %8ld %8ld %8ld\n",i,stats.pkts_CP[i],stats.pkts_CP[i]-stats.drops_CP[i]-stats.edrops_CP[i],stats.drops_CP[i],stats.edrops_CP[i]);

}


void dsREDQueue::printWRRcount() {
   int i;
   for (i = 0; i < numQueues_; i++){
      printf("%d: %d %d %d.\n", i, slicecount[i],pktcount[i],queueWeight[i]);
   }
}


/*------------------------------------------------------------------------------
void setSchedularMode(int schedtype)
   sets up the schedular mode.
----------------------------------------------------------------------------*/
void dsREDQueue::setSchedularMode(const char* schedtype) {
	if (strcmp(schedtype, "RR") == 0)
   	schedMode = schedModeRR;
	else if (strcmp(schedtype, "WRR") == 0)
		schedMode = schedModeWRR;
	else if (strcmp(schedtype, "WIRR") == 0)
		schedMode = schedModeWIRR;
	else if (strcmp(schedtype, "PRI") == 0)
		schedMode = schedModePRI;
	else
		printf("Error: Scheduler type %s does not exist\n",schedtype);
}


/*------------------------------------------------------------------------------
void addQueueWeights(int queueNum, int weight)
   An input method to set the individual Queue Weights.
----------------------------------------------------------------------------*/
void dsREDQueue::addQueueWeights(int queueNum, int weight) {

   if(queueNum < MAX_QUEUES){
      queueWeight[queueNum]=weight;
   } else {
      printf("The queue number is out of range.\n");
   }
}


/*------------------------------------------------------------------------------
void addQueueRate(int queueNum, int rate)
   An input method to set the individual Queue Max Rates for Priority Queueing.
----------------------------------------------------------------------------*/
void dsREDQueue::addQueueRate(int queueNum, int rate) {

   if(queueNum < MAX_QUEUES){
      queueMaxRate[queueNum]=(double)rate/8.0;
   } else {
      printf("The queue number is out of range.\n");
   }
}


/*------------------------------------------------------------------------------
int command(int argc, const char*const* argv)
    Commands from the ns file are interpreted through this interface.
------------------------------------------------------------------------------*/
int dsREDQueue::command(int argc, const char*const* argv)
{

	if (strcmp(argv[1], "configQ") == 0) {
		redq_[atoi(argv[2])].config(atoi(argv[3]), argv);
		return(TCL_OK);
	}
	if (strcmp(argv[1], "addPHBEntry") == 0) {
		addPHBEntry(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
		return (TCL_OK);
	}
	if (strcmp(argv[1], "meanPktSize") == 0) {
		for (int i = 0; i < MAX_QUEUES; i++)
			redq_[i].setMPS(atoi(argv[2]));
		return(TCL_OK);
	}
	if (strcmp(argv[1], "setNumPrec") == 0) {
		setNumPrec(atoi(argv[2]));
		return(TCL_OK);
	}
	if (strcmp(argv[1], "getAverage") == 0) {
		Tcl& tcl = Tcl::instance();
		tcl.resultf("%f", redq_[atoi(argv[2])].getWeightedLength());
		return(TCL_OK);
	}
	if (strcmp(argv[1], "getStat") == 0) {
		Tcl& tcl = Tcl::instance();
		tcl.resultf("%f", getStat(argc,argv));
		return(TCL_OK);
	}
	if (strcmp(argv[1], "getCurrent") == 0) {
		Tcl& tcl = Tcl::instance();
		tcl.resultf("%f", redq_[atoi(argv[2])].getRealLength()*1.0);
		return(TCL_OK);
	}
	if (strcmp(argv[1], "printStats") == 0) {
		printStats();
		return (TCL_OK);
	}
	if (strcmp(argv[1], "printWRRcount") == 0) {
		printWRRcount();
		return (TCL_OK);
	}
	if (strcmp(argv[1], "printPHBTable") == 0) {
		printPHBTable();
		return (TCL_OK);
	}
	if (strcmp(argv[1], "link") == 0) {
		Tcl& tcl = Tcl::instance();
		LinkDelay* del = (LinkDelay*) TclObject::lookup(argv[2]);
		if (del == 0) {
			tcl.resultf("RED: no LinkDelay object %s",
			argv[2]);
			return(TCL_ERROR);
		}
		link_ = del;
		return (TCL_OK);
	}
	if (strcmp(argv[1], "early-drop-target") == 0) {
		Tcl& tcl = Tcl::instance();
		NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
		if (p == 0) {
			tcl.resultf("no object %s", argv[2]);
			return (TCL_ERROR);
		}
		de_drop_ = p;
		return (TCL_OK);
	}
   if (strcmp(argv[1], "setSchedularMode") == 0) {
      setSchedularMode(argv[2]);
      return(TCL_OK);
   }
   if (strcmp(argv[1], "setMREDMode") == 0) {
		if (argc == 3)
	      setMREDMode(argv[2],0);
		else
			setMREDMode(argv[2],argv[3]);
      return(TCL_OK);
   }
   if (strcmp(argv[1], "addQueueWeights") == 0) {
      addQueueWeights(atoi(argv[2]), atoi(argv[3]));
      return(TCL_OK);
   }
   if (strcmp(argv[1], "addQueueRate") == 0) {
      addQueueRate(atoi(argv[2]), atoi(argv[3]));
      return(TCL_OK);
   }

	return(Queue::command(argc, argv));
}


