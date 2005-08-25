// -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-

/*
 * Copyright (C) 2004 by the University of Southern California
 * Copyright (C) 2004 by USC/ISI
 * $Id: xcp.h,v 1.8 2005/08/25 18:58:14 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */
/*
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcp.h,v 1.8 2005/08/25 18:58:14 johnh Exp $
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
