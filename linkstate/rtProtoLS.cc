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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/linkstate/rtProtoLS.cc,v 1.5 2002/05/30 17:44:04 haldar Exp $

#include "config.h"
#ifdef HAVE_STL

#include "hdr-ls.h"
#include "rtProtoLS.h"
#include "agent.h"
#include "string.h" // for strtok

// Helper classes
class LsTokenList : public LsList<char *> {
public:
	LsTokenList (char * str, const char * delim ) 
		: LsList<char*> () { 
		for ( char * token = strtok (str, delim);
		      token != NULL; token = strtok(NULL, delim) ) {
			push_back (token);
		}
	}
};
   
class LsIntList : public LsList<int> {
public:
	LsIntList (char * str, const char * delim)
		: LsList<int> () {
		for ( char * token = strtok (str, delim);
		      token != NULL; token = strtok(NULL, delim) ) {
			push_back ( atoi(token) );
		}
	}
};

static class rtProtoLSclass : public TclClass {
public:
	rtProtoLSclass() : TclClass("Agent/rtProto/LS") {}
	TclObject* create(int, const char*const*) {
		return (new rtProtoLS);
	}
} class_rtProtoLS;


int rtProtoLS::command(int argc, const char*const* argv)
{
        if (strcmp(argv[1], "send-update") == 0) {
                ns_addr_t dst;
		dst.addr_ = atoi(argv[2]);
		dst.port_ = atoi(argv[3]);
                u_int32_t mtvar = atoi(argv[4]);
		u_int32_t size  = atoi(argv[5]);
                sendpkt(dst, mtvar, size);
                return TCL_OK;
       }
	/* --------------- LS specific --------------- */
	if (strcmp(argv[1], "lookup") == 0) {
		if (argc == 3) {
			int dst = atoi(argv[2]);
			lookup (dst);
			/* use tcl.resultf () to return the lookup results */
			return TCL_OK;
		}
	}  
	if (strcmp(argv[1], "initialize") == 0) {
		initialize ();
		return TCL_OK;
	}
	if (strcmp(argv[1], "setDelay" ) == 0 ) {
		if ( argc == 4) {
			int nbrId = atoi(argv[2]);
			double delay = strtod ( argv[3], NULL);
			setDelay(nbrId, delay);
			return TCL_OK;
		}
	}
	if (strcmp(argv[1], "setNodeNumber" ) == 0 ) {
		if ( argc == 3 ) {
			int number_of_nodes = atoi(argv[2]);
			LsMessageCenter::instance().setNodeNumber(number_of_nodes);
		}
		return TCL_OK;
	}
	if (strcmp(argv[1], "computeRoutes") == 0) {
		computeRoutes();
		return TCL_OK;
	}
	if (strcmp(argv[1], "intfChanged") == 0) {
		intfChanged();
		return TCL_OK;
	}
	if (strcmp (argv[1], "send-buffered-messages") == 0) {
		sendBufferedMessages();
		return TCL_OK;
	}
	if (strcmp(argv[1], "sendUpdates") == 0) {
		sendUpdates ();
		return TCL_OK;
	}
	return Agent::command(argc, argv);
}

void rtProtoLS::sendpkt(ns_addr_t dst, u_int32_t mtvar, u_int32_t size)
{
	dst_ = dst;
	size_ = size;
	
        Packet* p = Agent::allocpkt();
	hdr_LS *rh = hdr_LS::access(p);
        rh->metricsVar() = mtvar;

        target_->recv(p);               
}

void rtProtoLS::recv(Packet* p, Handler*)
{   
	hdr_LS* rh = hdr_LS::access(p);
	hdr_ip* ih = hdr_ip::access(p);
	// -- LS stuffs --
	if (LS_ready_ || (rh->metricsVar() == LS_BIG_NUMBER))
		receiveMessage(findPeerNodeId(ih->src()), rh->msgId());
	else
		Tcl::instance().evalf("%s recv-update %d %d", name(),
				      ih->saddr(), rh->metricsVar());
	Packet::free(p);
}

/* LS specific */
// implement tcl cmd's

/* -- findPeerNodeId -- */
int rtProtoLS::findPeerNodeId (ns_addr_t agentAddr) 
{
	// because the agentAddr is the value, not the key of the map
	for (PeerAddrMap::iterator itr = peerAddrMap_.begin();
	     itr != peerAddrMap_.end(); itr++) {
		if ((*itr).second == agentAddr) {
			return (*itr).first;
		}
	}
	return LS_INVALID_NODE_ID;
}

