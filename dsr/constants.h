// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Ported from CMU/Monarch's code, appropriate copyright applies.  

/*   constants.h

   nice constants to know about that are used in various places
*/

#ifndef _constants_h
#define _constants_h

#include "path.h"

/************** exported from lsnode.cc **************/
extern Time RR_timeout;		// (sec) route request timeout
extern Time arp_timeout;	// (sec) arp request timeout
extern Time retry_arp_period;	// secs time between arps
extern Time rt_rq_period;	// secs length of one backoff period
extern Time rt_rq_max_period;	// secs maximum time between rt reqs
extern Time rt_rep_holdoff_period;
// to determine how long to sit on our rt reply we pick a number
// U(O.0,rt_rep_holdoff_period) + (our route length-1)*rt_rep_holdoff

extern Time process_time; // average time taken to process a packet 

#endif // _constants_h
