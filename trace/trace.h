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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/trace/trace.h,v 1.11 1997/08/12 22:22:46 gnguyen Exp $
 */

#ifndef ns_trace_h
#define ns_trace_h

#include "packet.h"
#include "connector.h"


class Trace : public Connector {
 protected:
        int type_;
        nsaddr_t src_;
        nsaddr_t dst_;
        Tcl_Channel channel_;
        int callback_;
        char wrk_[256];
        void format(int tt, int s, int d, Packet* p);
        void annotate(const char* s);
	int show_tcphdr_;  // bool flags; backward compat
 public:
        Trace(int type);
        ~Trace();
        int command(int argc, const char*const* argv);
        void recv(Packet* p, Handler*);
	void trace(TracedVar*);
        void dump();
        inline char* buffer() { return (wrk_); }

	int off_ip_;
	int off_tcp_;
	int off_rtp_;
};

#endif
