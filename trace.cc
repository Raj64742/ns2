/* -*-	Mode:C++; c-basic-offset:8; tab-width:8 -*- */
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
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/trace.cc,v 1.55 1998/09/01 01:19:40 tomh Exp $ (LBL)
 */

#include <stdio.h>
#include <stdlib.h>
#include "packet.h"
#include "ip.h"
#include "tcp.h"
#include "rtp.h"
#include "srm.h"
#include "flags.h"
#include "address.h"
#include "trace.h"


class TraceClass : public TclClass {
public:
	TraceClass() : TclClass("Trace") { }
	TclObject* create(int argc, const char*const* argv) {
		if (argc >= 5)
			return (new Trace(*argv[4]));
		return 0;
	}
} trace_class;


Trace::Trace(int type)
	: Connector(), type_(type), src_(0), dst_(0), channel_(0), callback_(0)
#ifdef NAM_TRACE
	, namChan_(0)
#endif
{
	bind("src_", (int*)&src_);
	bind("dst_", (int*)&dst_);
	bind("callback_", &callback_);
	bind("show_tcphdr_", &show_tcphdr_);

#ifdef OFF_HDR
	bind("off_ip_", &off_ip_);
	bind("off_tcp_", &off_tcp_);
	bind("off_rtp_", &off_rtp_);
	bind("off_srm_", &off_srm_);
#endif
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
#ifdef NAM_TRACE
			namChan_ = 0;
#endif
			return (TCL_OK);
		}
		if (strcmp(argv[1], "flush") == 0) {
#ifdef NAM_TRACE
			if (channel_ != 0) 
				Tcl_Flush(channel_);
			if (namChan_ != 0)
				Tcl_Flush(namChan_);
#else
			Tcl_Flush(channel_);
#endif
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
#ifdef NAM_TRACE
		if (strcmp(argv[1], "namattach") == 0) {
			int mode;
			const char* id = argv[2];
			namChan_ = Tcl_GetChannel(tcl.interp(), (char*)id,
						  &mode);
			if (namChan_ == 0) {
				tcl.resultf("trace: can't attach %s for writing", id);
				return (TCL_ERROR);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ntrace") == 0) {
			if (namChan_ != 0) 
				write_nam_trace(argv[2]);
			return (TCL_OK);
		}
#endif
	}
	return (Connector::command(argc, argv));
}

#ifdef NAM_TRACE
void Trace::write_nam_trace(const char *s)
{
	sprintf(nwrk_, "%s", s);
	namdump();
}
#endif

void Trace::annotate(const char* s)
{
	sprintf(wrk_, "v %g eval {set sim_annotation {%s}}", 
		Scheduler::instance().clock(), s);
	dump();
	sprintf(nwrk_, "v -t %.17g sim_annotation %g %s", 
		Scheduler::instance().clock(), 
		Scheduler::instance().clock(), s);
	namdump();
}

char* pt_names[] = {
	PT_NAMES
};

char* srm_names[] = {
        SRM_NAMES
};

// this function should retain some backward-compatibility, so that
// scripts don't break.
void Trace::format(int tt, int s, int d, Packet* p)
{
#ifdef OFF_HDR
	hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
	hdr_ip *iph = (hdr_ip*)p->access(off_ip_);
	hdr_tcp *tcph = (hdr_tcp*)p->access(off_tcp_);
	hdr_rtp *rh = (hdr_rtp*)p->access(off_rtp_);
	hdr_srm *sh = (hdr_srm*)p->access(off_srm_); 
#else
	hdr_cmn *th = hdr_cmn::access(p);
	hdr_ip *iph = hdr_ip::access(p);
	hdr_tcp *tcph = hdr_tcp::access(p);
	hdr_rtp *rh = hdr_rtp::access(p);
	hdr_srm *sh = hdr_srm::access(p); 
#endif
	const char* sname = "null";

	int t = th->ptype();
	const char* name = pt_names[t];

        /* SRM-specific */
	if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
            if ( sh->type() < 5 && sh->type() > 0 ) {
	        sname = srm_names[sh->type()];
	    }
	}

	if (name == 0)
		abort();

	int seqno;
	/* XXX */
	/* UDP's now have seqno's too */
	if (t == PT_RTP || t == PT_CBR || t == PT_UDP || t == PT_EXP ||
	    t == PT_PARETO)
		seqno = rh->seqno();
	else if (t == PT_TCP || t == PT_ACK || t == PT_HTTP || t == PT_FTP ||
	    t == PT_TELNET)
		seqno = tcph->seqno();
	else
		seqno = -1;
        /* 
         * When new flags are added, make sure to change NUMFLAGS
         * in trace.h
         */
        char flags[NUMFLAGS+1];
        for (int i = 0; i < NUMFLAGS; i++)
		flags[i] = '-';
        flags[NUMFLAGS] = 0;

