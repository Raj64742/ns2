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
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/emulate/arp.cc,v 1.2 1998/05/29 17:57:10 kfall Exp $";
#endif

#include "object.h"
#include "packet.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <memory.h>
#include <stdio.h>
#include <errno.h>

#include "emulate/net.h"
#include "emulate/ether.h"
#include "emulate/internet.h"

//
// arp.cc -- this agent may be inserted into nse as
// an ARP requestor/responder
// 

class ArpAgent : public NsObject, public IOHandler {
public:
	ArpAgent();
protected:
	void	recvpkt();
	void	dispatch(int);
	int	sendreq(in_addr);

	void recv(Packet*, Handler*) { abort(); }

	int command(int, const char*const*);

	Network*	net_;
	ether_header	eh_template_;
	ether_arp	ea_template_;
	ether_addr	my_ether_;
	in_addr		my_ip_;
	int		base_size_;
	char		callback_[80];
	u_char*		rcv_buf_;
};

static class ArpAgentClass : public TclClass { 
public:
        ArpAgentClass() : TclClass("ArpAgent") {}
        TclObject* create(int , const char*const*) {
                return (new ArpAgent());
        } 
} class_arpagent;

ArpAgent::ArpAgent() : net_(NULL)
{
	/* dest addr is broadcast */
	eh_template_.ether_dhost[0] = 0xff;
	eh_template_.ether_dhost[1] = 0xff;
	eh_template_.ether_dhost[2] = 0xff;
	eh_template_.ether_dhost[3] = 0xff;
	eh_template_.ether_dhost[4] = 0xff;
	eh_template_.ether_dhost[5] = 0xff;
	/* src addr is mine */
	memcpy(&eh_template_.ether_shost, &my_ether_, ETHER_ADDR_LEN);
	/* type is ARP */
	eh_template_.ether_type = htons(ETHERTYPE_ARP);

	ea_template_.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	ea_template_.ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	ea_template_.ea_hdr.ar_hln = ETHER_ADDR_LEN;
	ea_template_.ea_hdr.ar_pln = 4;			/* ip addr len */
	ea_template_.ea_hdr.ar_op = htons(ARPOP_REQUEST);	
	memcpy(&ea_template_.arp_sha, &my_ether_, ETHER_ADDR_LEN);	/* sender hw */
	memset(&ea_template_.arp_spa, 0, 4);			/* sender IP */
	memset(&ea_template_.arp_tha, 0, ETHER_ADDR_LEN);	/* target hw */
	memset(&ea_template_.arp_tpa, 0, 4);			/* target hw */
	base_size_ = sizeof(eh_template_) + sizeof(ea_template_);
	rcv_buf_ = new u_char[base_size_];
}

int
ArpAgent::sendreq(in_addr target)
{
	int pktsz = sizeof(eh_template_) + sizeof(ea_template_);
	if (pktsz < 64)
		pktsz = 64;
	u_char* buf = new u_char[pktsz];
	memset(buf, 0, pktsz);

	ether_header* eh = (ether_header*) buf;
	ether_arp* ea = (ether_arp*) (buf + sizeof(eh_template_));
	*eh = eh_template_;	/* set ether header */
	*ea = ea_template_;	/* set ether/IP arp pkt */
	memcpy(ea->arp_tpa, &target, sizeof(target));

	if (net_->send(buf, pktsz) < 0) {
                fprintf(stderr,
                    "ArpAgent(%s): sendpkt (%p, %d): %s\n",
                    name(), buf, pktsz, strerror(errno));
                return (-1);
	}
	delete[] buf;
	return (0);
}

void
ArpAgent::dispatch(int)
{
printf("DISPATCH\n");
	recvpkt();
}

/*
 * receive pkt from network:
 *	note that net->recv() gives us the pkt starting
 *	just BEYOND the frame header
 */
void
ArpAgent::recvpkt()
{
	double ts;
	sockaddr sa;
	int cc = net_->recv(rcv_buf_, base_size_, sa, ts);
	if (cc < (base_size_ - sizeof(ether_header))) {
                fprintf(stderr,
                    "ArpAgent(%s): recv small pkt (%d) [base sz:%d]: %s\n",
                    name(), cc, base_size_, strerror(errno));
		return;
	}
	ether_arp* ea = (ether_arp*) rcv_buf_;
	if (ea->ea_hdr.ar_op != htons(ARPOP_REPLY)) {
                fprintf(stderr,
                    "ArpAgent(%s): NOT REPLY(%hx)\n", name(),
			ntohs(ea->ea_hdr.ar_op));
		return;
	}
	in_addr t;
	memcpy(&t, ea->arp_tpa, 4);	// copy IP address
	
printf("IP %s -> Ether %s\n",
inet_ntoa(t), Ethernet::etheraddr_string(ea->arp_tha));
}

extern "C" {
ether_addr* ether_aton();
}

int
ArpAgent::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "callback") == 0) {
			strncpy(callback_, argv[2], sizeof(callback_));
			return (TCL_OK);
		}
                if (strcmp(argv[1], "network") == 0) { 
                        net_ = (Network *)TclObject::lookup(argv[2]);
                        if (net_ != 0) { 
				link(net_->rchannel(), TCL_READABLE);
				return (TCL_OK);
                        } else {
                                fprintf(stderr,
                                "ArpAgent(%s): unknown network %s\n",
                                    name(), argv[2]);
                                return (TCL_ERROR);
                        }       
                        return(TCL_OK);
                }       
		if (strcmp(argv[1], "myether") == 0) {
			my_ether_ = *(::ether_aton(argv[2]));
			memcpy(&eh_template_.ether_shost, &my_ether_,
				ETHER_ADDR_LEN);
			memcpy(&ea_template_.arp_sha,
				&my_ether_, ETHER_ADDR_LEN);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "myip") == 0) {
			u_long a = inet_addr(argv[2]);
			if (a == 0)
				return (TCL_ERROR);
			in_addr ia;
			ia.s_addr = a;
			my_ip_ = ia;
			memcpy(&ea_template_.arp_spa,
				&my_ip_, 4);
			return (TCL_OK);
		}
	} else if (argc == 4) {
		/* $obj resolve-callback ip-addr callback-fn */
		if (strcmp(argv[1], "resolve-callback") == 0) {
			u_long a = inet_addr(argv[2]);
			in_addr ia;
			ia.s_addr = a;
			sendreq(ia);
			strncpy(callback_, argv[3], sizeof(callback_));
			return (TCL_OK);
		}
	}
	return (NsObject::command(argc, argv));
}
