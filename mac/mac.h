/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
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
 *
 * Contributed by Giao Nguyen, http://daedalus.cs.berkeley.edu/~gnguyen
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac.h,v 1.20 1998/06/25 23:34:13 gnguyen Exp $ (UCB)
 */

#ifndef ns_mac_h
#define ns_mac_h

#include "connector.h"
#include "packet.h"

#define ZERO	0.00000

/*
// Medium Access Control (MAC)
*/

class Classifier;
class Channel;
class Mac;

#define EF_COLLISION 2		// collision error flag
#define BCAST_ADDR -1

enum MacState {
	MAC_IDLE	= 0x0000,
	MAC_POLLING	= 0x0001,
	MAC_RECV 	= 0x0010,
	MAC_SEND 	= 0x0100,
	MAC_RTS		= 0x0200,
	MAC_ACK		= 0x1000,
};

enum MacFrameType {
	MF_BEACON	= 0x0008, // beaconing
	MF_CONTROL	= 0x0010, // used as mask for control frame
	MF_SLOTS	= 0x001a, // announce slots open for contention
	MF_RTS		= 0x001b, // request to send
	MF_CTS		= 0x001c, // clear to send, grant
	MF_ACK		= 0x001d, // acknowledgement
	MF_CF_END	= 0x001e, // contention free period end
	MF_POLL		= 0x001f, // polling
	MF_DATA		= 0x0020, // also used as mask for data frame
	MF_DATA_ACK	= 0x0021, // ack for data frames
};

struct hdr_mac {
	MacFrameType ftype_;	// frame type
	int macSA_;		// source MAC address
	int macDA_;		// destination MAC address
	double txtime_;		// transmission time
	double sstime_;		// slot start time

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_mac* access(Packet* p, int off=-1) {
		return (hdr_mac*) p->access(off < 0 ? offset_ : off);
	}

	inline void set(MacFrameType ft, int sa, int da=-1) {
		ftype_ = ft;
		macSA_ = sa;
		if (da != -1)  macDA_ = da;
	}
	inline MacFrameType& ftype() { return ftype_; }
	inline int& macSA() { return macSA_; }
	inline int& macDA() { return macDA_; }
	inline double& txtime() { return txtime_; }
	inline double& sstime() { return sstime_; }
};


class MacHandlerResume : public Handler {
public:
	MacHandlerResume(Mac* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac* mac_;
};

class MacHandlerSend : public Handler {
public:
	MacHandlerSend(Mac* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac* mac_;
};


class Mac : public Connector {
public:
	Mac();
	virtual void recv(Packet* p, Handler* h);
	virtual void send(Packet* p);
	virtual void resume(Packet* p = 0);

	inline double txtime(Packet* p) {
		hdr_cmn *hdr = (hdr_cmn*)p->access(off_cmn_);
		return (8. * (hdr->size() + hlen_) / bandwidth_);
	}
	inline double txtime(int bytes) {
		return (8. * bytes / bandwidth_);
	}
	inline double bandwidth() const { return bandwidth_; }
	inline int addr() { return addr_; }
	inline MacState state() { return state_; }
	inline MacState state(int m) { return state_ = (MacState) m; }
	inline Mac*& macList() { return macList_; }

protected:
	int command(int argc, const char*const* argv);
	Mac* getPeerMac(Packet* p);
	double delay_;		// MAC overhead
	double bandwidth_;	// MAC bandwidth
	int hlen_;		// MAC header length
	int addr_;		// MAC address
	MacState state_;	// MAC's current state
	Channel* channel_;	// channel this MAC is connected to
	Classifier* mcl_;	// MAC classifier to obtain the peer MAC
	Handler* callback_;	// callback for end-of-transmission
	MacHandlerResume hRes_;	// resume handler
	MacHandlerSend hSend_;	// handle delay send due to busy channel
        Mac* macList_;		// circular list of MACs
	Event intr_;
};

#endif