#ifdef OFF_HDR
	hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
#else
	hdr_flags* hf = hdr_flags::access(p);
#endif
	flags[0] = hf->ecn_ ? 'C' : '-';          // Ecn Echo
	flags[1] = hf->pri_ ? 'P' : '-'; 
	flags[2] = '-';
	flags[3] = hf->cong_action_ ? 'A' : '-';   // Congestion Action
	flags[4] = hf->ecn_to_echo_ ? 'E' : '-';   // Congestion Experienced
	flags[5] = hf->fs_ ? 'F' : '-';
	flags[6] = hf->ecn_capable_ ? 'N' : '-';
	
#ifdef notdef
	flags[1] = (iph->flags() & PF_PRI) ? 'P' : '-';
	flags[2] = (iph->flags() & PF_USR1) ? '1' : '-';
	flags[3] = (iph->flags() & PF_USR2) ? '2' : '-';
	flags[5] = 0;
#endif
	char *src_nodeaddr = Address::instance().print_nodeaddr(iph->src());
	char *src_portaddr = Address::instance().print_portaddr(iph->src());
	char *dst_nodeaddr = Address::instance().print_nodeaddr(iph->dst());
	char *dst_portaddr = Address::instance().print_portaddr(iph->dst());

	if (!show_tcphdr_) {
		sprintf(wrk_, "%c %g %d %d %s %d %s %d %s%s %s%s %d %d",
			tt,
			Scheduler::instance().clock(),
			s,
			d,
			name,
			th->size(),
			flags,
			iph->flowid() /* was p->class_ */,
			// iph->src() >> (Address::instance().NodeShift_[1]), 
                        // iph->src() & (Address::instance().PortMask_), 
                        // iph->dst() >> (Address::instance().NodeShift_[1]), 
                        // iph->dst() & (Address::instance().PortMask_),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,
			th->uid() /* was p->uid_ */);
	} else {
		sprintf(wrk_, 
			"%c %g %d %d %s %d %s %d %s%s %s%s %d %d %d 0x%x %d %d",
			tt,
			Scheduler::instance().clock(),
			s,
			d,
			name,
			th->size(),
			flags,
			iph->flowid(), /* was p->class_ */
		        // iph->src() >> (Address::instance().NodeShift_[1]), 
			// iph->src() & (Address::instance().PortMask_), 
  		        // iph->dst() >> (Address::instance().NodeShift_[1]), 
  		        // iph->dst() & (Address::instance().PortMask_),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,
			th->uid(), /* was p->uid_ */
			tcph->ackno(),
			tcph->flags(),
			tcph->hlen(),
			tcph->sa_length());
	}
#ifdef NAM_TRACE
	if (namChan_ != 0)
		sprintf(nwrk_, 
			"%c -t %g -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s%s %s%s %d %s %s}",
			tt,
			Scheduler::instance().clock(),
			s,
 			d,
			name,
			th->size(),
			iph->flowid(),
			th->uid(),
			iph->flowid(),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			seqno,flags,sname);
#endif      
	delete [] src_nodeaddr;
  	delete [] src_portaddr;
  	delete [] dst_nodeaddr;
   	delete [] dst_portaddr;
}

