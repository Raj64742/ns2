/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 1999 by the University of Southern California
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
// Incorporating Polly's web traffic module into the PagePool framework
//
// XXX This has nothing to do with the HttpApp classes. Because we are 
// only interested in traffic pattern here, we do not want to be bothered 
// with the burden of transmitting HTTP headers, etc. 
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/webtraf.h,v 1.14 2002/07/12 18:29:07 xuanc Exp $

#ifndef ns_webtraf_h
#define ns_webtraf_h

#include "ranvar.h"
#include "random.h"
#include "timer-handler.h"

#include "lib/bsd-list.h"
#include "node.h"
#include "tcp.h"
#include "tcp-sink.h"
#include "pagepool.h"

const int WEBTRAF_DEFAULT_OBJ_PER_PAGE = 1;

class WebTrafPool;

class WebTrafSession : public TimerHandler {
public: 
	WebTrafSession(WebTrafPool *mgr, Node *src, int np, int id, int ftcp_, int recycle_p);
	virtual ~WebTrafSession();

	// Queried by individual pages/objects
	inline RandomVariable*& interPage() { return rvInterPage_; }
	inline RandomVariable*& pageSize() { return rvPageSize_; }
	inline RandomVariable*& interObj() { return rvInterObj_; }
	inline RandomVariable*& objSize() { return rvObjSize_; }

	void donePage(void* ClntData);
	void launchReq(void* ClntData, int obj, int size);
	inline int id() const { return id_; }
	inline WebTrafPool* mgr() { return mgr_; }
	inline void set_interPageOption(int option) { interPageOption_ = option; }

	static int LASTPAGE_;

private:
	virtual void expire(Event *e = 0);
	virtual void handle(Event *e);

	RandomVariable *rvInterPage_, *rvPageSize_, *rvInterObj_, *rvObjSize_;
	WebTrafPool* mgr_;
	Node* src_;		// One Web client (source of request) per session
	int nPage_, curPage_, donePage_;
	int id_, interPageOption_;
	
	// fulltcp support
	int fulltcp_;
	// Reuse of page level attributes support
	int recycle_page_;
};

struct job_s {
	Agent *tcp;
	int size;
	job_s *next;
};

// Data structure for web server
class WebServer : public TimerHandler{
public:
	WebServer(WebTrafPool *);
	
	// Server properties
	Node *node;
	double rate_;
	int busy_;
	double mode_;
	WebTrafPool * web_pool_;

	// Job queue
	job_s *head, *tail;

	virtual void expire(Event *e);

	// Job arrival and departure
	double job_arrival(Agent*, int);
	double job_departure();

	void schedule_next_job();
};

class WebTrafPool : public PagePool {
public: 
	WebTrafPool(); 
	virtual ~WebTrafPool(); 

	inline Node* picksrc() {
		int n = int(floor(Random::uniform(0, nClient_)));
		assert((n >= 0) && (n < nClient_));
		return client_[n];
	}
	inline Node* pickdst() {
		int n = int(floor(Random::uniform(0, nServer_)));
		assert((n >= 0) && (n < nServer_));
		return server_[n].node;
	}
	inline void doneSession(int idx) { 
		assert((idx>=0) && (idx<nSession_) && (session_[idx]!=NULL));
		if (isdebug())
			printf("deleted session %d\n", idx);
		delete session_[idx];
		session_[idx] = NULL; 
	}
	TcpAgent* picktcp();
	TcpSink* picksink();
	inline int nTcp() { return nTcp_; }
	inline int nSink() { return nSink_; }
	inline int isdebug() { return debug_; }

	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char*, const char*, TclObject*);

protected:
	virtual int command(int argc, const char*const* argv);

	// Session management: fixed number of sessions, fixed number
	// of pages per session
	int nSession_;
	WebTrafSession** session_; 

	// Web servers
	int nServer_;
	//Node** server_;
	// keep the finish time of the last job (M/G/1)
	WebServer *server_;

	// Browsers
	int nClient_;
	Node** client_;

	// Debo added this
	int asimflag_;

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
	inline int lookup_rv(RandomVariable*& rv, const char* name) {
		if (rv != NULL)
			Tcl::instance().evalf("delete %s", rv->name());
		rv = (RandomVariable*)lookup_obj(name);
		return rv ? (TCL_OK) : (TCL_ERROR);
	}
	int debug_;
	// fulltcp support
	int fulltcp_;
	// Reuse of page level attributes support
	int recycle_page_;
	// support simple server delay
	int server_delay_;
};

#endif // ns_webtraf_h


