/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

#ifndef ns_sa_h
#define ns_sa_h

#include "resv.h"


class SA_Agent : public UDP_Agent {
public:
	SA_Agent();
	int command(int, const char*const*);
	
protected:
	void start(); 
	void stop(); 
	void sendreq();
	void sendteardown();
	void recv(Packet *, Handler *);
	int off_resv_;
	double rate_;
	int bucket_;
};


#endif
