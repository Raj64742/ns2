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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/webtraf.cc,v 1.6 2000/02/24 02:17:02 haoboy Exp $

#include "config.h"
#include <tclcl.h>

#include "node.h"
#include "pagepool.h"
#include "webtraf.h"

// Data structures that are specific to this web traffic model and 
// should not be used outside this file.
//
// - WebTrafPage
// - WebTrafObject

class WebPage : public TimerHandler {
public:
	WebPage(int id, WebTrafSession* sess, int nObj, Node* dst) :
		id_(id), sess_(sess), nObj_(nObj), curObj_(0), dst_(dst) {}
	virtual ~WebPage() {}

	inline void start() {
		expire();
	}
	inline int id() const { return id_; }
	Node* dst() { return dst_; }

private:
	virtual void expire(Event* = 0) {
		// Launch a request. Make sure size is not 0!
		sess_->launchReq(this, LASTOBJ_++, 
				 (int)ceil(sess_->objSize()->value()));
	}
	virtual void handle(Event *e) {
		// XXX Note when curObj_ == nObj_, we still schedule the timer
		// once, but we do not actually send out requests. This extra
		// schedule is only meant to be a hint to wait for the last
		// request to finish, then we will ask our parent to delete
		// this page. 
		if (curObj_ < nObj_) 
			TimerHandler::handle(e);
		// If this is not the last object, schedule the next one.
		// Otherwise stop and tell session to delete me.
		if (curObj_ > nObj_) 
			sess_->donePage((void*)this);
		else {
			curObj_++;
			sched(sess_->interObj()->value());
		}
	}
	int id_;
	WebTrafSession* sess_;
	int nObj_, curObj_;
	Node* dst_;
	static int LASTOBJ_;
};

int WebPage::LASTOBJ_ = 1;


int WebTrafSession::LASTPAGE_ = 1;

// XXX Must delete this after all pages are done!!
WebTrafSession::~WebTrafSession() 
{
	if (donePage_ != curPage_) {
		fprintf(stderr, "done pages %d != all pages %d\n",
			donePage_, curPage_);
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
}

void WebTrafSession::donePage(void* ClntData) 
{
	delete (WebPage*)ClntData;
	// If all pages are done, tell my parent to delete myself
	if (++donePage_ >= nPage_)
		mgr_->doneSession(id_);
}

// Launch the current page
void WebTrafSession::expire(Event *)
{
	// Pick destination for this page
	Node* dst = mgr_->pickdst();
	// Make sure page size is not 0!
	WebPage* pg = new WebPage(LASTPAGE_++, this, 
				  (int)ceil(rvPageSize_->value()), dst);
#if 1
	printf("Session %d starting page %d, curpage %d \n", 
	       id_, LASTPAGE_-1, curPage_);
#endif
	pg->start();
}

void WebTrafSession::handle(Event *e)
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
void WebTrafSession::launchReq(void* ClntData, int obj, int size)
{
	WebPage* pg = (WebPage*)ClntData;

	// Choose source and dest TCP agents for both source and destination
	TcpAgent* ctcp = mgr_->picktcp();
	TcpAgent* stcp = mgr_->picktcp();
	TcpSink* csnk = mgr_->picksink();
	TcpSink* ssnk = mgr_->picksink();

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-req %d %s %s %s %s %s %s %d", 
			      mgr_->name(), obj, src_->name(), 
			      pg->dst()->name(),
			      ctcp->name(), csnk->name(), stcp->name(),
			      ssnk->name(), size);
	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst
#if 0
	printf("%d \t %d \t %d \t %d \t %g %d %d\n", size, obj, page, id_,
		Scheduler::instance().clock(), 
	       src_->address(), dst->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", mgr_->nTcp(),mgr_->nSink());
#endif
}


static class WebTrafPoolClass : public TclClass {
public:
        WebTrafPoolClass() : TclClass("PagePool/WebTraf") {}
        TclObject* create(int, const char*const*) {
		return (new WebTrafPool());
	}
} class_webtrafpool;

// By default we use constant request interval and page size
WebTrafPool::WebTrafPool() : 
	session_(NULL), nSrc_(0), server_(NULL), nClient_(0), client_(NULL),
	nTcp_(0), nSink_(0)
{
	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
}

WebTrafPool::~WebTrafPool()
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
	// XXX Destroy tcpPool_ and sinkPool_!!
}

TcpAgent* WebTrafPool::picktcp()
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

TcpSink* WebTrafPool::picksink()
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

int WebTrafPool::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "set-num-session") == 0) {
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) 
					delete session_[i];
				delete []session_;
			}
			nSession_ = atoi(argv[2]);
			session_ = new WebTrafSession*[nSession_];
			memset(session_, 0, sizeof(WebTrafSession*)*nSession_);
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
	} else if (argc == 9) {
		if (strcmp(argv[1], "create-session") == 0) {
			// <obj> create-session <session_index>
			//   <pages_per_sess> <launch_time>
			//   <inter_page_rv> <page_size_rv>
			//   <inter_obj_rv> <obj_size_rv>
			int n = atoi(argv[2]);
			if ((n < 0)||(n >= nSession_)||(session_[n] != NULL)) {
				fprintf(stderr,"Invalid session index %d\n",n);
				return (TCL_ERROR);
			}
			int npg = (int)strtod(argv[3], NULL);
			double lt = strtod(argv[4], NULL);
			WebTrafSession* p = 
				new WebTrafSession(this, picksrc(), npg, n);
			int res = lookup_rv(p->interPage(), argv[5]);
			res = (res == TCL_OK) ? 
				lookup_rv(p->pageSize(), argv[6]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->interObj(), argv[7]) : TCL_ERROR;
			res = (res == TCL_OK) ? 
				lookup_rv(p->objSize(), argv[8]) : TCL_ERROR;
			if (res == TCL_ERROR) {
				delete p;
				fprintf(stderr, "Invalid random variable\n");
				return (TCL_ERROR);
			}
			p->sched(lt);
			session_[n] = p;
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}


