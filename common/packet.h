/*
 * Copyright (c) 1993 Regents of the University of California.
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/packet.h,v 1.1 1996/12/19 03:22:45 mccanne Exp $ (LBL)
 */

#ifndef ns_packet_h
#define ns_packet_h

#include "config.h"
#include "scheduler.h"

class Trace;

#define PT_TCP		0
#define PT_TELNET	1
#define PT_CBR		2
#define PT_AUDIO	3
#define PT_VIDEO	4
#define PT_ACK		5
#define PT_START	6
#define PT_STOP		7
#define PT_PRUNE	8
#define PT_GRAFT	9
#define PT_MESSAGE	10
#define PT_RTCP		11
#define PT_RTP		12
#define PT_NTYPE	13

#define PT_NAMES "tcp", "telnet", "cbr", "audio", "video", "ack", \
	"start", "stop", "prune", "graft", "message", "rtcp", "rtp"

#define MCASTADDR 0x8000

struct bd_tcp {
#define NSA 3
	double ts_;		/* time packet generated (at source) */
        int sa_start_[NSA+1];	/* selective ack "holes" in packet stream */
        int sa_end_[NSA+1];	/* For each hole i, sa_start[i] contains the
                                 * starting packet sequence no., and sa_end[i]
                                 * contains the ending packet sequence no. */
	int sa_left_[NSA+1];
	int sa_right_[NSA+1];   /* In Jamshid's implementation, this    *
				 * pair of variables represents the blocks*
				 * not the holes.                         */
	int sa_length_;         /* Indicate the number of SACKs in this  *
				 * packet.  Adds 2+sack_length*8 bytes   */
};

/* ivs data packet; ctrl packets are sent back as "messages" */
struct bd_ivs {
	double ts_;		/* timestamp sent at source */
	u_int8_t S_;
	u_int8_t R_;
	u_int8_t state_;
	u_int8_t rshft_;
	u_int8_t kshft_;
	u_int16_t key_;
	double maxrtt_;
};

union body {
	bd_tcp tcp_;
	bd_ivs ivs_;
	char msg_[64];		/* too hard to make this variable length */
};
	
/*
 * A packet.
 */
class Packet : public Event {
 public:
	Packet* copy() const;
	Packet* next_;
	u_int8_t type_;		/* type */
	u_int8_t class_;		/* class for router stats and/or cbq */
#define PF_ECN 0x01	// congestion indication bit
#define PF_PRI 0x02	// two-level priority bit
#define PF_USR1 0x04		/* protocol defined */
#define PF_USR2 0x08		/* protocol defined */
	double qtime_;		/* time put on queue for current hop */
	u_int16_t flags_;	/* misc. flags */
	nsaddr_t src_;		/* source node (for routers) */
	nsaddr_t dst_;		/* destination node (for routers) */
	int size_;		/* size in bytes */
	int seqno_;		/* sequence number */
	int uid_;		/* unique id for this packet */
	union body bd_;		/* type specific fields */
	static Packet* alloc();
	static void free(Packet*);
 protected:
	static Packet* free_;
	static int uidcnt_;
};

inline Packet* Packet::alloc()
{
	Packet* p = free_;
	if (p != 0)
		free_ = p->next_;
	else
		p = new Packet;
	p->uid_ = uidcnt_++;
	return (p);
}

inline void Packet::free(Packet* p)
{
	p->next_ = free_;
	free_ = p;
}

inline Packet* Packet::copy() const
{
	Packet* p = alloc();
	*p = *this;
	return (p);
}

#endif
