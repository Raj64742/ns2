// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-11 10:04:09 haoboy>
//
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
// Original source contributed by Gaeil Ahn. See below.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mpls/classifier-addr-mpls.h,v 1.4 2001/02/22 19:45:40 haldar Exp $

/**************************************************************************
* Copyright (c) 2000 by Gaeil Ahn                                   	  *
* Everyone is permitted to copy and distribute this software.		  *
* Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     *
* this sources.								  *
**************************************************************************/

/***********************************************************
*                                                          *
*    File: Header File for packet switching in MPLS node   *
*    Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Dec. 1999      *
*                                                          *
***********************************************************/

#ifndef ns_classifier_addr_mpls_h
#define ns_classifier_addr_mpls_h

#include "address.h"
#include "classifier-addr.h"
#include "tclcl.h"

const int MPLS_MaxPFTEntryNB = 100;
const int MPLS_MaxLIBEntryNB = 100;
const int MPLS_MaxERBEntryNB = 100;
const int MPLS_MINIMUM_LSPID = 1000;

const int MPLS_DEFAULT_PHB = -1;
const int MPLS_DONTCARE = -77;
const int MPLS_GOTO_L3 = -99;

/* option for reroute scheme */
const int MPLS_DROPPACKET = 0;
const int MPLS_L3FORWARDING = 1;
const int MPLS_MAKENEWLSP = 2;

struct hdr_mpls {
	int  label_;
	int  bflag_;
	int  ttl_;
	hdr_mpls* nexthdr_;
	hdr_mpls* top_;
  
	int& label() { return label_; }
	int& bflag() { return bflag_; }
	int& ttl()   { return ttl_;   }

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_mpls* access(const Packet* p) {
		return (hdr_mpls*)p->access(offset_);
	}
};

struct PFTEntry {
	int  FEC_;
	int  PHB_;
	int  LIBptr_;
	int  aPATHptr_;     // altanative path 
};

struct PFT {
	PFTEntry Entry_[MPLS_MaxPFTEntryNB];
	int NB_;
};

struct ERBEntry {
	int LSPid_;
	int FEC_;
	int LIBptr_;
};

struct ERB {
	ERBEntry Entry_[MPLS_MaxERBEntryNB];
	int NB_;
};

struct LIBEntry {
	int iIface_;
	int iLabel_;
	int oIface_;
	int oLabel_;
	int LIBptr_;    // for push operation
};

struct LIB {
	LIBEntry Entry_[MPLS_MaxLIBEntryNB];
	int NB_;
};

struct PktInfo
{
	ns_addr_t dst_;
	int srcnode_;
	int phb_;
	hdr_mpls* shimhdr_;     // altanative path 
};

class MPLSAddressClassifier : public AddressClassifier {
public: 
	MPLSAddressClassifier();
	virtual int command(int argc, const char*const* argv);

	// Static configuration variables. Exposed to OTcl via methods in 
	// MPLSNodeClass (see mpls-node.cc)
	static int ordered_control_;
	static int on_demand_;

		
protected:
	virtual void install(int slot, NsObject *target);
	
	virtual int classify(Packet* p);
	int MPLSclassify(Packet* p);
    
	int processIP();
	int processLabelP();
    
	int convertL2toL2(int iLabel, int oIface, int oLabel, int LIBptr);
	int convertL3toL2(int oIface, int oLabel, int LIBptr);
    
	void GetIPInfo(Packet* p, ns_addr_t& dstaddr, int& phb, int& srcnode);
	hdr_mpls* checkTTL(hdr_mpls* shimhdr);
    
	hdr_mpls* GetShimHeader(Packet* p);
	hdr_mpls* DelAllShimHeader(hdr_mpls* shimhdr);
	hdr_mpls* push(hdr_mpls* shimhdr, int oLabel);
	hdr_mpls* pop(hdr_mpls* shimhdr);
	void swap(hdr_mpls* shimhdr, int oLabel) {
		shimhdr->label_ = oLabel;
	}
    
	void PFTinsert(int FEC, int PHB, int LIBptr);
	void PFTdelete(int entrynb);
	void PFTdeleteLIBptr(int LIBptr);
	void PFTupdate(int entrynb, int LIBptr);
	int PFTlocate(int FEC, int PHB, int &LIBptr);
	int PFTlookup(int FEC, int PHB, int &oIface, 
		      int &oLabel, int &LIBptr);

	void ERBinsert(int LSPid, int FEC, int LIBptr);
	void ERBdelete(int entrynb);
	void ERBupdate(int entrynb, int LIBptr);
	int ERBlocate(int LSPid, int FEC, int &LIBptr);
    
	int LIBinsert(int iIface, int iLabel, int oIface, int oLabel);
	int LIBisdeleted(int entrynb);
	void LIBupdate(int entrynb, int iIface, int iLabel, 
		       int oIface, int oLabel);
	int LIBlookup(int entrynb, int& oIface, int& oLabel, int& LIBptr);
	int LIBlookup(int iIface, int iLabel, 
		      int& oIface, int& oLabel, int& LIBptr);
	int LIBgetIncoming(int entrynb, int& iIface, int& iLabel);
    
	int ErLspBinding(int FEC,int PHB, int erFEC, int LSPid);
	int ErLspStacking(int erFEC0, int erLSPid0, int erFEC, int erLSPid);
	int FlowAggregation(int fineFEC, int finePHB, int coarseFEC,
			    int coarsePHB);

	int aPathBinding(int FEC, int PHB, int erFEC,int LSPid);
	int aPathLookup(int FEC, int PHB, 
			int& oIface, int& oLabel, int& LIBptr);
	int is_link_down(int node);
	int do_reroute(Packet* p);
       
	void PFTdump(const char* id);
	void ERBdump(const char* id);
	void LIBdump(const char* id);
	void trace(char* ptype, int psize, int ilabel, char *op, 
		   int oiface, int olabel, int ttl);
    
	// Hash mapping between neighbors and LDP agents
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

	int size_;
	int ttl_;
	int label_;
	int enable_reroute_;
	int reroute_option_;
    
	int trace_mpls_;

private:
	// Some states
	int data_driven_;
	int control_driven_;

	LIB     LIB_;
	PFT     PFT_;
	ERB     ERB_;

	PktInfo  PI_;
};

#endif
