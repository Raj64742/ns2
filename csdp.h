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
 *	Group at the University of California Berkeley.
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

#ifndef ns_csdp_h
#define ns_csdp_h

#include "queue.h"


class ErrorModel;

class IdPacketQueue : public PacketQueue {
public:
	IdPacketQueue() : id_(0), loss_(0), total_(0), em_(0) {}
	inline int& id() { return id_; }
	inline int& loss() { return loss_; }
	inline int& total() { return total_; }
	inline ErrorModel*& em() { return em_; }
protected:
	int id_;		// unique identifier for this queue
	int loss_;
	int total_;
	ErrorModel* em_;
};


class Csdp : public Queue {
public:
	Csdp();

protected:
	int command(int argc, const char*const* argv);
	void enque(Packet*);
	Packet* deque();
	void enque(Packet*, IdPacketQueue*);
	Packet* deque(IdPacketQueue*);
	virtual double weight(IdPacketQueue*);
	IdPacketQueue* getQueue(int id);

	IdPacketQueue** q_;
	int numq_;
	int maxq_;
	int qlen_;
	double totalweight_;
	int gqid_;
	int bqid_;
	int off_ll_;
	int off_mac_;
};

#endif
