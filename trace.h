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
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/trace.h,v 1.6 1997/05/22 20:55:55 polly Exp $
 */

#ifndef ns_trace_h
#define ns_trace_h

/* a trace header; things in a packet one may want to look at */

#include "config.h"
#include "packet.h"
#include "connector.h"

#define PT_TCP          0
#define PT_TELNET       1
#define PT_CBR          2
#define PT_AUDIO        3
#define PT_VIDEO        4
#define PT_ACK          5
#define PT_START        6
#define PT_STOP         7
#define PT_PRUNE        8
#define PT_GRAFT        9
#define PT_MESSAGE      10
#define PT_RTCP         11
#define PT_RTP          12
#define PT_RTPROTO_DV	13
#define PT_NTYPE        14
#define PT_CtrMcast_Encap 15
#define PT_CtrMcast_Decap 16

#define PT_NAMES "tcp", "telnet", "cbr", "audio", "video", "ack", \
        "start", "stop", "prune", "graft", "message", "rtcp", "rtp", \
	"rtProtoDV", "CtrMcast"

struct hdr_cmn {
	double		ts_;	// timestamp: for q-delay measurement
	int		ptype_;	// packet type (see above)
	int		uid_;	// unique id
	int		size_;	// simulated packet size

	/* per-field member functions */
	int& ptype() {
		return (ptype_);
	}
	int& uid() {
		return (uid_);
	}
	int& size() {
		return (size_);
	}
	double& timestamp() {
		return (ts_);
	}
};

class Trace : public Connector {
 protected:
        int type_;
        nsaddr_t src_;
        nsaddr_t dst_;
        Tcl_Channel channel_;
        int callback_;
        char wrk_[256];
        void format(int tt, int s, int d, Packet* p);
 public:
        Trace(int type);
        ~Trace();
        int command(int argc, const char*const* argv);
        void recv(Packet* p, Handler*);
        void dump();
        inline char* buffer() { return (wrk_); }

	int off_ip_;
	int off_tcp_;
	int off_rtp_;
};

#endif
