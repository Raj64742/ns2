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
// Empirical Web traffic model that simulates Web traffic based on a set of
// CDF (Cumulative Distribution Function) data derived from live tcpdump trace
// The structure of this file is largely borrowed from webtraf.h
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/empweb/empweb.h,v 1.12 2001/12/19 18:57:51 kclan Exp $

#ifndef ns_empweb_h
#define ns_empweb_h

#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "persconn.h"

const int WEBTRAF_DEFAULT_OBJ_PER_PAGE = 1;

class EmpWebTrafPool;

class EmpWebTrafSession : public TimerHandler {
public: 
	EmpWebTrafSession(EmpWebTrafPool *mgr, Node *src, int np, int id, int connNum, int cl) : 
		rvInterPage_(NULL), rvPageSize_(NULL),
		rvInterObj_(NULL), rvObjSize_(NULL), 
		rvReqSize_(NULL), rvPersistSel_(NULL), rvServerSel_(NULL),
		rvServerWin_(NULL), rvClientWin_(NULL),
//		numOfPersConn_(0), usePers_(0), 
//		maxNumOfPersConn_(connNum), clientIdx_(cl),
		clientIdx_(cl), 
		mgr_(mgr), src_(src), nPage_(np), curPage_(0), donePage_(0),
		id_(id) {}
	virtual ~EmpWebTrafSession();

	// Queried by individual pages/objects
	inline EmpiricalRandomVariable*& interPage() { return rvInterPage_; }
//	inline EmpiricalRandomVariable*& pageSize() { return rvPageSize_; }
inline RandomVariable*& pageSize() { return rvPageSize_; }
	inline EmpiricalRandomVariable*& interObj() { return rvInterObj_; }
	inline EmpiricalRandomVariable*& objSize() { return rvObjSize_; }

	inline EmpiricalRandomVariable*& reqSize() { return rvReqSize_; }
	inline EmpiricalRandomVariable*& persistSel() { return rvPersistSel_; }
	inline EmpiricalRandomVariable*& serverSel() { return rvServerSel_; }

	inline EmpiricalRandomVariable*& serverWin() { return rvServerWin_; }
	inline EmpiricalRandomVariable*& clientWin() { return rvClientWin_; }

	void donePage(void* ClntData);
	void launchReq(void* ClntData, int obj, int size, int reqSize, int sid);
	inline int id() const { return id_; }
	inline EmpWebTrafPool* mgr() { return mgr_; }

//        PersConn* lookupPersConn(int client, int server);

	static int LASTPAGE_;
//	inline void setPersOpt(int opt) { usePers_ = opt; }
//	inline int getPersOpt() { return usePers_; }
//	inline void initPersConn() { 
//	       if (getPersOpt() == PERSIST) {
//	           persistConn_ = new PersConn*[maxNumOfPersConn_];  
//	           memset(persistConn_, 0, sizeof(PersConn*)*maxNumOfPersConn_);
//               }
//         }

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

//	EmpiricalRandomVariable *rvInterPage_, *rvPageSize_, *rvInterObj_, *rvObjSize_;
	EmpiricalRandomVariable *rvInterPage_, *rvInterObj_, *rvObjSize_;
	RandomVariable *rvPageSize_;
	EmpiricalRandomVariable *rvReqSize_, *rvPersistSel_, *rvServerSel_;
	EmpiricalRandomVariable *rvServerWin_, *rvClientWin_;
	EmpWebTrafPool* mgr_;
	Node* src_;		// One Web client (source of request) per session
	int nPage_, curPage_, donePage_;
	int id_;

        int clientIdx_;

        //modeling HTTP1.1
//	PersConn** persistConn_; 
//	int numOfPersConn_ ;
//	int maxNumOfPersConn_ ;
//	int usePers_ ;  //0: http1.0  1: http1.1 ; use http1.0 as default
};

class EmpWebTrafPool : public PagePool {
public: 
	EmpWebTrafPool(); 
	virtual ~EmpWebTrafPool(); 

	inline void startSession() {
	       	concurrentSess_++;
		if (isdebug()) 
			printf("concurrent number of sessions = %d \n", concurrentSess_ );
	}
//	inline Node* picksrc() {
//		int n = int(floor(Random::uniform(0, nClient_)));
//		assert((n >= 0) && (n < nClient_));
//		return client_[n];
//	}
	inline void doneSession(int idx) { 

		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (concurrentSess_ > 0) concurrentSess_--;
		if (isdebug()) {
			printf("deleted session %d \n", idx );
			printf("concurrent number of sessions = %d \n", concurrentSess_ );
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

	int concurrentSess_;

	int nSrc_;
	Node** server_;		/* Web servers */

protected:
	virtual int command(int argc, const char*const* argv);

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	EmpWebTrafSession** session_; 

	int nClient_;
	Node** client_; 	/* Browsers */

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

        inline int lookup_rv1(RandomVariable*& rv, const char* name) {
                if (rv != NULL)
                        Tcl::instance().evalf("delete %s", rv->name());
                rv = (RandomVariable*)lookup_obj(name);
                return rv ? (TCL_OK) : (TCL_ERROR);
        }

	int debug_;
};


#endif // ns_empweb_h
