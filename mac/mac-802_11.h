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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mac/mac-802_11.h,v 1.10 1998/05/06 02:33:25 gnguyen Exp $ (UCB)
 */

#ifndef ns_mac_802_11_h
#define ns_mac_802_11_h

#include "mac-csma.h"


enum MacMode {
	MM_DCF,			// Distributed Coordination Mode
	MM_RTS_CTS,		// DCF with RCS/CTS option
	MM_PCF,			// Point Coordination Mode
};

#define STR2MM(s) (!strcmp(s,"MM_PCF") ? MM_PCF : (!strcmp(s,"MM_DCF") ? MM_DCF : MM_RTS_CTS))


class Mac802_11;

class MacHandlerRts : public Handler {
public:
	MacHandlerRts(Mac802_11* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac802_11* mac_;
};

class MacHandlerData : public Handler {
public:
	MacHandlerData(Mac802_11* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac802_11* mac_;
};
	
class MacHandlerIdle : public Handler {
public:
	MacHandlerIdle(Mac802_11* m) : mac_(m) {}
	void handle(Event*);
protected:
	Mac802_11* mac_;
};
	

class Mac802_11 : public MacCsmaCa {
public:
	Mac802_11();
	virtual void recv(Packet* p, Handler* h);
	virtual void resume(Packet* p = 0);
	virtual void sendRts();
	virtual void sendData();

protected:
	int command(int argc, const char*const* argv);
	virtual void backoff(Handler* h, Packet* p, double delay = 0);
	void transmit(Packet* p, double ifs);
	void RtsCts_recv(Packet* p);
	void RtsCts_send(Packet* p);
	void sendCts(Packet* p);
	void sendAck(Packet* p);
	double lengthNAV(Packet* p);

	MacMode mode_;		// current operating mode
	int bssId_;		// base station identifier
	double sifs_;		// short IFS for RTS, ACK
	double pifs_;		// PCF IFS
	double difs_;		// DCF IFS
	int rtxAck_;		// ACK retransmit counter
	int rtxAckLimit_;	// ACK retransmit limit
	int rtxRts_;		// RTS retransmit counter
	int rtxRtsLimit_;	// RTS retramsmit limit
	int sender_;		// MAC addr for sender of RTS
	Packet* pkt_;		// copy of the data packet
	Packet* pktTx_;		// the pkt being scheduled for transmit
	MacHandlerRts hRts_;	// handle RTS retransmission
	MacHandlerData hData_;	// handle ACK retransmission
	MacHandlerIdle hIdle_;	// timeout of the MAC_RECV state
	Event eIdle_;		// event for the mhIdle_ handler
};

#endif
