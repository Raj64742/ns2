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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/ip.h,v 1.1 1997/02/27 04:38:44 kfall Exp $
 */

/* a network layer; basically like IPv6 */
#ifndef ns_ip_h
#define ns_ip_h

#include "config.h"
#include "packet.h"


#define	IP_ECN	0x01	/* ECN bit in flags below (experimental) */
struct bd_ipv6 {
	/* common to IPv{4,6} */
	int		size_;
	nsaddr_t	src_;
	nsaddr_t	dst_;
	int		ttl_;
	/* IPv6 */
	int		fid_;	/* flow id */
	int		prio_;
	/* ns: experimental */
	int		flags_;
};


class IPHeader : public PacketHeader {
private:
	static IPHeader* myaddress_;
	bd_ipv6* hdr_;
public:
	IPHeader() : hdr_(NULL) { }
	inline int hdrsize() { return (sizeof(*hdr_)); }
        inline void header_addr(u_char *base) {
                if (offset_ < 0) abort();
                hdr_ = (bd_ipv6 *) (base + offset_);
        }
        static inline IPHeader* access(u_char *p) {    
                myaddress_->header_addr(p);
                return (myaddress_);
        }
	/* per-field member functions */
	int& size() {
		return (hdr_->size_);
	}
	nsaddr_t& src() {
		return (hdr_->src_);
	}
	nsaddr_t& dst() {
		return (hdr_->dst_);
	}
	int& ttl() {
		return (hdr_->ttl_);
	}
	/* ipv6 fields */
	int& flowid() {
		return (hdr_->fid_);
	}
	int& prio() {
		return (hdr_->prio_);
	}
	/* experimental */
	int& flags() {
		return (hdr_->flags_);
	}
};

#endif
