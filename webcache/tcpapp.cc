// Copyright (c) Xerox Corporation 1998. All rights reserved.
//
// License is granted to copy, to use, and to make and to use derivative
// works for research and evaluation purposes, provided that Xerox is
// acknowledged in all documentation pertaining to any such copy or
// derivative work. Xerox grants no other licenses expressed or
// implied. The Xerox trade name should not be used in any advertising
// without its written permission. 
//
// XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
// MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
// FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
// express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// software. 
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/tcpapp.cc,v 1.1 1998/08/18 23:42:44 haoboy Exp $
//
// Tcp application: transmitting real application data
// 
// Allows only one connection. Model underlying TCP connection as a 
// FIFO byte stream, use this to deliver user data

#include "agent.h"
#include "app.h"
#include "tcpapp.h"

CBuf::CBuf(const char *c, int size, int nbytes)
{
	nbytes_ = nbytes;
	if (c != NULL) {
		data_ = new char[size];
		memcpy(data_, c, size);
		size_ = size;
	} else {
		data_ = NULL;
		size_ = 0;
	}
	next_ = NULL;
}

CBufList::~CBufList() 
{
	while (head_ != NULL) {
		tail_ = head_;
		head_ = head_->next_;
		delete tail_;
	}
}

void CBufList::insert(CBuf *cbuf) 
{
	if (tail_ == NULL) 
		head_ = tail_ = cbuf;
	else {
		tail_->next_ = cbuf;
		tail_ = cbuf;
	}
#ifdef TCPAPP_DEBUG
	num_++;
#endif
}

CBuf* CBufList::detach()
{
	if (head_ == NULL)
		return NULL;
	CBuf *p = head_;
	if ((head_ = head_->next_) == NULL)
		tail_ = NULL;
#ifdef TCPAPP_DEBUG
	num_--;
#endif
	return p;
}

static class TcpCncClass : public TclClass {
public:
	TcpCncClass() : TclClass("Application/TcpApp") {}
	TclObject* create(int argc, const char*const* argv) {
		if (argc != 5)
			return NULL;
		Agent *tcp = (Agent *)TclObject::lookup(argv[4]);
		if (tcp == NULL) 
			return NULL;
		return (new TcpApp(tcp));
	}
} class_tcpcnc_app;

TcpApp::TcpApp(Agent *tcp) : 
	Application(), curbytes_(0), curdata_(0)
{
	agent_ = tcp;
	agent_->attachApp(this);
}

TcpApp::~TcpApp()
{
	// XXX Before we quit, let our agent know what we no longer exist
	// so that it won't give us a call later...
	agent_->attachApp(NULL);
}

// Send with callbacks to transfer application data
void TcpApp::send(int nbytes, int datasize, const char *cbk)
{
	CBuf *p = new CBuf(cbk, datasize, nbytes);
#ifdef TCPAPP_DEBUG
	p->time() = Scheduler::instance().clock();
#endif
	cbuf_.insert(p);
	Application::send(nbytes);
}

// All we need to know is that our sink has received one message
void TcpApp::recv(int size)
{
	// If it's the start of a new transmission, grab info from dest, 
	// and execute callback
	if (curdata_ == 0)
		curdata_ = dst_->get_data();
	if (curdata_ == 0) {
		fprintf(stderr, "[%g] %s receives a packet but no callback!\n",
			Scheduler::instance().clock(), name_);
		return;
	}
	curbytes_ += size;
#ifdef TCPAPP_DEBUG
	fprintf(stderr, "[%g] Get data size %d, %s\n", 
		Scheduler::instance().clock(), curbytes_, 
		curdata_->data());
#endif
	if (curbytes_ == curdata_->bytes()) {
		// We've got exactly the data we want
		// If we've received all data, execute the callback
		process_data(curdata_->size(), curdata_->data());
		// Then cleanup this data transmission
		delete curdata_;
		curdata_ = NULL;
		curbytes_ = 0;
	} else if (curbytes_ > curdata_->bytes()) {
		// We've got more than we expected. Must contain other data.
		// Continue process callbacks until the unfinished callback
		while (curbytes_ >= curdata_->bytes()) {
			process_data(curdata_->size(), curdata_->data());
			curbytes_ -= curdata_->bytes();
#ifdef TCPAPP_DEBUG
			fprintf(stderr, "[%g] Get data size %d(left %d) %s\n", 
				Scheduler::instance().clock(), 
				curdata_->bytes(), curbytes_, 
				curdata_->data());
#endif
			delete curdata_;
			curdata_ = dst_->get_data();
			if (curdata_ != 0)
				continue;
			if ((curdata_ == 0) && (curbytes_ > 0)) {
				fprintf(stderr, "[%g] %s gets extra data!\n",
					Scheduler::instance().clock(), name_);
				abort();
			} else
				// Get out of the look without doing a check
				break;
		}
	}
}

void TcpApp::resume()
{
	// Do nothing
}

int TcpApp::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (strcmp(argv[1], "connect") == 0) {
		dst_ = (TcpApp *)TclObject::lookup(argv[2]);
		if (dst_ == NULL) {
			tcl.resultf("%s: connected to null object.", name_);
			return (TCL_ERROR);
		}
		dst_->connect(this);
		return (TCL_OK);
	} else if (strcmp(argv[1], "send") == 0) {
		/*
		 * <app> send <size> <tcl_script>
		 */
		int size = atoi(argv[2]);
		if (argc == 3)
			send(size, 0, NULL);
		else 
			send(size, strlen(argv[3])+1, argv[3]);
		return (TCL_OK);
	} else if (strcmp(argv[1], "dst") == 0) {
		tcl.resultf("%s", dst_->name());
		return TCL_OK;
	}
	return Application::command(argc, argv);
}
