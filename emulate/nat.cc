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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/nat.cc,v 1.3 1998/12/01 06:49:49 kfall Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "agent.h"
#include "scheduler.h"
#include "packet.h"
#include "ip.h"
#include "emulate/net.h"
#include "emulate/internet.h"

//
// Nat -- a limited-functionality TCP address rewriting
// facility for emulation mode.
// 

class NatAgent : public Agent {
public:
	NatAgent() : Agent(PT_LIVE) { }
	void recv(Packet*, Handler*);
protected:
	virtual void	rewrite_addr(ip*) = 0;
	u_short		addrsum(in_addr*);
	u_short		addrsum(in_addr*, in_addr*);
	void		fixtcpudpcksums(ip*, int);
	void	nat(Packet*);
	virtual u_short	newval() = 0;
	virtual u_short oldval(ip*) = 0;
	virtual void	fixcksums(ip*, int);
	int	command(int argc, const char*const* argv);
};

class TCPDestNat : public virtual NatAgent {
protected:
	void	rewrite_addr(ip*);
	void fixcksums(ip* iph, int iphlen) {
		fixtcpudpcksums(iph, iphlen);
	}
	u_short newval();
	u_short oldval(ip*);
	int	command(int argc, const char*const* argv);
	in_addr	newdst_;
};

class TCPSrcNat : public virtual NatAgent {
protected:
	void	rewrite_addr(ip*);
	void fixcksums(ip* iph, int iphlen) {
		fixtcpudpcksums(iph, iphlen);
	}
	u_short newval();
	u_short oldval(ip*);
	int	command(int argc, const char*const* argv);
	in_addr	newsrc_;
};

class TCPSrcDestNat : public TCPDestNat, public TCPSrcNat {
protected:
	void	rewrite_addr(ip*);
	u_short newval();
	u_short oldval(ip*);
	void fixcksums(ip* iph, int iphlen) {
		fixtcpudpcksums(iph, iphlen);
	}
	int	command(int argc, const char*const* argv);
};

static class NatTCPSrcAgentClass : public TclClass { 
public:
        NatTCPSrcAgentClass() : TclClass("Agent/NatAgent/TCPSrc") {}
        TclObject* create(int , const char*const*) {
                return (new TCPSrcNat());
        } 
} class_tcpsrcnat;

static class NatTCPDestAgentClass : public TclClass { 
public:
        NatTCPDestAgentClass() : TclClass("Agent/NatAgent/TCPDest") {}
        TclObject* create(int , const char*const*) {
                return (new TCPDestNat());
        } 
} class_tcpdstnat;

static class NatTCPSrcDestAgentClass : public TclClass { 
public:
        NatTCPSrcDestAgentClass() : TclClass("Agent/NatAgent/TCPSrcDest") {}
        TclObject* create(int , const char*const*) {
                return (new TCPSrcDestNat());
        } 
} class_tcpsrcdstnat;

void
NatAgent::recv(Packet *pkt, Handler *)
{
	nat(pkt);
	// we are merely rewriting an already-existing
	// packet (which was destined for us), so be
	// sure to rewrite the simulator's notion of the
	// address, otherwise we just keep sending to ourselves
	// (ouch).
	hdr_ip* iph = hdr_ip::access(pkt);
	iph->src() = addr_;
	iph->dst() = dst_;
	send(pkt, 0);
}

/*
 * NatAgent base class: fix only IP-layer checksums
 */

void
NatAgent::fixcksums(ip* iph, int iphlen)
{
	// fix IP cksum
	iph->ip_sum = 0;
	iph->ip_sum = Internet::in_cksum((u_short*) iph, iphlen);
	return;
}

/*
 * rewrite packet addresses, calls object-specific rewrite_addr() function
 */

void
NatAgent::nat(Packet* pkt)
{
        hdr_cmn* hc = (hdr_cmn*)pkt->access(off_cmn_);
        ip* iph = (ip*) pkt->accessdata();
	if (pkt->datalen() < hc->size()) {
		fprintf(stderr,
		    "NatAgent(%s): recvd packet with pkt sz %d but bsize %d\n",
			name(), hc->size(), pkt->datalen());
		return;
	}
	int iphlen = (((u_char*)iph)[0] & 0x0f) << 2;
	rewrite_addr(iph);
	fixcksums(iph, iphlen);
}

