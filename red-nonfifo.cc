/*
 * Copyright (c) 1990-1997 Regents of the University of California.
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
 *	This product includes software developed by the Daedalus Research 
 *      Group at the University of California at Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be used
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
 */
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include "red.h"
#include "queue-nonfifo.h"

class NonFifoREDQueue : public REDQueue, QueueHelper {
  public:	
	NonFifoREDQueue();
	PacketQueue *q() { return nfq_; }
  protected:
	NonFifoPacketQueue *nfq_;
	void enque(PacketQueue *q, Packet *pkt) {
		QueueHelper::enque((NonFifoPacketQueue *)q, pkt, off_cmn_, off_tcp_, off_ip_);
	}
	Packet* deque(PacketQueue *q) {
		return QueueHelper::deque((NonFifoPacketQueue *)q, off_cmn_);
	}
	void remove(PacketQueue *q, Packet *pkt) {
		QueueHelper::remove((NonFifoPacketQueue *)q, pkt, off_cmn_);
	}
	NonFifoPacketQueue q_;
 private:
        int off_ip_;
        int off_tcp_;
};

static class NonFifoREDClass : public TclClass {
public:
	NonFifoREDClass() : TclClass("Queue/RED/NonFifo") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NonFifoREDQueue);
	}
} class_red_nonfifo;

NonFifoREDQueue::NonFifoREDQueue()
{
        bind("off_ip_", &off_ip_);
        bind("off_tcp_", &off_tcp_);

	bind_bool("interleave_", &interleave_);
	bind_bool("acksfirst_", &acksfirst_);
	bind_bool("ackfromfront_", &ackfromfront_);
	bind_bool("fracthresh_", &edp_.fracthresh);
	bind_bool("filteracks_", &filteracks_);
	bind_bool("replace_head_", &replace_head_);
	nfq_ = new NonFifoPacketQueue;
}
