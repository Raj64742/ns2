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
 * dsredq.h
 *
 */

#ifndef dsredq_h
#define dsredq_h

//#include "dsred.h"

#define MAX_PREC 3	// maximum number of virtual RED queues in one physical queue

enum mredModeType {rio_c, rio_d, wred, dropTail};


/*------------------------------------------------------------------------------
struct qParam
    This structure specifies the parameters needed to be maintained for each
RED queue.
------------------------------------------------------------------------------*/
struct qParam {
	edp edp_;		// early drop parameters (see red.h)
	edv edv_;		// early drop variables (see red.h)
	int qlen;		// actual (not weighted) queue length in packets
	double idletime_;	// needed to calculate avg queue
	bool idle_;		// needed to calculate avg queue
};

/*------------------------------------------------------------------------------
class redQueue
    This class provides specs for one physical queue.
------------------------------------------------------------------------------*/
class redQueue {
public:
	int numPrec;	// the current number of precedence levels (or virtual queues)
	int qlim;
	mredModeType mredMode;
	redQueue();
	void config(int prec, const char*const* argv);	// configures one virtual RED queue
	void initREDStateVar(void);		// initializes RED state variables
	void updateREDStateVar(int prec);	// updates RED variables after dequing a packet
        // enques packets into a physical queue
	int enque(Packet *pkt, int prec, int ecn);
	Packet* deque(void);			// deques packets
	double getWeightedLength();
	int getRealLength(void);		// queries length of a physical queue
        // sets packet time constant values
        //(needed for calc. avgQSize) for each virtual queue
	void setPTC(double outLinkBW);
        // sets mean packet size (needed to calculate avg. queue size)
	void setMPS(int mps);


private:
	PacketQueue *q_;		// underlying FIFO queue
        // used to maintain parameters for each of the virtual queues
	qParam qParam_[MAX_PREC];
	void calcAvg(int prec, int m); // to calculate avg. queue size of a virtual queue

};
#endif

