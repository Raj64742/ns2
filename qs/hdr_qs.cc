/*
 * Copyright (c) 2001 University of Southern California.
 * All rights reserved.                                            
 *                                                                
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 * Quick Start for TCP and IP.
 * A scheme for transport protocols to dynamically determine initial 
 * congestion window size.
 *
 * http://www.ietf.org/internet-drafts/draft-amit-quick-start-02.ps
 *
 * Packet header definition for Quick Start implementation
 * hdr_qs.h
 *
 * Srikanth Sundarrajan, 2002
 * sundarra@usc.edu
 */

#include <stdio.h>
#include "hdr_qs.h"
#include "qsagent.h"
#include <math.h>

int hdr_qs::offset_;

static class QSHeaderClass : public PacketHeaderClass {
public:
	QSHeaderClass() : PacketHeaderClass("PacketHeader/TCP_QS", sizeof(hdr_qs)) {
		bind_offset(&hdr_qs::offset_);
	}

} class_QShdr;


/*
 * These two functions convert rate in QS packet to KBps and vice versa.
 */
double hdr_qs::rate_to_Bps(int rate)
{
       if (rate == 0)
               return 0;
       switch(QSAgent::rate_function_) {
       case 1:
       default:
               return rate * 10240; // rate unit: 10 kilobytes per sec
       case 2:
               return pow(2, rate) * 5000; // exponential, base 2
       }
}

int hdr_qs::Bps_to_rate(double Bps)
{
       if (Bps == 0)
               return 0;
       switch(QSAgent::rate_function_) {
       case 1:
       default:
               return (int) Bps / 10240; // rate unit: 10 kilobytes per sec
       case 2:
               return (int) (log(Bps / 5000) / log(2));
       }
}

// Local Variables:
// mode:c++
// c-basic-offset: 8
// End:

