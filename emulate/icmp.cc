/*    
 * Copyright (c) 1998 Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/icmp.cc,v 1.1 1998/05/30 01:35:52 kfall Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include "agent.h"
#include "scheduler.h"
#include "emulate/net.h"
#include "emulate/internet.h"

//
// icmp.cc -- a very limited-functionality set of icmp routines
// 

class IcmpAgent : public Agent {
public:
	IcmpAgent();
	void recv(Packet*, Handler*) { abort(); }
protected:
	void	sendredirect(in_addr&, in_addr&, in_addr&);
	void	makeip(ip*, u_short, in_addr&, in_addr&);
	void	makeicmp(ip*);
	u_char	buf_[64];

	int	command(int argc, const char*const* argv);

	Network* net_;
	int	ttl_;
	int	icmptype_;
	int	icmpcode_;

	in_addr	gwaddr_;	// gw addr for redirects
};

static class IcmpAgentClass : public TclClass { 
public:
        IcmpAgentClass() : TclClass("Agent/IcmpAgent") {}
        TclObject* create(int , const char*const*) {
                return (new IcmpAgent());
        } 
} class_icmpsagent;

IcmpAgent::IcmpAgent() : Agent(PT_LIVE)
{
	bind("ttl_", &ttl_);
	bind("icmptype_", &icmptype_);
	bind("icmpcode_", &icmpcode_);
}

/*
 * sendpkt()
 */

void
IcmpAgent::sendredirect(in_addr& src, in_addr& dst, in_addr& gw)
{
	int cc = 0;
	ip* iph = (ip*) buf_;
	makeip(iph, 28, src, dst);
	gwaddr_ = gw;
	makeicmp(iph);
	if (net_) {
		cc = net_->send(buf_, 28);
		if (cc < 0) {
			fprintf(stderr, 
				"IcmpAgent(%s): couldn't send buffer\n",
				name());
			return;
		}
	} else {
		fprintf(stderr, 
			"IcmpAgent(%s): no network!\n",
			name());
		return;
	}
	return;
}

/*
 * cons up a basic-looking ip header, no options
 */
void
IcmpAgent::makeip(ip* iph, u_short len, in_addr& src, in_addr& dst)
{
	u_char *p = (u_char*) iph;
	*p = 0x45;	/* ver + hl */
	iph->ip_tos = 0;
	iph->ip_len = htons(len);
	iph->ip_id = (u_short) Scheduler::instance().clock();	// why not?
	iph->ip_off = 0x0000;	// mf and df bits off, offset zero
	iph->ip_ttl = ttl_;
	iph->ip_p = IPPROTO_ICMP;
	memcpy(&iph->ip_src, &src, 4);
	memcpy(&iph->ip_dst, &dst, 4);
	iph->ip_sum = Internet::in_cksum((u_short*) iph, 20);
	return;
}

void
IcmpAgent::makeicmp(ip* iph)
{
	icmp* icp = (icmp*) (iph + 1);	// assumes no options
	icp->icmp_type = icmptype_;
	icp->icmp_code = icmpcode_;

	switch (icmptype_) {
	case ICMP_REDIRECT:
		{
		    in_addr* gwaddr = &icp->icmp_gwaddr;
		    *gwaddr = gwaddr_;
		    icp->icmp_cksum = 0;
		    icp->icmp_cksum = Internet::in_cksum(
			(u_short*) icp, 8);
		}
		break;
	default:
		fprintf(stderr, "IcmpAgent(%s): type %d not supported\n",
			icmptype_);
		break;
	}
	return;
}

int
IcmpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
                if (strcmp(argv[1], "network") == 0) { 
                        net_ = (Network *)TclObject::lookup(argv[2]);
			// for now, we are not an IOHandler (don't recv)
                        if (net_ == 0) {
                                fprintf(stderr,
                                "IcmpAgent(%s): unknown network %s\n",
                                    name(), argv[2]);
                                return (TCL_ERROR);
                        }       
                        return(TCL_OK);
                }       
	} else if (argc > 5) {
		// $obj send name src dst [...stuff...]
		if (strcmp(argv[1], "send") == 0) {
			if (strcmp(argv[2], "redirect") == 0 &&
			    argc == 6) {
				// $obj send redirect src dst gwaddr
				u_long s, d, g;
				s = inet_addr(argv[3]);
				d = inet_addr(argv[4]);
				g = inet_addr(argv[5]);
				in_addr src, dst, gw;
				src.s_addr = s;
				dst.s_addr = d;
				gw.s_addr = g;
				sendredirect(src, dst, gw);
				return (TCL_OK);
			}
		}
	}
	return (Agent::command(argc, argv));
}
