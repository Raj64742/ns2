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
 * Pragmatic General Multicast (PGM), Reliable Multicast
 *
 * rcvbuf.h
 *
 * Class RcvBuffer: maintains information about what packets were received,
 * and keeps track of gaps in packet sequence number.
 *
 * Christos Papadopoulos
 */

#ifdef PGM
#ifndef RCVBUF_H_
#define RCVBUF_H_
class Gap;

class RcvBuffer {
 public:
	RcvBuffer ();
	//	~RcvBuffer ();
	void	add_pkt (int, double);	// add packet with seq num n, received at time t
	int	exists_pkt (int);	// query the existence of packet n
	void	print ();
	int	nextpkt_;		// seq num of the next expected packet
	int	maxpkt_;		// packets received without gap
	int	pkts_recovered_;	// total number of recovered packets
	int	duplicates_;		// duplicate packets
	double	delay_sum_;		// total delay to recover lost packets
	double	max_delay_;		// max recovery delay
	double	min_delay_;		// min recovery delay

 protected:
	Gap	*gap_;
	Gap	*tail_;
};

//
// Class Gap: identifies a gap
// in packet reception sequence number
//
class Gap {
 public:
	int	start_;
	int	end_;
	double	time_;
	Gap	*next_;
};

#endif

#endif
