/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (C) 2004 by USC/ISI
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcp.h,v 1.7 2005/01/13 18:39:06 haldar Exp $
 */

#ifndef NS_XCP
#define NS_XCP

#include "packet.h"
#include "queue.h"
#include "xcpq.h"


enum {XCPQ=0, TCPQ=1, OTHERQ=2, MAX_QNUM};

// code points for separating XCP/TCP flows
#define CP_XCP 10
#define CP_TCP 20
#define CP_OTHER 30


class XCPWrapQ : public Queue {
public:
	XCPWrapQ();
	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler*);

protected:
	Queue		*q_[MAX_QNUM];

	XCPQueue 	*xcpq_; // same as q_[XCPQ]

	unsigned int    routerId_;
  
	int qToDq_;                 // current queue being dequed
	double wrrTemp_[MAX_QNUM];     // state of queue being serviced
	double queueWeight_[MAX_QNUM]; // queue weight for each queue (dynamic)

	int spread_bytes_;
	bool tcp_xcp_on_;	    // true if XCP/TCP sharing is enables
	
	// Modified functions
	virtual void enque(Packet* pkt); // high level enque function
	virtual Packet* deque();         // high level deque function
    
	void addQueueWeights(int queueNum, int weight);
	int queueToDeque();              // returns qToDq
	int queueToEnque(int codePt);    // returns queue based on codept
	void mark(Packet *p);             // marks pkt based on flow type
	int getCodePt(Packet *p);        // returns codept in pkt hdr
	void setVirtualQueues();         // setup virtual queues(for xcp/tcp)
};

#endif //NS_XCP
