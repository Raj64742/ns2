/*
 * Copyright (c) 1996 Regents of the University of California.
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
 */

#ifndef ns_mac_csma_h
#define ns_mac_csma_h

#include "mac.h"


class CsmaMac;

class MacHandlerEoc : public Handler {
public:
	MacHandlerEoc(CsmaMac& m) : mac_(m) {}
	void handle(Event* e);
protected:
	CsmaMac& mac_;
};


class CsmaMac : public Mac {
public:
	CsmaMac();
	virtual void send(Packet*);
	virtual void backoff(Packet*, double delay=0);
	virtual void endofContention(Packet*);

protected:
	int cwmin_;		// minimum contention window for 1st backoff
	int cwmax_;		// maximum contention window (backoff range)
	int cw_;		// current contention window
	double slotTime_;	// duration of antenna slot in seconds
	int attempt_;		// number of access attempts for 1 frame
	int maxAttempt_;	// maximum number of attempts
	MacHandlerEoc mhEoc_;	// handle end-of-contention
};


class CsmaCdMac : public CsmaMac
{
public:
	CsmaCdMac() : CsmaMac() {}
	void endofContention(Packet*);
};


class CsmaCaMac : public CsmaMac
{
public:
	CsmaCaMac() : CsmaMac() {}
	void recv(Packet*, Handler* h);
};

#endif
