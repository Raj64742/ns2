/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 2001 by the University of Southern California
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
//
// Empirical FTP traffic model that simulates FTP traffic based on a set of
// CDF (Cumulative Distribution Function) data derived from live tcpdump trace
// The structure of this file is largely borrowed from empweb.h
//

#ifndef ns_empftp_h
#define ns_empftp_h

#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "persconn.h"


class EmpFtpTrafPool;

class EmpFtpTrafSession : public TimerHandler {
public: 
	EmpFtpTrafSession(EmpFtpTrafPool *mgr, int np, int id) : 
		rvInterFile_(NULL), rvFileSize_(NULL),
		rvServerWin_(NULL), rvClientWin_(NULL),
		rvServerSel_(NULL), 
		mgr_(mgr), nFile_(np), curFile_(0), 
		id_(id) {}
	virtual ~EmpFtpTrafSession();

	// Queried by individual pages/objects
	inline EmpiricalRandomVariable*& interFile() { return rvInterFile_; }
	inline EmpiricalRandomVariable*& fileSize() { return rvFileSize_; }
	inline EmpiricalRandomVariable*& serverSel() { return rvServerSel_; }

	inline EmpiricalRandomVariable*& serverWin() { return rvServerWin_; }
	inline EmpiricalRandomVariable*& clientWin() { return rvClientWin_; }

	inline void setServer(Node* s) { src_ = s; }
	inline void setClient(Node* c) { dst_ = c; }

	void sendFile(int obj, int size);
	inline int id() const { return id_; }
	inline EmpFtpTrafPool* mgr() { return mgr_; }

	static int LASTFILE_;

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

	EmpiricalRandomVariable *rvInterFile_, *rvFileSize_, *rvServerSel_;
	EmpiricalRandomVariable *rvServerWin_, *rvClientWin_;

	EmpFtpTrafPool* mgr_;
	Node* src_;		
	Node* dst_;		
	int nFile_, curFile_ ;
	int id_;


};

class EmpFtpTrafPool : public PagePool {
public: 
	EmpFtpTrafPool(); 
	virtual ~EmpFtpTrafPool(); 

	inline void doneSession(int idx) { 

		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (isdebug()) {
			printf("deleted session %d \n", idx );
                }
		delete session_[idx];
		session_[idx] = NULL; 
	}
	TcpAgent* picktcp(int size);
	TcpSink* picksink();
	inline int nTcp() { return nTcp_; }
	inline int nSink() { return nSink_; }
	inline int isdebug() { return debug_; }

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

	int nSrcL_;
	int nClientL_;


	int nSrc_;
	Node** server_;		/* FTP servers */

protected:
	virtual int command(int argc, const char*const* argv);

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	EmpFtpTrafSession** session_; 

	int nClient_;
	Node** client_; 	/* FTP clients */

	// TCP agent pool management
	struct AgentListElem {
		AgentListElem(Agent* a) : agt_(a) {
			link.le_next = NULL;
			link.le_prev = NULL;
		}
		Agent* agt_;
		LIST_ENTRY(AgentListElem) link;
	};
	LIST_HEAD(AgentList, AgentListElem);
	inline void insertAgent(AgentList* l, Agent *a) {
		AgentListElem *e = new AgentListElem(a);
		LIST_INSERT_HEAD(l, e, link);
	}
	inline Agent* detachHead(AgentList* l) {
		AgentListElem *e = l->lh_first;
		if (e == NULL)
			return NULL;
		Agent *a = e->agt_;
		LIST_REMOVE(e, link);
		delete e;
		return a;
	}
	int nTcp_, nSink_;
	AgentList tcpPool_;	/* TCP agent pool */
	AgentList sinkPool_;	/* TCP sink pool */

	// Helper methods
	inline int lookup_rv(EmpiricalRandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf("delete %s", rv->name());
		rv = (EmpiricalRandomVariable*)lookup_obj(name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}

	int debug_;
};


#endif // ns_empftp_h