void rtProtoLS::initialize() // init nodeState_ and routing_
{
	Tcl & tcl = Tcl::instance();
	// call tcl get-node-id, atoi, set nodeId
	tcl.evalf("%s get-node-id", name());
	char * resultString = tcl.result();
	nodeId_ = atoi(resultString);
  
	// call tcl get-peers, strtok, set peerAddrMap, peerIdList;
	tcl.evalf("%s get-peers", name());
	resultString = tcl.result();

	int nodeId, neighborId;
	ns_addr_t peer;
	ls_status_t status;
	int cost;

	// XXX no error checking for now yet. tcl MUST return pairs of numbers
	for ( LsIntList intList(resultString, " \t\n");
	      !intList.empty(); ) {
		nodeId = intList.front();
		intList.pop_front();
		// Agent.addr_
		peer.addr_ = intList.front();
		intList.pop_front();
		peer.port_ = intList.front();
		intList.pop_front();
		peerAddrMap_.insert(nodeId, peer);
		peerIdList_.push_back(nodeId);
	}
	
	// call tcl get-links-status, strtok, set linkStateList;
	tcl.evalf("%s get-links-status", name());
	resultString = tcl.result();
	// cout << "get-links-status:\t" << resultString <<endl; 
	// XXX no error checking for now. tcl MUST return triplets of numbers
	for ( LsIntList intList2(resultString, " \t\n");
	      !intList2.empty(); ) {
		neighborId = intList2.front();
		intList2.pop_front();
		status = (ls_status_t) intList2.front();
		intList2.pop_front();
		cost = (int) intList2.front();
		intList2.pop_front();
		linkStateList_.push_back(LsLinkState(neighborId,status,cost));
	}    

	// call tcl get-delay-estimates
	tcl.evalf ("%s get-delay-estimates", name());

	// call routing.init(this); and computeRoutes
	routing_.init(this);
	routing_.computeRoutes();
	// debug
	tcl.evalf("%s set LS_ready", name());
	char* token = strtok(tcl.result(), " \t\n");
	if (token == NULL) 
		LS_ready_ = 0;
	else
		LS_ready_ = atoi(token); // buggy
}

void rtProtoLS::intfChanged ()
{
	// XXX redudant, to be changed later
	Tcl & tcl = Tcl::instance();
	// call tcl get-links-status, strtok, set linkStateList;
	tcl.evalf("%s get-links-status", name());
	char * resultString = tcl.result();

	// destroy the old link states
	linkStateList_.eraseAll();
	// XXX no error checking for now. tcl MUST return triplets of numbers
  
	for (LsIntList intList2(resultString, " \t\n");
	     !intList2.empty(); ) {
		int neighborId = intList2.front();
		intList2.pop_front();
		ls_status_t  status = ( ls_status_t ) intList2.front();
		intList2.pop_front();
		int cost = (int) intList2.front();
		intList2.pop_front();
		// LsLinkState ls;
		// ls.init(neighborId, status, cost);
		linkStateList_.push_back(LsLinkState(neighborId,status,cost));
	}    
	// call routing_'s LinkStateChanged() 
	//   for now, don't compute routes yet (?)
	routing_.linkStateChanged();
}

void rtProtoLS::lookup(int destId) 
{
	// Call routing_'s lookup
	LsEqualPaths* EPptr = routing_.lookup(destId);

	// then use tcl.resultf() to return the results
	if (EPptr == NULL) {
		Tcl::instance().resultf( "%s",  "");
		return;
	}
	char resultBuf[64]; // XXX buggy;
	sprintf(resultBuf, "%d" , EPptr->cost);
	char tmpBuf[16]; // XXX
 
	for (LsNodeIdList::iterator itr = (EPptr->nextHopList).begin();
	     itr != (EPptr->nextHopList).end(); itr++) {
		sprintf(tmpBuf, " %d", (*itr) );
		strcat (resultBuf, tmpBuf); // strcat (dest, src);
	}

	Tcl::instance().resultf("%s", resultBuf);
}

void rtProtoLS::receiveMessage(int sender, u_int32_t msgId) 
{ 
	if (routing_.receiveMessage(sender, msgId))
		installRoutes();
}

// Implementing LsNode interface
bool rtProtoLS::sendMessage(int destId, u_int32_t messageId, int size) 
{
	ns_addr_t* agentAddrPtr = peerAddrMap_.findPtr(destId);
	if (agentAddrPtr == NULL) 
		return false;
	dst_ = *agentAddrPtr;
	size_ = size;
  
	Packet* p = Agent::allocpkt();
	hdr_LS *rh = hdr_LS::access(p);
	rh->msgId() = messageId;
	rh->metricsVar() = LS_BIG_NUMBER;
	target_->recv(p);           
	// sendpkt( *agentAddrPtr , messageId, size);
	return true;
}

#endif // HAVE_STL
