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
// Definition of Agent/Invalidation
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/inval-agent.h,v 1.9 1999/05/26 01:20:16 haoboy Exp $

#ifndef ns_invalagent_h
#define ns_invalagent_h

#include <string.h>
#include <tclcl.h>
#include "config.h"
#include "packet.h"
#include "agent.h"
#include "tcpapp.h"

struct hdr_inval {
	int size_;
	int& size() { return size_; }
};

// XXX If we have an interface declaration, we could define a common 
// interface for the two implementations below. But in C++ the only way
// to define an interface is through abstract base class, and use 
// multiple inheritance to get different implementations with the same 
// interface. 
// Because multiple inheritance prohibits conversion from 
// base class pointer to derived class pointer, and TclObject::lookup 
// must do such a conversion, we cannot declare a common interface here. :(

class HttpApp;

// Implementation 1: multicast, this is a real agent
class HttpInvalAgent : public Agent {
public: 
	HttpInvalAgent();

	virtual void recv(Packet *, Handler *);
	virtual void send(int realsize, const AppData* data);

protected:
	int off_inv_;
	int inval_hdr_size_;
};

// Implementation 2: unicast, actually an application on top of TCP
class HttpUInvalAgent : public TcpApp {
public:
	HttpUInvalAgent(Agent *a) : TcpApp(a) {}

	void send(int realsize, const AppData* data) {
		TcpApp::send(realsize, data);
	}
	virtual void process_data(int size, char *data);
	virtual AppData* get_data(int&, const AppData*) {
		abort(); 
		return NULL;
	}
protected:
	virtual int command(int argc, const char*const* argv);
};

#endif // ns_invalagent_h
