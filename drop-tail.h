/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/drop-tail.h,v 1.14 2001/08/02 03:59:53 sfloyd Exp $ (LBL)
 */

#ifndef ns_drop_tail_h
#define ns_drop_tail_h

#include <string.h>
#include "queue.h"
#include "config.h"

/*
 * A bounded, drop-tail queue
 */
class DropTail : public Queue {
  public:
	DropTail() { 
		q_ = new PacketQueue; 
		pq_ = q_;
		bind_bool("drop_front_", &drop_front_);
		bind_bool("summarystats_", &summarystats);
		//		_RENAMED("drop-front_", "drop_front_");
	}
	~DropTail() {
		delete q_;
	}
  protected:
	void reset();
	int command(int argc, const char*const* argv); 
	void enque(Packet*);
	Packet* deque();
	PacketQueue *q_;	/* underlying FIFO queue */
	int drop_front_;	/* drop-from-front (rather than from tail) */
	int summarystats;
	double v_true_ave;	/* true long-term average queue size */
	double v_total_time;	/* total time average queue size compute for */
	void print_summarystats();
};

#endif
