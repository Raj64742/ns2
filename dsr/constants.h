/*  -*- c++ -*- 
   constants.h

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
