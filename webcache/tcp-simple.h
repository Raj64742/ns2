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
// Simple TCP only preserves: 
// (1) FullTcp::advance-bytes, 
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/tcp-simple.h,v 1.9 1999/05/26 01:20:25 haoboy Exp $

#ifndef ns_tcp_simple_h
#define ns_tcp_simple_h

#include "tcp.h"

class SimpleTcpAgent : public TcpAgent {
public:
	SimpleTcpAgent();

	virtual void sendmsg(int nbytes, const char *flags);
	virtual void recv(Packet *pkt, Handler *);
	virtual int command(int argc, const char*const* argv);

	// To make base Tcp happy
	virtual void timeout(int) {} 
	virtual void timeout_nonrtx(int) {}

protected:
	int seqno_;
};

#endif // ns_tcp_simple_h
