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
// Incorporation Polly's web traffic module into the PagePool framework
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/empweb/empweb.cc,v 1.5 2001/07/17 02:25:09 kclan Exp $

#include <tclcl.h>

#include "empweb.h"


// Data structures that are specific to this web traffic model and 
// should not be used outside this file.
//
// - EmpWebTrafPage
// - EmpWebTrafObject

class EmpWebPage : public TimerHandler {
public:
	EmpWebPage(int id, EmpWebTrafSession* sess, int nObj, Node* dst, int svrId) :
		id_(id), sess_(sess), nObj_(nObj), curObj_(0), dst_(dst), svrId_(svrId) {}
	virtual ~EmpWebPage() {}

	inline void start() {
		// Call expire() and schedule the next one if needed
		status_ = TIMER_PENDING;
		handle(&event_);
	}
	inline int id() const { return id_; }
	inline int svrId() const { return svrId_; }
	Node* dst() { return dst_; }

private:
	virtual void expire(Event* = 0) {
		// Launch a request. Make sure size is not 0!
		if (curObj_ >= nObj_) 
			return;
		sess_->launchReq(this, LASTOBJ_++, 
				 (int)ceil(sess_->objSize()->value()),
				 (int)ceil(sess_->reqSize()->value()), sess_->id());
		if (sess_->mgr()->isdebug())
			printf("Session %d launched page %d obj %d\n",
			       sess_->id(), id_, curObj_);
	}
	virtual void handle(Event *e) {
		// XXX Note when curObj_ == nObj_, we still schedule the timer
		// once, but we do not actually send out requests. This extra
		// schedule is only meant to be a hint to wait for the last
		// request to finish, then we will ask our parent to delete
		// this page.
		if (curObj_ <= nObj_) {
			// If this is not the last object, schedule the next 
			// one. Otherwise stop and tell session to delete me.
			TimerHandler::handle(e);
			curObj_++;
			sched(sess_->interObj()->value());
		} else
			sess_->donePage((void*)this);
	}
	int id_;
	EmpWebTrafSession* sess_;
	int nObj_, curObj_;
	Node* dst_;
	int svrId_ ;
	static int LASTOBJ_;
};

int EmpWebPage::LASTOBJ_ = 1;


int EmpWebTrafSession::LASTPAGE_ = 1;

// XXX Must delete this after all pages are done!!
EmpWebTrafSession::~EmpWebTrafSession() 
{

	if (donePage_ != curPage_) {
		fprintf(stderr, "done pages %d != all pages %d\n",
			donePage_, curPage_);
		abort();
	}
	if (status_ != TIMER_IDLE) {
		fprintf(stderr, "EmpWebTrafSession must be idle when deleted.\n");
		abort();
	}
	
	if (rvInterPage_ != NULL)
		Tcl::instance().evalf("delete %s", rvInterPage_->name());
	if (rvPageSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvPageSize_->name());
	if (rvInterObj_ != NULL)
		Tcl::instance().evalf("delete %s", rvInterObj_->name());
	if (rvObjSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvObjSize_->name());
	if (rvReqSize_ != NULL)
		Tcl::instance().evalf("delete %s", rvReqSize_->name());
	if (rvPersistSel_ != NULL)
		Tcl::instance().evalf("delete %s", rvPersistSel_->name());
	if (rvServerSel_ != NULL)
		Tcl::instance().evalf("delete %s", rvServerSel_->name());


        if ((persistConn_ != NULL) && (getPersOpt() == PERSIST)) {
            for (int i = 0; i < maxNumOfPersConn_; i++) {
	      if (persistConn_[i] != NULL) delete persistConn_[i];
            }
	    delete []persistConn_;
        }

}

