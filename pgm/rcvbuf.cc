#ifdef PGM

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
 * rcvbuf.cc
 *
 * Utility class used by receivers to provide for packet loss detection,
 * and to compute latency statistics for recovered packets.
 *
 * Christos Papadopoulos
 */

#include <stdio.h>
#include "rcvbuf.h"

RcvBuffer::RcvBuffer ()
{
	nextpkt_ = 0;
	maxpkt_ = -1;
	duplicates_ = 0;
	delay_sum_ = 0.0;
	max_delay_ = 0.0;
	min_delay_ = 1e6;
	pkts_recovered_ = 0;
	tail_ = gap_ = 0;
}

//
// add pkt to buffer with seqno i, received at time tnow
//
void RcvBuffer::add_pkt (int i, double tnow)
{
	Gap	*g, *pg;

	if (exists_pkt (i))
		{
		duplicates_++;
		return;
		}
	// common case first: got what we expected
	if (i == nextpkt_)
		{
		nextpkt_++;
		if (i == maxpkt_ + 1)
			maxpkt_++;
		return;
		}
	// new gap
	if (i > nextpkt_)
		{
		g = new Gap;
		g->start_ = nextpkt_;
		g->end_ = i-1;
		g->time_ = tnow;
		g->next_ = 0;

		if (!gap_)
			gap_ = tail_ = g;
		else	{
			tail_->next_ = g;
			tail_ = g;
			}
		nextpkt_ = i+1;
		return;
		}

	// i < nextpkt_ (a retransmission)

	pkts_recovered_++;

	// is packet part of the first gap?
	if (gap_->start_ <= i && i <= gap_->end_)
		{
		double	d = tnow - gap_->time_;

		if (d > max_delay_)
			max_delay_ = d;
		if (d < min_delay_)
			min_delay_ = d;
		delay_sum_ += d;
		if (i == maxpkt_ + 1)
			{
			maxpkt_++;
			if (++gap_->start_ > gap_->end_)
				{
				g = gap_;
				gap_ = gap_->next_;
				if (gap_)
					maxpkt_ = gap_->start_ - 1;
				else	{
					maxpkt_ = nextpkt_ - 1;
					tail_ = 0;
					}
				delete g;
				}
			return;
			}
		g = gap_;
		}
	else	{
		double	d;

		// locate gap this packet belongs to
		pg = gap_;
		g = gap_->next_;
		while (!(i >= g->start_ && i <= g->end_))
			{
			pg = g;
			g = g->next_;
			}
		d = tnow - g->time_;
		delay_sum_ += d;
		if (d > max_delay_) max_delay_ = d;
		if (d < min_delay_) min_delay_ = d;

		// first packet in gap
		if (g->start_ == i)
			{
			if (++g->start_ > g->end_)
				{
				pg->next_ = g->next_;
				if (tail_ == g)
					tail_ = pg;
				delete g;
				}
			return;
			}
		}
	// last packet in gap
	if (g->end_ == i)
		{
		g->end_--;
		return;
		}
	// a packet in the middle of gap
	pg = new Gap;
	pg->start_ = i+1;
	pg->end_   = g->end_;
	pg->time_  = g->time_;
	pg->next_  = g->next_;

	g->next_ = pg;
	g->end_  = i-1;
	if (tail_ == g)
		tail_ = pg;
}

//
// Return 0 if packet does not exist in the
// buffer, 1 if it does
//
int RcvBuffer::exists_pkt (int i)
{
	if (i <= maxpkt_)  return 1;
	if (i >= nextpkt_) return 0;

	Gap *g;
	for (g = gap_; g; g = g->next_)
		if (g->start_ <= i && i <= g->end_)
			return 0;
	return 1;
}

void RcvBuffer::print ()
{
	Gap	*g;

	printf ("maxpkt: %d ", maxpkt_);

	for (g = gap_; g; g = g->next_)
		printf ("(%d, %d) ", g->start_, g->end_);

	printf ("nextpkt: %d\n", nextpkt_);
}

#endif
