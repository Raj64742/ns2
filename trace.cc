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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/trace.cc,v 1.15 1997/07/14 21:44:17 tomh Exp $ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "rtp.h"
#include "flags.h"
#include "queue.h"

/*
 * tcl command interface
 */
class TraceClass : public TclClass {
public:
	TraceClass() : TclClass("Trace") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new Trace(*argv[4]));
		else
			return NULL;
	}
} trace_class;


Trace::Trace(int type)
	: Connector(), type_(type), src_(0), dst_(0), channel_(0), callback_(0)
{
	bind("src_", (int*)&src_);
	bind("dst_", (int*)&dst_);
	bind("callback_", &callback_);
	bind("show_tcphdr_", &show_tcphdr_);

	bind("off_ip_", &off_ip_);
	bind("off_tcp_", &off_tcp_);
	bind("off_rtp_", &off_rtp_);
}

Trace::~Trace()
{
}

/*
 * $trace detach
 * $trace flush
 * $trace attach $fileID
 */
int Trace::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "detach") == 0) {
			channel_ = 0;
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flush") == 0) {
			Tcl_Flush(channel_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "annotate") == 0) {
			if (channel_ != 0)
				annotate(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attach") == 0) {
			int mode;
			const char* id = argv[2];
			channel_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (channel_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}

void Trace::annotate(const char* s)
{
	sprintf(wrk_, "v %g eval {set sim_annotation {%s}}", 
		Scheduler::instance().clock(), s);
	dump();
}

char* pt_names[] = {
	PT_NAMES
};

// this function should retain some backward-compatibility, so that
// scripts don't break.
void Trace::format(int tt, int s, int d, Packet* p)
{
	hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
	hdr_ip *iph = (hdr_ip*)p->access(off_ip_);
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	hdr_rtp *rh = (hdr_rtp*)p->access(off_rtp_);
	int t = th->ptype();
	const char* name = pt_names[t];

	if (name == 0)
		abort();

	int seqno;
	/* XXX */
		/* CBR's now have seqno's too */
	if (t == PT_RTP || t == PT_CBR)
		seqno = rh->seqno();
	else if (t == PT_TCP || t == PT_ACK)
		seqno = tcph->seqno();
	else
		seqno = -1;

	/*XXX*/
	char flags[5];
	flags[0] = flags[1] = flags[2] = flags[3] = flags[4] = '-';
	hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
	flags[0] = hf->ecn_ ? 'C' : '-';
	flags[1] = hf->pri_ ? 'P' : '-'; 
	flags[2] = hf->usr1_ ? '1' : '-';
	flags[3] = hf->usr2_ ? '2' : '-';
	flags[4] = hf->ecn_to_echo_ ? 'E' : '-';
	flags[5] = 0;

	if (!show_tcphdr_) {
		sprintf(wrk_, "%c %g %d %d %s %d %s %d %d.%d %d.%d %d %d",
			tt,
			Scheduler::instance().clock(),
			s,
			d,
			name,
			th->size(),
			flags,
			iph->flowid() /* was p->class_ */,
			iph->src() >> 8, iph->src() & 0xff,	// XXX
			iph->dst() >> 8, iph->dst() & 0xff,	// XXX
			seqno,
			th->uid() /* was p->uid_ */);
	} else {
		sprintf(wrk_, "%c %g %d %d %s %d %s %d %d.%d %d.%d %d %d %d 0x%x %d",
			tt,
			Scheduler::instance().clock(),
			s,
			d,
			name,
			th->size(),
			flags,
			iph->flowid() /* was p->class_ */,
			iph->src() >> 8, iph->src() & 0xff,	// XXX
			iph->dst() >> 8, iph->dst() & 0xff,	// XXX
			seqno,
			th->uid(), /* was p->uid_ */
			tcph->ackno(),
			tcph->flags(),
			tcph->hlen());
	}
}

void Trace::dump()
{
	if (channel_ != 0) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		int n = strlen(wrk_);
		wrk_[n] = '\n';
		wrk_[n + 1] = 0;
		(void)Tcl_Write(channel_, wrk_, n + 1);
		wrk_[n] = 0;
	}
	if (callback_) {
		Tcl& tcl = Tcl::instance();
		tcl.evalf("%s handle { %s }", name(), wrk_);
	}
}

void Trace::recv(Packet* p, Handler* h)
{
	format(type_, src_, dst_, p);
	dump();
	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
