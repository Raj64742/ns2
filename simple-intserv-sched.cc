/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or
 * derivative work. Xerox grants no other licenses expressed or
 * implied. The Xerox trade name should not be used in any advertising
 * without its written permission. 
 *
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * software. 
 */

/*
 * Copyright (c) 1994 Regents of the University of California.
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
static const char rcsid[] =
    "@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/simple-intserv-sched.cc,v 1.5 1999/09/24 17:04:38 heideman Exp $ (LBL)";
#endif


//Simple scheduler with 2 service priority levels and protects signalling
//ctrl  packets


#include "config.h"
#include "queue.h"

#define CLASSES 2

class SimpleIntServ : public Queue {
public:
	SimpleIntServ() {
		int i;
		char buf[10];
		for (i=0;i<CLASSES;i++) {
			q_[i] = new PacketQueue;
			qlimit_[i] = 0;
			sprintf(buf,"qlimit%d_",i);
			bind(buf,&qlimit_[i]);
		}
		bind("off_ip_",&off_ip_);
	}
	protected :
	void enque(Packet *);
	Packet *deque();
	PacketQueue *q_[CLASSES];
	int qlimit_[CLASSES];
	int off_ip_;
	
};


static class SimpleIntServClass : public TclClass {
public:
	SimpleIntServClass() : TclClass("Queue/SimpleIntServ") {}
	TclObject* create(int, const char*const*) {
		return (new SimpleIntServ);
	}
} class_simple_intserv;

void SimpleIntServ::enque(Packet* p)
{
	
	hdr_ip* iph=(hdr_ip*)p->access(off_ip_);
	int cl=(iph->flowid()) ? 1:0;
	
	if (q_[cl]->length() >= (qlimit_[cl]-1)) {
		hdr_cmn* ch=(hdr_cmn*)p->access(off_cmn_);
		packet_t ptype = ch->ptype();
		if ( (ptype != PT_REQUEST) && (ptype != PT_REJECT) && (ptype != PT_ACCEPT) && (ptype != PT_CONFIRM) && (ptype != PT_TEARDOWN) ) {
			drop(p);
		}
		else {
			q_[cl]->enque(p);
		}
	}
	else {
		q_[cl]->enque(p);
	}
}


Packet *SimpleIntServ::deque()
{
	int i;
	for (i=CLASSES-1;i>=0;i--)
		if (q_[i]->length())
			return q_[i]->deque();
	return 0;
}

