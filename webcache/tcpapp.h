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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/tcpapp.h,v 1.13 1999/07/16 16:58:06 haoboy Exp $
//
// TcpApp - Transmitting real application data via TCP
//
// XXX Model a TCP connection as a FIFO byte stream. It shouldn't be used 
// if this assumption is violated.

#ifndef ns_tcpapp_h
#define ns_tcpapp_h

#include "ns-process.h"
#include "app.h"

class CBuf { 
public:
	CBuf(const AppData *c, int nbytes);
	~CBuf() {
		if (data_ != NULL)
			delete []data_;
	}
	char* data() { return data_; }
	int size() { return size_; }
	int bytes() { return nbytes_; }

#ifdef TCPAPP_DEBUG	
	// debug only
	double& time() { return time_; }
#endif

protected:
	friend class CBufList;
	char *data_;
	int size_;
	int nbytes_; 	// Total length of this transmission
	CBuf *next_;

#ifdef TCPAPP_DEBUG
	// for debug only 
	double time_; 
#endif
};

// A FIFO queue
class CBufList {
public:	
#ifdef TCPAPP_DEBUG 
	CBufList() : head_(NULL), tail_(NULL), num_(0) {}
#else
	CBufList() : head_(NULL), tail_(NULL) {}
#endif
	~CBufList();

	void insert(CBuf *cbuf);
	CBuf* detach();

protected:
	CBuf *head_;
	CBuf *tail_;
#ifdef TCPAPP_DEBUG
	int num_;
#endif
};


class TcpApp : public Application {
public:
	TcpApp(Agent *tcp);
	~TcpApp();

	virtual void recv(int nbytes);
	virtual void send(int nbytes, const AppData *data);

	void connect(TcpApp *dst) { dst_ = dst; }

	virtual void process_data(int size, char* data);
	virtual AppData* get_data(int&, const AppData*) {
		// Not supported
		abort();
		return NULL;
	}

	// Do nothing when a connection is terminated
	virtual void resume();

protected:
	virtual int command(int argc, const char*const* argv);
	CBuf* rcvr_retrieve_data() { return cbuf_.detach(); }

	// We don't have start/stop
	virtual void start() { abort(); } 
	virtual void stop() { abort(); }

	TcpApp *dst_;
	CBufList cbuf_;
	CBuf *curdata_;
	int curbytes_;
};

#endif // ns_tcpapp_h
