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
// Empirical FTP traffic model that simulates FTP traffic based on a set of
// CDF (Cumulative Distribution Function) data derived from live tcpdump trace
// The structure of this file is largely borrowed from empweb.cc
//

#include <tclcl.h>

#include "empftp.h"


int EmpFtpTrafSession::LASTFILE_ = 1;

// XXX Must delete this after all pages are done!!
EmpFtpTrafSession::~EmpFtpTrafSession() 
{

	if (nFile_ != curFile_) {
		fprintf(stderr, "done files %d != all files %d\n",
			nFile_, curFile_);
		abort();
	}
	if (status_ != TIMER_IDLE) {
		fprintf(stderr, "EmpFtpTrafSession must be idle when deleted.\n");
		abort();
	}
/*	
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

*/

}


// Launch the current file
void EmpFtpTrafSession::expire(Event *)
{
	int n;

	if (curFile_ >= nFile_) return;

	sendFile(LASTFILE_++, (int)ceil(rvFileSize_->value()));

	if (mgr_->isdebug())
		printf("Session %d starting file %d, curfile %d \n", 
		       id_, LASTFILE_-1, curFile_);

}

void EmpFtpTrafSession::handle(Event *e)
{
        TimerHandler::handle(e);
    	if (curFile_ < nFile_) {
            	// If this is not the last file, schedule the next
                // one. Otherwise stop and tell session to delete itself.
                curFile_++;
		sched(rvInterFile_->value());
     	} else
	    	mgr_->doneSession(id_);
}

// Launch a request for a particular object
void EmpFtpTrafSession::sendFile(int file, int size)
{

	int wins = int(ceil(serverWin()->value()));
	int winc = int(ceil(clientWin()->value()));
	int window = (wins >= winc) ? wins : winc;

	// Choose source and dest TCP agents for both source and destination
	TcpAgent* ctcp = mgr_->picktcp(window);
	TcpAgent* stcp = mgr_->picktcp(window);
	TcpSink* csnk = mgr_->picksink();
	TcpSink* ssnk = mgr_->picksink();

	// Setup new TCP connection and launch request
	Tcl::instance().evalf("%s send-file %d %s %s %s %s %d", 
	     	mgr_->name(), file, src_->name(), 
	      	dst_->name(), stcp->name(), ssnk->name(), size);


	// Debug only
	// $numPacket_ $objectId_ $pageId_ $sessionId_ [$ns_ now] src dst

if (mgr_->isdebug()) {
	printf("%d \t %d \t %d \t %g %d %d\n", size, file, id_,
	       Scheduler::instance().clock(), 
	       src_->address(), dst_->address());
	printf("** Tcp agents %d, Tcp sinks %d\n", mgr_->nTcp(),mgr_->nSink());
}
}


static class EmpFtpTrafPoolClass : public TclClass {
public:
        EmpFtpTrafPoolClass() : TclClass("PagePool/EmpFtpTraf") {}
        TclObject* create(int, const char*const*) {
		return (new EmpFtpTrafPool());
	}
} class_empwebtrafpool;

EmpFtpTrafPool::~EmpFtpTrafPool()
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

void EmpFtpTrafPool::delay_bind_init_all()
{
	delay_bind_init_one("debug_");
	PagePool::delay_bind_init_all();
}

int EmpFtpTrafPool::delay_bind_dispatch(const char *varName,const char *localName,
				     TclObject *tracer)
{
	if (delay_bind_bool(varName, localName, "debug_", &debug_, tracer)) 
		return TCL_OK;
	return PagePool::delay_bind_dispatch(varName, localName, tracer);
}

EmpFtpTrafPool::EmpFtpTrafPool() : 
	session_(NULL), nSrc_(0), server_(NULL), nClient_(0), client_(NULL),
	nTcp_(0), nSink_(0)
{
	LIST_INIT(&tcpPool_);
	LIST_INIT(&sinkPool_);
}

TcpAgent* EmpFtpTrafPool::picktcp(int win)
{

	TcpAgent* a = (TcpAgent*)detachHead(&tcpPool_);
	if (a == NULL) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s alloc-tcp %d", name(), win);
		a = (TcpAgent*)lookup_obj(tcl.result());
		if (a == NULL) {
			fprintf(stderr, "Failed to allocate a TCP agent\n");
			abort();
		}
	} else 
		nTcp_--;
	return a;
}

TcpSink* EmpFtpTrafPool::picksink()
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

int EmpFtpTrafPool::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "set-num-session") == 0) {
			if (session_ != NULL) {
				for (int i = 0; i < nSession_; i++) 
					delete session_[i];
				delete []session_;
			}
			nSession_ = atoi(argv[2]);
			session_ = new EmpFtpTrafSession*[nSession_];
			memset(session_, 0, sizeof(EmpFtpTrafSession*)*nSession_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-server-lan") == 0) {
			nSrcL_ = atoi(argv[2]);
			if (nSrcL_ >  nSrc_) {
				fprintf(stderr, "Wrong server index %d\n", nSrcL_);
				return TCL_ERROR;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "set-num-remote-client") == 0) {
			nClientL_ = atoi(argv[2]);
			if (nClientL_ > nClient_) {
				fprintf(stderr, "Wrong client index %d\n", nClientL_);
				return TCL_ERROR;
			}
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
	} else if (argc == 11) {
		if (strcmp(argv[1], "create-session") == 0) {
			// <obj> create-session <session_index>
			//   <files_per_sess> <launch_time>
			//   <inter_file_rv> <file_size_rv>
			//   <server_sel_rv>
			//   <inbound/outbound flag>
			int n = atoi(argv[2]);
			if ((n < 0)||(n >= nSession_)||(session_[n] != NULL)) {
				fprintf(stderr,"Invalid session index %d\n",n);
				return (TCL_ERROR);
			}
			int nfile = (int)strtod(argv[3], NULL);
			double lt = strtod(argv[4], NULL);

			int flip = atoi(argv[10]);
			if ((flip < 0)||(flip > 1)) {
				fprintf(stderr,"Invalid I/O flag %d\n",flip);
				return (TCL_ERROR);
			}


			EmpFtpTrafSession* p = 
				new EmpFtpTrafSession(this, nfile, n);

			int res = lookup_rv(p->interFile(), argv[5]);
			res = (res == TCL_OK) ? 
				lookup_rv(p->fileSize(), argv[6]) : TCL_ERROR;
		   	res = (res == TCL_OK) ?
		          	lookup_rv(p->serverSel(), argv[7]) : TCL_ERROR;
		   	res = (res == TCL_OK) ?
		          	lookup_rv(p->serverWin(), argv[8]) : TCL_ERROR;
		   	res = (res == TCL_OK) ?
		          	lookup_rv(p->clientWin(), argv[9]) : TCL_ERROR;
			if (res == TCL_ERROR) {
				delete p;
				fprintf(stderr, "Invalid random variable\n");
				return (TCL_ERROR);
			}

                        int cl, svr;
			if (flip == 1) {
                          	cl = int(floor(Random::uniform(0, nClientL_)));
				svr=0;
			} else {
                          	cl = int(floor(Random::uniform(nClientL_, nClient_)));
           			svr= int(ceil(p->serverSel()->value()));
			}
                        assert((cl >= 0) && (cl < nClient_));
        		assert((svr >= 0) && (svr < nSrc_));
                        Node* c=client_[cl];
        		Node* s=server_[svr];

			p->setClient(c);
			p->setServer(s);

			p->sched(lt);
			session_[n] = p;
		       
			return (TCL_OK);
		}
	}
	return PagePool::command(argc, argv);
}


