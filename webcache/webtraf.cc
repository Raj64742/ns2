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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/webtraf.cc,v 1.20 2002/06/27 21:26:21 xuanc Exp $

#include "config.h"
#include <tclcl.h>
#include <iostream.h>

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
		id_(id), sess_(sess), nObj_(nObj), curObj_(0), doneObj_(0),
		dst_(dst) {}
	virtual ~WebPage() {}

	inline void start() {
		// Call expire() and schedule the next one if needed
		status_ = TIMER_PENDING;
		handle(&event_);
	}
	inline int id() const { return id_; }
	Node* dst() { return dst_; }

	void doneObject() {
		if (++doneObj_ >= nObj_) {
			//printf("doneObject: %g %d %d \n", Scheduler::instance().clock(), doneObj_, nObj_);
			sess_->donePage((void*)this);
		}
	}
	inline int curObj() const { return curObj_; }
	inline int doneObj() const { return doneObj_; }

private:
	virtual void expire(Event* = 0) {
		// Launch a request. Make sure size is not 0!
		if (curObj_ >= nObj_) 
			return;
		sess_->launchReq(this, LASTOBJ_++, 
				 (int)ceil(sess_->objSize()->value()));
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
		// if (curObj_ <= nObj_) {
		//
		// Polly Huang: Wed Nov 21 18:18:51 CET 2001
		// With explicit doneObject() upcalls from the tcl
		// space, we don't need to play this trick anymore.
		if (curObj_ < nObj_) {
			// If this is not the last object, schedule the next 
			// one. Otherwise stop and tell session to delete me.
			TimerHandler::handle(e);
			curObj_++;
			// Kun-chan Lan: Mon Feb 11 10:12:27 PST 2002
			// Don't schedule another one when curObj_ = nObj_
			// otherwise the page might already have been deleted
			// before the next one is up and cause seg fault
			// in the case of larger interObj()->value()
			// sched(sess_->interObj()->value());
			if (curObj_ < nObj_) sched(sess_->interObj()->value());
		}
	}
	int id_;
	WebTrafSession* sess_;
	int nObj_, curObj_, doneObj_;
	Node* dst_;
	static int LASTOBJ_;
};

int WebPage::LASTOBJ_ = 1;

int WebTrafSession::LASTPAGE_ = 1;

// Constructor
WebTrafSession::WebTrafSession(WebTrafPool *mgr, Node *src, int np, int id, int ftcp_, int recycle_p) : 
	rvInterPage_(NULL), rvPageSize_(NULL),
	rvInterObj_(NULL), rvObjSize_(NULL), 
	mgr_(mgr), src_(src), nPage_(np), curPage_(0), donePage_(0),
	id_(id), interPageOption_(1), fulltcp_(0) {
	fulltcp_ = ftcp_;
	recycle_page_ = recycle_p;
}

// XXX Must delete this after all pages are done!!
WebTrafSession::~WebTrafSession() 
{
	if (donePage_ != curPage_) {
		fprintf(stderr, "done pages %d != all pages %d\n",
			donePage_, curPage_);
		abort();
	}
	if (status_ != TIMER_IDLE) {
		fprintf(stderr, "WebTrafSession must be idle when deleted.\n");
		abort();
	}

	// Recycle the objects of page level attributes if needed
	// Reuse these objects may save memory for large simulations--xuanc
	if (recycle_page_) {
		if (rvInterPage_ != NULL)
			Tcl::instance().evalf("delete %s", rvInterPage_->name());
		if (rvPageSize_ != NULL)
			Tcl::instance().evalf("delete %s", rvPageSize_->name());
		if (rvInterObj_ != NULL)
			Tcl::instance().evalf("delete %s", rvInterObj_->name());
		if (rvObjSize_ != NULL)
			Tcl::instance().evalf("delete %s", rvObjSize_->name());
	}
}

void WebTrafSession::donePage(void* ClntData) 
{
	WebPage* pg = (WebPage*)ClntData;
	if (mgr_->isdebug()) 
		printf("Session %d done page %d\n", id_, 
		       pg->id());
	if (pg->doneObj() != pg->curObj()) {
		fprintf(stderr, "done objects %d != all objects %d\n",
			pg->doneObj(), pg->curObj());
		abort();
	}
	delete pg;
	// If all pages are done, tell my parent to delete myself
	//
	if (++donePage_ >= nPage_)
		mgr_->doneSession(id_);
	else if (interPageOption_) {
		// Polly Huang: Wed Nov 21 18:23:30 CET 2001
		// add inter-page time option
		// inter-page time = end of a page to the start of the next
		sched(rvInterPage_->value());
		// printf("donePage: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
	}
}

