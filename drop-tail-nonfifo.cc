/*
 * Copyright (c) 1997 Regents of the University of California.
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
#include "drop-tail.h"
#include "queue-nonfifo.h"

/*
 * A bounded, drop-tail queue with special enque & deque functions.
 */
class NonFifoDropTail : public DropTail {
 public:
	NonFifoDropTail();
	PacketQueue *q() { return nfq_; }
 protected:
	NonFifoPacketQueue *nfq_;
	void enque(PacketQueue *q, Packet *p) {
		((NonFifoPacketQueue *)q)->enque(p, off_cmn_,off_tcp_,off_ip_); }
	Packet* deque(PacketQueue *q) { 
		return ((NonFifoPacketQueue *)q)->deque(off_cmn_); }
	void remove(PacketQueue *q, Packet *p) {
		((NonFifoPacketQueue *)q)->remove(p, off_cmn_); }
 private:
        int off_ip_;
        int off_tcp_;
};

static class NonFifoDropTailClass : public TclClass {
public:
	NonFifoDropTailClass() : TclClass("Queue/DropTail/NonFifo") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new NonFifoDropTail);
	}
} class_drop_tail_nonfifo;

NonFifoDropTail::NonFifoDropTail()
{
        bind("off_ip_", &off_ip_);
        bind("off_tcp_", &off_tcp_);

	nfq_ = new NonFifoPacketQueue;
	bind_bool("acksfirst_", &((NonFifoPacketQueue *)q())->acksfirst_);
	bind_bool("filteracks_", &((NonFifoPacketQueue *)q())->filteracks_);
	bind_bool("replace_head_", &((NonFifoPacketQueue *)q())->replace_head_);
}