void Trace::dump()
{
	int n = strlen(wrk_);
	if ((n > 0) && (channel_ != 0)) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
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

#ifdef NAM_TRACE
void Trace::namdump()
{
	int n = 0;

	/* Otherwise nwrk_ isn't initialized */
	if (namChan_ != 0)
		n = strlen(nwrk_);
	if ((n > 0) && (namChan_ != 0)) {
		/*
		 * tack on a newline (temporarily) instead
		 * of doing two writes
		 */
		nwrk_[n] = '\n';
		nwrk_[n + 1] = 0;
		(void)Tcl_Write(namChan_, nwrk_, n + 1);
		nwrk_[n] = 0;
	}
}
#endif

void Trace::recv(Packet* p, Handler* h)
{
	format(type_, src_, dst_, p);
	dump();
	namdump();
	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}


void Trace::trace(TracedVar* var)
{
	char tmp[256] = "";
	Scheduler& s = Scheduler::instance();
	if (&s == 0)
		return;

	// format: use Mark's nam feature code without the '-' prefix
	sprintf(wrk_, "%c t%g a%s n%s v%s",
		type_,
		s.clock(),
		var->owner()->name(),
		var->name(),
		var->value(tmp));
	dump();
}

//
// we need a DequeTraceClass here because a 'h' event need to go together
// with the '-' event. It's possible to use a postprocessing script, but 
// seems that's inconvient.
//
static class DequeTraceClass : public TclClass {
public:
	DequeTraceClass() : TclClass("Trace/Deque") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new DequeTrace(*argv[4]));
		return NULL;
	}
} dequetrace_class;


DequeTrace::~DequeTrace()
{
}

void 
DequeTrace::recv(Packet* p, Handler* h)
{
	// write the '-' event first
	format(type_, src_, dst_, p);
	dump();
	namdump();

#ifdef NAM_TRACE
	if (namChan_ != 0) {
#ifdef OFF_HDR
		hdr_cmn *th = (hdr_cmn*)p->access(off_cmn_);
		hdr_ip *iph = (hdr_ip*)p->access(off_ip_);
		hdr_srm *sh = (hdr_srm*)p->access(off_srm_);
#else
		hdr_cmn *th = hdr_cmn::access(p);
		hdr_ip *iph = hdr_ip::access(p);
		hdr_srm *sh = hdr_srm::access(p);
#endif
		const char* sname = "null";   

		int t = th->ptype();
		const char* name = pt_names[t];
		
		if (strcmp(name,"SRM") == 0 || strcmp(name,"cbr") == 0 || strcmp(name,"udp") == 0) {
		    if ( sh->type() < 5 && sh->type() > 0  ) {
		        sname = srm_names[sh->type()];
		    }
		}   

		char *src_nodeaddr = Address::instance().print_nodeaddr(iph->src());
		char *src_portaddr = Address::instance().print_portaddr(iph->src());
		char *dst_nodeaddr = Address::instance().print_nodeaddr(iph->dst());
		char *dst_portaddr = Address::instance().print_portaddr(iph->dst());

		char flags[NUMFLAGS+1];
		for (int i = 0; i < NUMFLAGS; i++)
			flags[i] = '-';
		flags[NUMFLAGS] = 0;

#ifdef OFF_HDR
		hdr_flags* hf = (hdr_flags*)p->access(off_flags_);
#else
		hdr_flags* hf = hdr_flags::access(p);
#endif
		flags[0] = hf->ecn_ ? 'C' : '-';          // Ecn Echo
		flags[1] = hf->pri_ ? 'P' : '-'; 
		flags[2] = '-';
		flags[3] = hf->cong_action_ ? 'A' : '-';   // Congestion Action
		flags[4] = hf->ecn_to_echo_ ? 'E' : '-';   // Congestion Experienced
		flags[5] = hf->fs_ ? 'F' : '-';
		flags[6] = hf->ecn_capable_ ? 'N' : '-';
	
#ifdef notdef
		flags[1] = (iph->flags() & PF_PRI) ? 'P' : '-';
		flags[2] = (iph->flags() & PF_USR1) ? '1' : '-';
		flags[3] = (iph->flags() & PF_USR2) ? '2' : '-';
		flags[5] = 0;
#endif
		
		sprintf(nwrk_, 
			"%c -t %g -s %d -d %d -p %s -e %d -c %d -i %d -a %d -x {%s%s %s%s %d %s %s}",
			'h',
			Scheduler::instance().clock(),
			src_,
  			dst_,
			name,
			th->size(),
			iph->flowid(),
			th->uid(),
			iph->flowid(),
			src_nodeaddr,
			src_portaddr,
			dst_nodeaddr,
			dst_portaddr,
			-1, flags, sname);
		namdump();
		delete [] src_nodeaddr;
		delete [] src_portaddr;
		delete [] dst_nodeaddr;
		delete [] dst_portaddr;
	}
#endif

	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