// Launch the current page
void WebTrafSession::expire(Event *)
{
	// Pick destination for this page
	Node* dst = mgr_->pickdst();
	// Make sure page size is not 0!
	WebPage* pg = new WebPage(LASTPAGE_++, this, 
				  (int)ceil(rvPageSize_->value()), dst);
	if (mgr_->isdebug())
		printf("Session %d starting page %d, curpage %d \n", 
		       id_, LASTPAGE_-1, curPage_);
	pg->start();
}

void WebTrafSession::handle(Event *e)
{
	// If I haven't scheduled all my pages, do the next one
	TimerHandler::handle(e);
	++curPage_;
	// XXX Notice before each page is done, it will schedule itself 
	// one more time, this makes sure that this session will not be
	// deleted after the above call. Thus the following code will not
	// be executed in the context of a deleted object. 
	//
	// Polly Huang: Wed Nov 21 18:23:30 CET 2001
	// add inter-page time option
	// inter-page time = inter-page-start time
	// If the interPageOption_ is not set, the XXX Notice above applies.
	if (!interPageOption_) {
		if (curPage_ < nPage_) {
			sched(rvInterPage_->value());
			// printf("schedule: %g %d %d\n", Scheduler::instance().clock(), donePage_, curPage_);
		}
	}
}

// Launch a request for a particular object
void WebTrafSession::launchReq(void* ClntData, int obj, int size)
{
	Agent *csnk, *ssnk;
	WebPage* pg = (WebPage*)ClntData;

	// Choose source and dest TCP agents for both source and destination
	TcpAgent* ctcp = mgr_->picktcp();
	TcpAgent* stcp = mgr_->picktcp();

	if (fulltcp_) {
		csnk = mgr_->picktcp();
		ssnk = mgr_->picktcp();
	} else {
		csnk = mgr_->picksink();
		ssnk = mgr_->picksink();
	}

	// Setup TCP connection and done
	Tcl::instance().evalf("%s launch-req %d %d %s %s %s %s %s %s %d %d", 
			      mgr_->name(), obj, pg->id(),
			      src_->name(), pg->dst()->name(),
			      ctcp->name(), csnk->name(), stcp->name(),
			      ssnk->name(), size, ClntData);
	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst
#if 0
	printf("%d \t %d \t %d \t %d \t %g %d %d\n", size, obj, pg->id(), id_,
	       Scheduler::instance().clock(), 
	       src_->address(), pg->dst()->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", mgr_->nTcp(),mgr_->nSink());
#endif
}


static class WebTrafPoolClass : public TclClass {
public:
        WebTrafPoolClass() : TclClass("PagePool/WebTraf") { 	

	}
        TclObject* create(int, const char*const*) {
		return (new WebTrafPool());
	}
} class_webtrafpool;

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
	// XXX Destroy tcpPool_ and sinkPool_ ?
}

void WebTrafPool::delay_bind_init_all()
{
	delay_bind_init_one("debug_");
	PagePool::delay_bind_init_all();
}

int WebTrafPool::delay_bind_dispatch(const char *varName,const char *localName,
				     TclObject *tracer)
{
	if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) 
		return TCL_OK;
	return PagePool::delay_bind_dispatch(varName, localName, tracer);
}

