/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/trafgen.h,v 1.4 1998/06/09 21:53:20 breslau Exp $ (Xerox)
 */
 
#ifndef ns_trafgen_h
#define ns_trafgen_h

#include "object.h"
#include "scheduler.h"
#include "udp.h"

/* abstract class for traffic generation modules.  derived classes
 * must define  the next_interval() function.  the traffic generation 
 * module schedules an event for a UDP_Agent when it is time to 
 * generate a new packet.  it passes the packet size with the event 
 * (to accommodate traffic generation modules that may not use fixed 
 * size packets).
 */

/* abstract class for traffic generation modules.  derived classes
 * must define the next_interva() function that is invoked by
 * UDP_Agent.  This function returns the time to the next packet
 * is generated and sets the size of the packet (which is passed
 * by reference).  The init() method is called from the start()
 * method of the UDP_Agent.  It can do any one-time initialization
 * needed by the traffic generation process.
 */

class TrafficGenerator : public NsObject {
 public:
	TrafficGenerator() {}
	virtual double next_interval(int &) = 0;
	virtual void init() {}
	virtual double interval(){return 0;}
	virtual int on() {return 1;}
 protected:
	/* recv() shouldn't ever get called, it is needed because
	 * all NsObjects are Handlers as well. 
	 */
	void recv(Packet*, Handler*) { abort(); }
	int size_;
};

#endif

