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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory and the Daedalus
 *	research group at UC Berkeley.
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
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 */

#ifndef ns_channel_h
#define ns_channel_h

#include "connector-drop.h"


class Trace;

/*
// Channel:  a shared medium that supports contention and collision
*/

class Channel : public DropConnector {
public:
	Channel();
	void recv(Packet* p, Handler*);
	virtual int send(Packet* p, double txtime);
	virtual void contention(Packet*, Handler*); // content for the channel
	int hold(double txtime);
	virtual int collision() { return numtx_ > 1; }
	virtual double txstop() { return txstop_; }
	Packet* pkt() { return pkt_; }

protected:
	int command(int argc, const char*const* argv);
	double delay_;		// channel delay, for collision interval
	int off_ll_;		// link-layer header offset
	double txstop_;		// end of the last transmission
	double cwstop_;		// end of the contention window
	int numtx_;		// number of transmissions during contention
	Packet* pkt_;		// packet current transmitted on the channel
	Trace* trace_;
};


class DuplexChannel : public Channel {
public:
	DuplexChannel() : Channel() {}
	int send(Packet* p, double txtime);
	void contention(Packet*, Handler*); // content for the channel
	inline double txstop() { return 0; }
};
	
#endif