void EmpWebTrafSession::donePage(void* ClntData) 
{
	if (mgr_->isdebug()) 
		printf("Session %d done page %d\n", id_, 
		       ((EmpWebPage*)ClntData)->id());

	delete (EmpWebPage*)ClntData;
	// If all pages are done, tell my parent to delete myself
	if (++donePage_ >= nPage_) {
	
            for (int i = 0; i < numOfPersConn_; i++) {
	      if (persistConn_[i] != NULL) 
	         // teardown all the persistent connections 
	         Tcl::instance().evalf("%s teardown-conn %s %s %s %s %s %s", 
			      mgr_->name(),  
			      persistConn_[i]->getCNode()->name(), 
			      persistConn_[i]->getSNode()->name(),
			      persistConn_[i]->getCTcpAgent()->name(), 
			      persistConn_[i]->getCTcpSink()->name(), 
			      persistConn_[i]->getSTcpAgent()->name(),
			      persistConn_[i]->getSTcpSink()->name());
            }
	    
	    mgr_->doneSession(id_);
        }
}

// Launch the current page
void EmpWebTrafSession::expire(Event *)
{
	// Pick destination for this page
        int n = int(ceil(serverSel()->value()));
        assert((n >= 0) && (n < mgr()->nSrc_));
        Node* dst = mgr()->server_[n];

	// Make sure page size is not 0!
	EmpWebPage* pg = new EmpWebPage(LASTPAGE_++, this, 
				  (int)ceil(rvPageSize_->value()), dst, n);
	if (mgr_->isdebug())
		printf("Session %d starting page %d, curpage %d \n", 
		       id_, LASTPAGE_-1, curPage_);
	pg->start();
}

void EmpWebTrafSession::handle(Event *e)
{
	// If I haven't scheduled all my pages, do the next one
	TimerHandler::handle(e);
	// XXX Notice before each page is done, it will schedule itself 
	// one more time, this makes sure that this session will not be
	// deleted after the above call. Thus the following code will not
	// be executed in the context of a deleted object. 
	if (++curPage_ < nPage_)
		sched(rvInterPage_->value());
}

