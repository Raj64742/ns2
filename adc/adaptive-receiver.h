/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/adc/adaptive-receiver.h,v 1.2 1998/06/27 01:23:20 gnguyen Exp $
 */

#ifndef ns_adaptivercvr_h
#define ns_adaptivercvr_h

class AdaptiveRcvr : public Agent {
	public :
	AdaptiveRcvr();
	void recv(Packet* pkt, Handler*);
	protected :
	int off_rtp_;

	virtual int adapt(Packet *pkt,u_int32_t time)=0;
};

/*
  XXX
  Samplerate defined here in receiver as well so that the delay stats can be printed
  in real time */

#define SAMPLERATE 8000.0

#endif