// By default we use constant request interval and page size
WebTrafPool::WebTrafPool() : 
	session_(NULL), nServer_(0), server_(NULL), nClient_(0), client_(NULL),
	nTcp_(0), nSink_(0), fulltcp_(0), recycle_page_(0), server_delay_(0)
{
	bind("fulltcp_", &fulltcp_);
	bind("recycle_page_", &recycle_page_);
	bind("server_delay_", &server_delay_);
	// Debo
	asimflag_=0;
	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
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

	// Debojyoti Dutta ... for asim
	if (argc == 2){
		if (strcmp(argv[1], "use-asim") == 0) {
			asimflag_ = 1;
			//Tcl::instance().evalf("puts \"Here\"");
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
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
			nServer_ = atoi(argv[2]);
			if (server_ != NULL) 
				delete []server_;
			server_ = new WebServer[nServer_];
			// Initialize the data structure for web servers
			for (int i = 0; i < nServer_; i++) {
				server_[i].finish_t_ = 0;
				server_[i].rate_ = 1;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-client") == 0) {
			nClient_ = atoi(argv[2]);
			if (client_ != NULL) 
				delete []client_;
			client_ = new Node*[nClient_];
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-interPageOption") == 0) {
			int option = atoi(argv[2]);
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) {
					WebTrafSession* p = session_[i];
					p->set_interPageOption(option);
				}
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "doneObj") == 0) {
			WebPage* p = (WebPage*)atoi(argv[2]);
			// printf("doneObj for Page id: %d\n", p->id());
			p->doneObject();
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "set-server") == 0) {
			Node* s = (Node*)lookup_obj(argv[3]);
			if (s == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nServer_) {
				fprintf(stderr, "Wrong server index %d\n", n);
				return TCL_ERROR;
			}
			server_[n].node = s;
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-client") == 0) {
			Node* c = (Node*)lookup_obj(argv[3]);
			if (c == NULL)
				return (TCL_ERROR);
			int n = atoi(argv[2]);
			if (n >= nClient_) {
				fprintf(stderr, "Wrong client index %d\n", n);
				return TCL_ERROR;
			}
			client_[n] = c;
			return (TCL_OK);
		} else if (strcmp(argv[1], "recycle") == 0) {
			// <obj> recycle <tcp> <sink>
			//
			// Recycle a TCP source/sink pair
			Agent* tcp = (Agent*)lookup_obj(argv[2]);
			Agent* snk = (Agent*)lookup_obj(argv[3]);
			if ((tcp == NULL) || (snk == NULL))
				return (TCL_ERROR);
			
			if (fulltcp_) {
				delete tcp;
				delete snk;
			} else {
				// Recyle both tcp and sink objects
				nTcp_++;
				// XXX TBA: recycle tcp agents
				insertAgent(&tcpPool_, tcp);
				nSink_++;
				insertAgent(&sinkPool_, snk);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "job_arrival") == 0) {
			// <obj> job_arrive <server> <size> 
			int sid = atoi(argv[2]);
			int size = atoi(argv[3]);
			
			int n = 0;
			while ((server_[n].node)->nodeid() != sid) {
				n++;
			}
			
			if ((server_[n].node)->nodeid() != sid) {
				return (TCL_ERROR);
			} else {
				double delay = server_[n].job_arrival(size);
				
				Tcl::instance().resultf("%f", delay);
				return (TCL_OK);
			}
		} else if (strcmp(argv[1], "set_server_rate") == 0) {
			// <obj> set_rate <server> <size> 
			int sid = atoi(argv[2]);
			int rate = atoi(argv[3]);
			
			int n = 0;
			while ((server_[n].node)->nodeid() != sid) {
				n++;
			}
			
			if ((server_[n].node)->nodeid() != sid) {
				return (TCL_ERROR);
			} else {
				server_[n].rate_ = rate;
				return (TCL_OK);
			}
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
				new WebTrafSession(this, picksrc(), npg, n, fulltcp_, recycle_page_);
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
			
			// Debojyoti added this for asim
			if(asimflag_){										// Asim stuff. Added by Debojyoti Dutta
				// Assumptions exist 
				//Tcl::instance().evalf("puts \"Here\"");
				double lambda = (1/(p->interPage())->avg())/(nServer_*nClient_);
				double mu = ((p->objSize())->value());
				//Tcl::instance().evalf("puts \"Here\"");
				for (int i=0; i<nServer_; i++){
					for(int j=0; j<nClient_; j++){
						// Set up short flows info for asim
						Tcl::instance().evalf("%s add2asim %d %d %lf %lf", this->name(),(server_[i].node)->nodeid(),client_[j]->nodeid(),lambda, mu);
					}
				}
				//Tcl::instance().evalf("puts \"Here\""); 
			}
			
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}

// Simple web server implementation
double WebServer::job_arrival(int job_size) {
	double now = Scheduler::instance().clock();
	
	if (finish_t_ < now) {
		finish_t_ = now;
	}
	
	finish_t_ = finish_t_ + job_size / rate_;
	
	double delay_ = finish_t_ - now;
	//printf("%d, %f, %f %f\n", job_size, rate_, finish_t_, delay_);
	return delay_;
}

 
 
