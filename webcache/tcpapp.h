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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/tcpapp.h,v 1.7 1999/02/18 23:15:50 haoboy Exp $
//
// TcpApp - Transmitting real application data via TCP
//
// XXX Model a TCP connection as a FIFO byte stream. It shouldn't be used 
// if this assumption is violated.

#ifndef ns_tcpapp_h
#define ns_tcpapp_h

#include "app-connector.h"
#include "app.h"

// Buffer management stuff.
class CBuf { 
public:
	CBuf(AppData *c, int nbytes);
	~CBuf() {
		if (data_ != NULL)
			delete data_;
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

// ADU for plain TcpApp, which is by default a string of otcl script
class TcpAppString : public AppData {
public:
	TcpAppString() : AppData(TCPAPP_STRING), size_(0), str_(NULL) {}
	TcpAppString(char *b) : AppData(b) {
		b += hdrlen();
		if (*b == 0)
			size_ = NULL, str_ = 0;
		else {
			size_ = strlen(b) + 1;
			str_ = new char[size_];
			assert(str_ != NULL);
			strcpy(str_, b);
		}
	}
	virtual ~TcpAppString() { 
		if (str_ != NULL) 
			delete []str_; 
	}

	char* str() { return str_; }
	virtual int size() const { return hdrlen() + size_; }

	// Insert string-contents into the ADU
	void set_string(const char* s) {
		if ((s == NULL) || (*s == 0)) 
			str_ = NULL, size_ = 0;
		else {
			size_ = strlen(s) + 1;
			str_ = new char[size_];
			assert(str_ != NULL);
			strcpy(str_, s);
		}
	}

	virtual void pack(char* buf) const {
		AppData::pack(buf);
		buf += hdrlen();
		strcpy(buf, str_);
	}

protected:
	int size_;
	char* str_; 
};

class TcpApp : public Application, public AppConnector {
public:
	TcpApp(Agent *tcp);
	~TcpApp();

	virtual void recv(int nbytes);
	void send(int nbytes, AppData *data);

	void connect(TcpApp *dst) { dst_ = dst; }

	virtual void process_data(int size, char* data) {
		// XXX Default behavior:
		// If there isn't a target, use tcl to evaluate the data
		if (target_ != 0)
			send_data(size, data);
		else if (AppData::type(data) == TCPAPP_STRING) {
			TcpAppString *tmp = new TcpAppString(data);
			Tcl::instance().eval(tmp->str());
			delete tmp;
		}
	}

	// Do nothing when a connection is terminated
	virtual void resume();

protected:
	virtual int command(int argc, const char*const* argv);
	CBuf* get_data() { return cbuf_.detach(); }

	// We don't have start/stop
	virtual void start() { abort(); } 
	virtual void stop() { abort(); }

	TcpApp *dst_;
	CBufList cbuf_;
	CBuf *curdata_;
	int curbytes_;
};

#endif // ns_tcpapp_h
