// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
//  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
//  This software is developed at Rensselaer Polytechnic Institute under 
//  DARPA grant No. F30602-97-C-0274
//  Redistribution and use in source and binary forms are permitted
//  provided that the above copyright notice and this paragraph are
//  duplicated in all such forms and that any documentation, advertising
//  materials, and other materials related to such distribution and use
//  acknowledge that the software was developed by Mingzhou Sun at the
//  Rensselaer  Polytechnic Institute.  The name of the University may not 
//  be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/linkstate/rtProtoLS.h,v 1.1 2000/07/27 01:29:16 haoboy Exp $

#ifndef ns_rtprotols_h
#define ns_rtprotols_h

#include "packet.h"
#include "agent.h"
#include "ip.h"
#include "ls.h" 

extern LsMessageCenter messageCenter;

struct hdr_LS {
        u_int32_t mv_;  // metrics variable identifier
	int msgId_;

        u_int32_t& metricsVar() { return mv_; }
	int& msgId() { return msgId_; }
};

//  ---------- Generic Agent stuffs ----------------
class rtProtoLS : public Agent , public LsNode {
public:
        rtProtoLS() : Agent(PT_RTPROTO_LS) { 
		bind("off_LS_", &off_LS_);
		LS_ready_ = 0;
	}
        int command(int argc, const char*const* argv);
        void sendpkt(ns_addr_t dst, u_int32_t z, u_int32_t mtvar);
        void recv(Packet* p, Handler*);

protected:
	int off_LS_;

	void initialize(); // init nodeState_ and routing_
	void setDelay(int nbrId, double delay) {
		delayMap_.insert(nbrId, delay);
	}
	void sendBufferedMessages() { routing_.sendBufferedMessages(); }
	void computeRoutes() { routing_.computeRoutes(); }
	void intfChanged();
	void sendUpdates() { routing_.sendLinkStates(); }
	void lookup(int destinationNodeId);

public:
	bool sendMessage(int destId, u_int32_t messageId, int size);
	void receiveMessage(int sender, u_int32_t msgId);

	int getNodeId() { return nodeId_; }
	LsLinkStateList* getLinkStateListPtr()  { return &linkStateList_; }
	LsNodeIdList* getPeerIdListPtr() { return &peerIdList_; }
	LsDelayMap* getDelayMapPtr() { 
		return delayMap_.empty() ? (LsDelayMap *)NULL : &delayMap_;
	}
	void installRoutes() {
		Tcl::instance().evalf("%s route-changed", name());
	}

private:
	typedef LsMap<int, ns_addr_t> PeerAddrMap; // addr for peer Id
	PeerAddrMap peerAddrMap_;
	int nodeId_;
	int LS_ready_;	// to differentiate fake and real LS, debug, 0 == no
			// needed in recv and sendMessage;

	LsLinkStateList linkStateList_;
	LsNodeIdList peerIdList_;
	LsDelayMap delayMap_;
	LsRouting routing_;

	int findPeerNodeId(ns_addr_t agentAddr);
};

#endif // ns_rtprotols_h
