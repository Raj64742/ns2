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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/flags.h,v 1.10 1998/05/20 22:06:37 sfloyd Exp $
 */

/*
 * contains a "flags header", which is common to each packet
 */

#ifndef ns_flags_h
#define ns_flags_h

#include "config.h"
#include "packet.h"

struct hdr_flags {
	unsigned char ecn_;          /* transport receiver notifying
				      *  transport sender of ECN 
				      *  (the ECN Echo bit) */
	unsigned char ecn_to_echo_;  /* ecn to be echoed back in the
					opposite direction (the CE bit) */
	unsigned char eln_;     /* explicit loss notification (snoop) */
	unsigned char fs_;	/* tcp fast start (work in progress --venkat) */
	unsigned char no_ts_;	/* don't use the tstamp of this pkt for rtt */
	unsigned char pri_;	/* unused */
	unsigned char ecn_capable_;  /* an ecn-capable tranport (ECT bit) */
	unsigned char cong_action_;  /* Congestion Action.  Transport 
				      *	sender notifying transport
				      * receiver of responses to
				      * congestion. */
	/*
	 * these functions use the newer ECN names but leaves the actual field
	 * names above to maintain backward compat
	 */
	unsigned char& ect()	{ return ecn_capable_; }
	unsigned char& ecnecho() { return ecn_; }
	unsigned char& ce() { return ecn_to_echo_; }
	unsigned char& cong_action() { return cong_action_; }
};

#endif
