/*
 * Copyright (c) 1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 */

#include "queue.h"
#include "ip.h"
#include "tcp.h"
#include "drop-tail.h"

class PacketCompress : public Connector {
public:
	PacketCompress() {
		bind("packetSize_", &packetSize_);
	}
	void recv(Packet* p, Handler* h) {
		((hdr_cmn*)p->access(0))->size() = packetSize_;
		send(p, h);
	}
private:
	int packetSize_;
};

class QueueFilter {
 public:
	QueueFilter() {};
//	int command(int argc, const char*const* argv);
	void filter(PacketQueue *q, Packet *p);
	void compress(Packet *p);
	int compareFlows(hdr_ip *, hdr_ip *);
	int off_ip(int val) { return off_ip_ = val; }
	int off_tcp(int val) { return off_tcp_ = val; }
 private:
	int off_ip_;
	int off_tcp_;
};

class DropTailFilter : public QueueFilter, public DropTail {
 public:
	DropTailFilter();
	int command(int argc, const char*const* argv) {
		return DropTail::command(argc, argv);
//		return QueueFilter::command(argc, argv);
	}
	void recv(Packet *, Handler *);
  private:
	int off_ip_;
	int off_tcp_;
};

class DropTailFilterClass : public TclClass {
 public: 
	DropTailFilterClass() : TclClass("Queue/DropTail/Filter") {}
	TclObject* create(int argc, const char*const* argv) {
                return (new DropTailFilter);
        }
} class_drop_tail_filter;
