// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-08-31 17:42:19 haoboy>
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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mpls/ldp.h,v 1.2 2000/09/01 03:04:11 haoboy Exp $

/**************************************************************************
* Copyright (c) 2000 by Gaeil Ahn                                   	  *
* Everyone is permitted to copy and distribute this software.		  *
* Please send mail to fog1@ce.cnu.ac.kr when you modify or distribute     *
* this sources.								  *
**************************************************************************/

/***********************************************************
*                                                          *
*    File: Header File for LDP & CR-LDP protocol           *
*    Author: Gaeil Ahn (fog1@ce.cnu.ac.kr), Jan. 2000      *
*                                                          *
***********************************************************/

#ifndef ns_ldp_h
#define ns_ldp_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"

const int LDP_MaxMSGTEntryNB = 100;

/* LDP msg types */
const int LDP_NotificationMSG = 0x0001;
const int LDP_MappingMSG =      0x0400;
const int LDP_RequestMSG =      0x0401;
const int LDP_WithdrawMSG =     0x0402;
const int LDP_ReleaseMSG =      0x0403;

const int LDP_LoopDetected =    0x000B;
const int LDP_NoRoute =         0x000D;

const int LDP_LabelALLOC =      0;
const int LDP_LabelPASS =       1;
const int LDP_LabelSTACK =      2;

struct hdr_ldp {
	int  msgtype;
	int  msgid;  	// request msg id (mapping msg triggered by request 
			// msg). if (msgid > -1): by request, else:  no 
	int  fec;
	int  label;
	
	int  reqmsgid;
	int  status;      // for Notification

	// XXX This is VERY BAD behavior! Should NEVER put a pointer
	// in a packet header!!

	char *pathvec; 
	// The following is for CR-LSP
	char *er;

	int  lspid;
	int  rc;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_ldp* access(const Packet* p) {
		return (hdr_ldp*)p->access(offset_);
	}
};

// Used for supporting ordered distribuiton mode (Explicit LSP)
struct MsgTable {
	int  MsgID;
	int  FEC;
	int  LspID;
	int  Src;
	int  PMsgID;	// previsou msg-id
	int  LabelOp;   // LabelALLOC, LabelPASS, or LabelSTACK
	int  ERLspID;
};

struct MsgT {
	MsgTable Entry[LDP_MaxMSGTEntryNB];
	int      NB;
};


class LDPAgent : public Agent {
public:
	LDPAgent();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);
  
	void PKTinit(hdr_ldp *hdrldp, int msgtype, const char *pathvec, 
		     const char *er);
	int  PKTsize(const char *pathvec, const char *er);
	
	int  MSGTinsert(int MsgID, int FEC, int LspID, int Src, int PMsgID);
	void MSGTdelete(int entrynb);
	int  MSGTlocate(int MsgID);
	int  MSGTlocate(int FEC,int LspID,int Src);
	void MSGTlookup(int entrynb, int &MsgID, int &FEC, int &LspID, 
			int &src, int &PMsgID, int &LabelOp);
	void MSGTdump();
   
protected:
	int    new_msgid_;
	int    trace_ldp_;
	void   trace(ns_addr_t src, hdr_ldp *hdrldp);
  
	char* parse_msgtype(int msgtype, int lspid);
	char* parse_status(int status);
  
	MsgT  MSGT_;
};

#endif
