/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/udp.h,v 1.11 1998/10/22 00:10:45 tomh Exp $ (Xerox)
 */

#ifndef ns_udp_h
#define ns_udp_h

#include "agent.h"
#include "trafgen.h"

//"rtp timestamp" needs the samplerate
#define SAMPLERATE 8000
#define RTP_M 0x0080 // marker for significant events

class UdpAgent : public Agent {
public:
	UdpAgent();
	UdpAgent(int);
	virtual void sendmsg(int nbytes, const char *flags = 0);
protected:
	int seqno_;
	int off_rtp_;
};

#endif