/*
 * functions to compute 1's complement sum of 1 and 2 IP addresses
 * (note: only the sum, not the complement of the sum)
 */
u_short
NatAgent::addrsum(in_addr* ia)
{
	u_short* p = (u_short*) ia;
	u_short sum = 0;

	sum += *p++;
	sum += *p;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (sum);
}

u_short
NatAgent::addrsum(in_addr* ia1, in_addr* ia2)
{
	u_short* p = (u_short*) ia1;
	u_short sum = 0;

	sum += *p++;
	sum += *p;

	p = (u_short*) ia2;
	sum += *p++;
	sum += *p;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (sum);
}

/*
 * incrementally update tcp or udp packet for new addresses:
 *	rewrite IP addresses and recompute IP header cksum
 *	recompute TCP or UDP pseudoheader checksum
 */
void
NatAgent::fixtcpudpcksums(ip* iph, int iphlen)
{
	NatAgent::fixcksums(iph, iphlen);

	tcphdr* tcph = (tcphdr*)(((u_char*) iph) + iphlen);
	u_short sum = tcph->th_sum;
	u_long nsum = ~sum + ~oldval(iph) + newval();
	nsum = (nsum >> 16) + (nsum & 0xffff);
	nsum += (nsum >> 16);
	sum = ~nsum;
	tcph->th_sum = sum;
	return;
}


void
TCPSrcNat::rewrite_addr(ip* iph)
{
	iph->ip_src = newsrc_;
}

u_short
TCPSrcNat::newval()
{
	return (addrsum(&newsrc_));
}

u_short
TCPSrcNat::oldval(ip* iph)
{
	return (addrsum(&iph->ip_src));
}

u_short
TCPDestNat::newval()
{
	return (addrsum(&newdst_));
}

u_short
TCPDestNat::oldval(ip* iph)
{
	return (addrsum(&iph->ip_dst));
}

void
TCPDestNat::rewrite_addr(ip* iph)
{
	iph->ip_dst = newdst_;
}

void
TCPSrcDestNat::rewrite_addr(ip* iph)
{
	TCPSrcNat::rewrite_addr(iph);
	TCPDestNat::rewrite_addr(iph);
}

u_short
TCPSrcDestNat::newval()
{
	return(addrsum(&newsrc_, &newdst_));
}

u_short
TCPSrcDestNat::oldval(ip* iph)
{
	return(addrsum(&iph->ip_src, &iph->ip_dst));
}

int
NatAgent::command(int argc, const char*const* argv)
{
	return(Agent::command(argc, argv));
}

int
TCPSrcNat::command(int argc, const char*const* argv)
{
	// $srcnat source <ipaddr>
	if (argc == 3) {
		if (strcmp(argv[1], "source") == 0) {
			u_long ns;
			ns = inet_addr(argv[2]);
			newsrc_.s_addr = ns;
			return (TCL_OK);
		}
	}

	return (NatAgent::command(argc, argv));
}

int
TCPDestNat::command(int argc, const char*const* argv)
{
	// $srcnat destination <ipaddr>
	if (argc == 3) {
		if (strcmp(argv[1], "destination") == 0) {
			u_long nd;
			nd = inet_addr(argv[2]);
			newdst_.s_addr = nd;
			return (TCL_OK);
		}
	}
	return (NatAgent::command(argc, argv));
}

int
TCPSrcDestNat::command(int argc, const char*const* argv)
{
	// $srcnat source <ipaddr>
	if (argc == 3) {
		if (strcmp(argv[1], "source") == 0) {
			u_long ns;
			ns = inet_addr(argv[2]);
			newsrc_.s_addr = ns;
			return (TCL_OK);
		}
	}

	// $srcnat destination <ipaddr>
	if (argc == 3) {
		if (strcmp(argv[1], "destination") == 0) {
			u_long nd;
			nd = inet_addr(argv[2]);
			newdst_.s_addr = nd;
			return (TCL_OK);
		}
	}

	return (NatAgent::command(argc, argv));
}