// Launch a request for a particular object
void EmpWebTrafSession::launchReq(void* ClntData, int obj, int size, int reqSize, int sid)
{

	EmpWebPage* pg = (EmpWebPage*)ClntData;

        if (getPersOpt() == PERSIST) { //for HTTP1.1 persistent-connection

           PersConn* p = lookupPersConn(src_->nodeid(), pg->dst()->nodeid());

	   if (p == NULL)  {


	      // Choose source and dest TCP agents for both source and destination
	      TcpAgent* ctcp = mgr_->picktcp();
	      TcpAgent* stcp = mgr_->picktcp();
	      TcpSink* csnk = mgr_->picksink();
	      TcpSink* ssnk = mgr_->picksink();

              // store the state of this connection
	      p = new PersConn();
	      p->setDst(pg->dst()->nodeid());
	      p->setSrc(src_->nodeid());
	      p->setCTcpAgent(ctcp);
	      p->setSTcpAgent(stcp);
	      p->setCTcpSink(csnk);
	      p->setSTcpSink(ssnk);
	      p->setCNode(src_);
	      p->setSNode(pg->dst());
	      p->setStatus(INUSE);
	      persistConn_[numOfPersConn_] = p;
	      numOfPersConn_++;

	      // Setup new TCP connection and launch request
	      Tcl::instance().evalf("%s first-launch-reqP %d %s %s %s %s %s %s %d %d %d", 
			      mgr_->name(), obj, src_->name(), 
			      pg->dst()->name(),
			      ctcp->name(), csnk->name(), stcp->name(),
			      ssnk->name(), size, reqSize, sid);

           } else {

             if (p->getStatus() == IDLE) {
	        p->setStatus(INUSE);
	        // use existing persistent connection to launch request
	        Tcl::instance().evalf("%s launch-reqP %d %s %s %s %s %s %s %d %d %d", 
			      mgr_->name(), obj, p->getCNode()->name(), 
			      p->getSNode()->name(),
			      p->getCTcpAgent()->name(), 
			      p->getCTcpSink()->name(), 
			      p->getSTcpAgent()->name(),
			      p->getSTcpSink()->name(), size, reqSize, sid);
             }
             else {
                p->pendingReqByte_ = p->pendingReqByte_ + reqSize;
                p->pendingReplyByte_ = p->pendingReplyByte_ + size;
             }
	   }
        } else { //for HTTP1.0 non-consistent connection


	  // Choose source and dest TCP agents for both source and destination
	  TcpAgent* ctcp = mgr_->picktcp();
	  TcpAgent* stcp = mgr_->picktcp();
	  TcpSink* csnk = mgr_->picksink();
	  TcpSink* ssnk = mgr_->picksink();

	  // Setup new TCP connection and launch request
	  Tcl::instance().evalf("%s launch-req %d %s %s %s %s %s %s %d %d", 
			      mgr_->name(), obj, src_->name(), 
			      pg->dst()->name(),
			      ctcp->name(), csnk->name(), stcp->name(),
			      ssnk->name(), size, reqSize);

	}

	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst

if (mgr_->isdebug()) {
	printf("%d \t %d \t %d \t %d \t %g %d %d\n", size, obj, pg->id(), id_,
	       Scheduler::instance().clock(), 
	       src_->address(), pg->dst()->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", mgr_->nTcp(),mgr_->nSink());
}
}

// Lookup for a particular persistent connection
PersConn* EmpWebTrafSession::lookupPersConn(int client, int server)
{
    for (int i = 0; i < numOfPersConn_; i++) {
       if ((persistConn_[i]->getSrc() == client) &&
           (persistConn_[i]->getDst() == server))
	   return persistConn_[i];
    }
    return NULL;
}

static class EmpWebTrafPoolClass : public TclClass {
public:
        EmpWebTrafPoolClass() : TclClass("PagePool/EmpWebTraf") {}
        TclObject* create(int, const char*const*) {
		return (new EmpWebTrafPool());
	}
} class_empwebtrafpool;

EmpWebTrafPool::~EmpWebTrafPool()
{
	if (session_ != NULL) {
		for (int i = 0; i < nSession_; i++)
			delete session_[i];
		delete []session_;
	}
	if (server_ != NULL)
		delete []server_;
	if (client_ != NULL)
		delete []client_;
	// XXX Destroy tcpPool_ and sinkPool_ ?
}

void EmpWebTrafPool::delay_bind_init_all()
{
	delay_bind_init_one("debug_");
	PagePool::delay_bind_init_all();
}

int EmpWebTrafPool::delay_bind_dispatch(const char *varName,const char *localName,
				     TclObject *tracer)
{
	if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) 
		return TCL_OK;
	return PagePool::delay_bind_dispatch(varName, localName, tracer);
}

// By default we use constant request interval and page size
EmpWebTrafPool::EmpWebTrafPool() : 
	session_(NULL), nSrc_(0), server_(NULL), nClient_(0), client_(NULL),
	nTcp_(0), nSink_(0)
{
	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
}

TcpAgent* EmpWebTrafPool::picktcp()
{

	TcpAgent* a = (TcpAgent*)detachHead(&tcpPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp", name());
		a = (TcpAgent*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP agent\n");
			abort();
		}
	} else 
		nTcp_--;
	return a;
}

TcpSink* EmpWebTrafPool::picksink()
{
	TcpSink* a = (TcpSink*)detachHead(&sinkPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp-sink", name());
		a = (TcpSink*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP sink\n");
			abort();
		}
	} else 
		nSink_--;
	return a;
}

