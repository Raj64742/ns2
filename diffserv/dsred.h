/*
 * Copyright (c) 2000, Nortel Networks.
 * All rights reserved.
 *
 * License is granted to copy, to use, to make and to use derivative
 * works for research and evaluation purposes.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 *
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 *
 */


/*
 * dsred.h
 *
 * The Positions of dsREDQueue, edgeQueue, and coreQueue in the Object Hierarchy.
 *
 * This class, i.e. "dsREDQueue", is positioned in the class hierarchy as follows:
 *
 *             Queue
 *               |
 *           dsREDQueue
 *
 *
 *   This class stands for "Differentiated Services RED Queue".  Since the
 * original RED does not support multiple parameters, and other functionality
 * needed by a RED gateway in a Diffserv architecture, this class was created to
 * support the desired functionality.  This class is then inherited by two more
 * classes, moulding the old hierarchy as follows:
 *
 *
 *             Queue
 *               |
 *           dsREDQueue
 *           |        |
 *     edgeQueue    coreQueue
 *
 *
 * These child classes correspond to the "edge" and "core" routers in a Diffserv
 * architecture.
 *
 */


#ifndef dsred_h
#define dsred_h

#include "red.h"	// need RED class specs (edp definition, for example)
#include "queue.h"	// need Queue class specs
#include "dsredq.h"


/* The dsRED class supports the creation of up to MAX_QUEUES physical queues at
each network device, with up to MAX_PREC virtual queues in each queue. */ 
#define MAX_QUEUES 8// maximum number of physical RED queues
#define MAX_PREC 3	// maximum number of virtual RED queues in one physical queue
#define MAX_CP 40	// maximum number of code points in a simulation
#define MEAN_PKT_SIZE 1000 	// default mean packet size, in bytes, needed for RED calculations

enum schedModeType {schedModeRR, schedModeWRR, schedModeWIRR, schedModePRI};

#define PKT_MARKED 3
#define PKT_EDROPPED 2
#define PKT_ENQUEUED 1
#define PKT_DROPPED 0


/*------------------------------------------------------------------------------
struct phbParam
    This struct is used to maintain entries for the PHB parameter table, used 
to map a code point to a physical queue-virtual queue pair.
------------------------------------------------------------------------------*/
struct phbParam {
   int codePt_;
   int queue_;	// physical queue
   int prec_;	// virtual queue (drop precedence)
};

struct statType {
	long drops;       // per queue stats
	long edrops;
	long pkts;
	long valid_CP[MAX_CP];  // per CP stats
	long drops_CP[MAX_CP];
	long edrops_CP[MAX_CP];
	long pkts_CP[MAX_CP];
};


/*------------------------------------------------------------------------------
class dsREDQueue 
    This class specifies the characteristics for a Diffserv RED router.
------------------------------------------------------------------------------*/
class dsREDQueue : public Queue {
public:	
	dsREDQueue();
	int command(int argc, const char*const* argv);	// interface to ns scripts

protected:
	redQueue redq_[MAX_QUEUES];	// the physical queues at the router
	NsObject* de_drop_;		// drop_early target
	statType stats; // used for statistics gatherings
	int qToDq;			// current queue to be dequeued in a round robin manner
	int numQueues_;			// the number of physical queues at the router
	int numPrec;			// the number of virtual queues in each physical queue
	phbParam phb_[MAX_CP];		// PHB table
	int phbEntries;     		// the current number of entries in the PHB table
	int ecn_;			// used for ECN (Explicit Congestion Notification)
	LinkDelay* link_;		// outgoing link
   int schedMode;                  // the Queue Scheduling mode
   int queueWeight[MAX_QUEUES];    // A queue weight per queue
	double queueMaxRate[MAX_QUEUES];   // Maximum Rate for Priority Queueing
	double queueAvgRate[MAX_QUEUES];   // Average Rate for Priority Queueing
	double queueArrTime[MAX_QUEUES];	  // Arrival Time for Priority Queueing
   int slicecount[MAX_QUEUES];
   int pktcount[MAX_QUEUES];
   int wirrTemp[MAX_QUEUES];
   unsigned char wirrqDone[MAX_QUEUES];
   int queuesDone;

	void reset();
	void edrop(Packet* p); // used so flowmonitor can monitor early drops
	void enque(Packet *pkt); // enques a packet
	Packet *deque(void);	// deques a packet
	int getCodePt(Packet *p); // given a packet, extract the code point marking from its header field
	void selectQueueToDeque();	// round robin scheduling dequing algorithm
	void lookupPHBTable(int codePt, int* queue, int* prec); // looks up queue and prec numbers corresponding to a code point
	void addPHBEntry(int codePt, int queue, int prec); // edits phb entry in the table
	void setNumPrec(int curPrec);
	void setMREDMode(const char* mode, const char* queue);
	void printStats(); // print various stats
	double getStat(int argc, const char*const* argv);
	void printPHBTable();  // print the PHB table
   void setSchedularMode(const char* schedtype); // Sets the schedular mode
   void addQueueWeights(int queueNum, int weight); // Add a maxRate to a PRI queue
   void addQueueRate(int queueNum, int rate); // Add a weigth to a WRR or WIRR queue
	void printWRRcount();		// print various stats
	void applyTSWMeter(Packet *pkt); // apply meter to calculate average rate of a PRI queue
};

#endif