int EmpWebTrafPool::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "set-num-session") == 0) {
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) 
					delete session_[i];
				delete []session_;
			}
			nSession_ = atoi(argv[2]);
			session_ = new EmpWebTrafSession*[nSession_];
			memset(session_, 0, sizeof(EmpWebTrafSession*)*nSession_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-server") == 0) {
			nSrc_ = atoi(argv[2]);
			if (server_ != NULL) 
				delete []server_;
			server_ = new Node*[nSrc_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-client") == 0) {
			nClient_ = atoi(argv[2]);
			if (client_ != NULL) 
				delete []client_;
			client_ = new Node*[nClient_];
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "set-server") == 0) {
			Node* cli = (Node*)lookup_obj(argv[3]);
			if (cli == NULL)
				return (TCL_ERROR);
			int nc = atoi(argv[2]);
			if (nc >= nSrc_) {
				fprintf(stderr, "Wrong server index %d\n", nc);
				return TCL_ERROR;
			}
			server_[nc] = cli;
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-client") == 0) {
			Node* s = (Node*)lookup_obj(argv[3]);
			if (s == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nClient_) {
				fprintf(stderr, "Wrong client index %d\n", n);
				return TCL_ERROR;
			}
			client_[n] = s;
			return (TCL_OK);
		} else if (strcmp(argv[1], "recycle") == 0) {
			// <obj> recycle <tcp> <sink>
			//
			// Recycle a TCP source/sink pair
			Agent* tcp = (Agent*)lookup_obj(argv[2]);
			Agent* snk = (Agent*)lookup_obj(argv[3]);
			nTcp_++, nSink_++;
			if ((tcp == NULL) || (snk == NULL))
				return (TCL_ERROR);
			// XXX TBA: recycle tcp agents
			insertAgent(&tcpPool_, tcp);
			insertAgent(&sinkPool_, snk);
			return (TCL_OK);
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "send-pending") == 0) {
		   int id = atoi(argv[2]);
		   int client = atoi(argv[3]);
		   int server = atoi(argv[4]);
		   int sid = atoi(argv[5]);
                   PersConn* p = session_[sid]->lookupPersConn(client, server);
                   if (p != NULL) {
	              p->setStatus(IDLE);

                      //send out the pending request
                      if (( p->pendingReqByte_ > 0) &&
                          ( p->getStatus() != INUSE)) {
	                   p->setStatus(INUSE);

	                   Tcl::instance().evalf("%s launch-reqP %d %s %s %s %s %s %s %d %d %d", 
			      this->name(), id, p->getCNode()->name(), 
			      p->getSNode()->name(),
			      p->getCTcpAgent()->name(), 
			      p->getCTcpSink()->name(), 
			      p->getSTcpAgent()->name(),
			      p->getSTcpSink()->name(),
                              p->pendingReplyByte_, p->pendingReqByte_, sid);
                           p->pendingReqByte_ = 0;
                           p->pendingReplyByte_ = 0;
                      }
		      return (TCL_OK);
                   } else {
		     return (TCL_ERROR);
                   }
                }
	} else if (argc == 12) {
		if (strcmp(argv[1], "create-session") == 0) {
			// <obj> create-session <session_index>
			//   <pages_per_sess> <launch_time>
			//   <inter_page_rv> <page_size_rv>
			//   <inter_obj_rv> <obj_size_rv>
			//   <req_size_rv> <persist_sel_rv> <server_sel_rv>
			int n = atoi(argv[2]);
			if ((n < 0)||(n >= nSession_)||(session_[n] != NULL)) {
				fprintf(stderr,"Invalid session index %d\n",n);
				return (TCL_ERROR);
			}
			int npg = (int)strtod(argv[3], NULL);
			double lt = strtod(argv[4], NULL);

			EmpWebTrafSession* p = 
				new EmpWebTrafSession(this, picksrc(), npg, n, nSrc_);
			int res = lookup_rv(p->interPage(), argv[5]);
			res = (res == TCL_OK) ? 
				lookup_rv1(p->pageSize(), argv[6]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->interObj(), argv[7]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->objSize(), argv[8]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->reqSize(), argv[9]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->persistSel(), argv[10]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->serverSel(), argv[11]) : TCL_ERROR;
			if (res == TCL_ERROR) {
				delete p;
				fprintf(stderr, "Invalid random variable\n");
				return (TCL_ERROR);
			}
			p->sched(lt);
			session_[n] = p;
		       
		        // decide to use either persistent or non-persistent
			int opt = (int)ceil(p->persistSel()->value());
			opt = 0;
			p->setPersOpt(opt);
                        p->initPersConn();
                           
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}


