/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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

 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/packet.h,v 1.55 1999/03/06 02:26:49 yaxu Exp $ (LBL)

 */

#ifndef ns_packet_h
#define ns_packet_h

#include <config.h>
#include <scheduler.h>
#include <string.h>
#include <assert.h>

#include <object.h>
#include <list.h>
#include <packet-stamp.h>


#define RT_PORT		255	/* port that all route msgs are sent to */
#define HDR_CMN(p)      ((struct hdr_cmn*)(p)->access(hdr_cmn::offset_))
#define HDR_ARP(p)      ((struct hdr_arp*)(p)->access(off_arp_))
#define HDR_MAC(p)      ((struct hdr_mac*)(p)->access(hdr_mac::offset_))
#define HDR_MAC802_11(p) ((struct hdr_mac802_11*)(p)->access(hdr_mac::offset_))
#define HDR_LL(p)       ((struct hdr_ll*)(p)->access(hdr_ll::offset_))
#define HDR_IP(p)       ((struct hdr_ip*)(p)->access(hdr_ip::offset_))
#define HDR_RTP(p)      ((struct hdr_rtp*)(p)->access(hdr_rtp::offset_))
#define HDR_TCP(p)      ((struct hdr_tcp*)(p)->access(hdr_tcp::offset_))
//

enum packet_t {
	PT_TCP,
	PT_UDP,
	PT_CBR,
	PT_AUDIO,
	PT_VIDEO,
	PT_ACK,
	PT_START,
	PT_STOP,
	PT_PRUNE,
	PT_GRAFT,
	PT_GRAFTACK,
	PT_JOIN,
	PT_ASSERT,
	PT_MESSAGE,
	PT_RTCP,
	PT_RTP,
	PT_RTPROTO_DV,
	PT_CtrMcast_Encap,
	PT_CtrMcast_Decap,
	PT_SRM,
	/* simple signalling messages */
	PT_REQUEST,	
	PT_ACCEPT,	
	PT_CONFIRM,	
	PT_TEARDOWN,	
	PT_LIVE,	// packet from live network
	PT_REJECT,

	PT_TELNET,	// not needed: telnet use TCP
	PT_FTP,
	PT_PARETO,
	PT_EXP,
	PT_INVAL,
	PT_HTTP,
	/* new encapsulator */
	PT_ENCAPSULATED,
	PT_MFTP,
	/* CMU/Monarch's extnsions */
	PT_ARP,
	PT_MAC,
	PT_TORA,
	PT_DSR,
	PT_AODV,

	// insert new packet types here

	PT_NTYPE // This MUST be the LAST one
};

class p_info {
public:
	p_info() {
		name_[PT_TCP]= "tcp";
		name_[PT_UDP]= "udp";
		name_[PT_CBR]= "cbr";
		name_[PT_AUDIO]= "audio";
		name_[PT_VIDEO]= "video";
		name_[PT_ACK]= "ack";
		name_[PT_START]= "start";
		name_[PT_STOP]= "stop";
		name_[PT_PRUNE]= "prune";
		name_[PT_GRAFT]= "graft";
		name_[PT_GRAFTACK]= "graftAck";
		name_[PT_JOIN]= "join";
		name_[PT_ASSERT]= "assert";
		name_[PT_MESSAGE]= "message";
		name_[PT_RTCP]= "rtcp";
		name_[PT_RTP]= "rtp";
		name_[PT_RTPROTO_DV]= "rtProtoDV";
		name_[PT_CtrMcast_Encap]= "CtrMcast_Encap";
		name_[PT_CtrMcast_Decap]= "CtrMcast_Decap";
		name_[PT_SRM]= "SRM";

		name_[PT_REQUEST]= "sa_req";	
		name_[PT_ACCEPT]= "sa_accept";
		name_[PT_CONFIRM]= "sa_conf";
		name_[PT_TEARDOWN]= "sa_teardown";
		name_[PT_LIVE]= "live"; 
		name_[PT_REJECT]= "sa_reject";

		name_[PT_TELNET]= "telnet";
		name_[PT_FTP]= "ftp";
		name_[PT_PARETO]= "pareto";
		name_[PT_EXP]= "exp";
		name_[PT_INVAL]= "httpInval";
		name_[PT_HTTP]= "http";
		name_[PT_ENCAPSULATED]= "encap";
		name_[PT_MFTP]= "mftp";
		name_[PT_ARP]= "ARP";
		name_[PT_MAC]= "MAC";
		name_[PT_TORA]= "TORA";
		name_[PT_DSR]= "DSR";
		name_[PT_AODV]= "AODV";
	}
	const char* name(packet_t p) const { 
		if ( p <= PT_NTYPE ) return name_[p];
		return 0;
	}
	static bool data_packet(packet_t type) {
		return ( (type) == PT_TCP || \
			 (type) == PT_TELNET || \
			 (type) == PT_CBR || \
			 (type) == PT_AUDIO || \
			 (type) == PT_VIDEO || \
			 (type) == PT_ACK \
			 );
	}
private:
	static char* name_[PT_NTYPE];
};
extern p_info packet_info; /* map PT_* to string name */
//extern char* p_info::name_[];

#define OFFSET(type, field)	((int) &((type *)0)->field)

//Monarch ext
typedef void (*FailureCallback)(Packet *,void *);
//
class Packet : public Event {
private:
	unsigned char* bits_;	// header bits
	unsigned char* data_;	// variable size buffer for 'data'
	unsigned int datalen_;	// length of variable size buffer
       //void init();            // initialize pkts getting freed.
	bool fflag_;
protected:
	static Packet* free_;	// packet free list
public:
	Packet* next_;		// for queues and the free list
	static int hdrlen_;
	Packet() : bits_(0), datalen_(0), next_(0) { }
	unsigned char* const bits() { return (bits_); }
	Packet* copy() const;
	static Packet* alloc();
	static Packet* alloc(int);
	inline void allocdata(int);
	static void free(Packet*);
	inline unsigned char* access(int off) const {
		if (off < 0)
			abort();
		return (&bits_[off]);
	}
	inline unsigned char* accessdata() const { return data_; }
	inline int datalen() const { return datalen_; }

	//Monarch extn
	// the pkt stamp carries all info about how/where the pkt
        // was sent needed for a receiver to determine if it correctly
        // receives the pkt

        PacketStamp	txinfo_;  

	//monarch extns end;

};

/* 
 * static constant associations between interface special (negative) 
 * values and their c-string representations that are used from tcl
 */
class iface_literal {
public:
	enum iface_constant { 
		UNKN_IFACE= -1, /* iface value for locally originated packets 
				 */
		ANY_IFACE= -2   /* hashnode with iif == ANY_IFACE_                           
				 * matches any pkt iface (imported from TCL);                
				 * this value should be different from hdr_cmn::UNKN_IFACE   
				 * from packet.h                                             
				 */                                                          
	};
	iface_literal(const iface_constant i, const char * const n) : 
		value_(i), name_(n) {}
	inline int value() const { return value_; }
	inline const char * const name() const { return name_; }
private:
	const iface_constant value_;
	const char * const name_; /* strings used in TCL to access those special values */
};

static const iface_literal UNKN_IFACE(iface_literal::UNKN_IFACE, "?");
static const iface_literal ANY_IFACE(iface_literal::ANY_IFACE, "*");


struct hdr_cmn {
	packet_t ptype_;		// packet type (see above)
	int	size_;		// simulated packet size
	int	uid_;		// unique id
	int	error_;		// error flag
	double	ts_;		// timestamp: for q-delay measurement
	int	iface_;		// receiving interface (label)
	int	direction_;	// direction: 0=none, 1=up, -1=down
	int	ref_count_;	// free the pkt until count to 0

	//Monarch extn begins
	nsaddr_t next_hop_;	// next hop for this packet
	int      addr_type_;    // type of next_hop_ addr
#define AF_NONE 0
#define AF_ILINK 1
#define AF_INET 2

        // called if pkt can't obtain media or isn't ack'd. not called if
        // droped by a queue
        FailureCallback xmit_failure_; 
        void *xmit_failure_data_;

        /*
         * MONARCH wants to know if the MAC layer is passing this back because
         * it could not get the RTS through or because it did not receive
         * an ACK.
         */
        int     xmit_reason_;
#define XMIT_REASON_RTS 0x01
#define XMIT_REASON_ACK 0x02

        // filled in by GOD on first transmission, used for trace analysis
        int num_forwards_;	// how many times this pkt was forwarded
        int opt_num_forwards_;   // optimal #forwards
// Monarch extn ends;

	static int offset_;	// offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_cmn* access(Packet* p) {
		return (hdr_cmn*) p->access(offset_);
	}
	
        /* per-field member functions */
	inline packet_t& ptype() { return (ptype_); }
	inline int& size() { return (size_); }
	inline int& uid() { return (uid_); }
	inline int& error() { return error_; }
	inline double& timestamp() { return (ts_); }
	inline int& iface() { return (iface_); }
	inline int& direction() { return (direction_); }
	inline int& ref_count() { return (ref_count_); }
	// monarch_begin
	inline nsaddr_t& next_hop() { return (next_hop_); }
	inline int& addr_type() { return (addr_type_); }
	inline int& num_forwards() { return (num_forwards_); }
	inline int& opt_num_forwards() { return (opt_num_forwards_); }
        //monarch_end
};


class PacketHeaderClass : public TclClass {
protected:
	PacketHeaderClass(const char* classname, int hdrsize);
	virtual int method(int argc, const char*const* argv);
	void field_offset(const char* fieldname, int offset);
	inline void bind_offset(int* off) { offset_ = off; }
	inline void offset(int* off) {offset_= off;}
	int hdrlen_;		// # of bytes for this header
	int* offset_;		// offset for this header
public:
	virtual void bind();
	virtual void export_offsets();
	TclObject* create(int argc, const char*const* argv);
};


inline Packet* Packet::alloc()
{
	Packet* p = free_;
	if (p != 0) {
		assert(p->fflag_ == FALSE);
		free_ = p->next_;
		if (p->datalen_) {
			delete[] p->data_;
			// p->data_ = 0;
			p->datalen_ = 0;
		}
		p->uid_ = 0;
		p->time_ = 0;
	}
	else {
		p = new Packet;
		p->bits_ = new unsigned char[hdrlen_];
		if (p == 0 || p->bits_ == 0)
			abort();
//		p->data_ = 0;
//		p->datalen_ = 0;
		bzero(p->bits_, hdrlen_);
	}
	p->fflag_ = TRUE;
	p->next_ = 0;
	return (p);
}

/* allocate a packet with an n byte data buffer */

inline Packet* Packet::alloc(int n)
{
	Packet* p = alloc();
	if (n > 0) 
		p->allocdata(n);
	return (p);
}

/* allocate an n byte data buffer to an existing packet */

inline void Packet::allocdata(int n)
{
	datalen_ = n;
	data_ = new unsigned char[n];
	if (data_ == 0)
		abort();

}

inline void Packet::free(Packet* p)
{
	int off_cmn_ = hdr_cmn::offset_;
	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	if (p->fflag_) {
		if (ch->ref_count() == 0) {
		        assert(p_uid_<0);
			p->next_ = free_;
			free_ = p;
			//init();
			p->fflag_ = FALSE;
		} else {
			ch->ref_count() = ch->ref_count() - 1;
		}
	}
}

	
	


inline Packet* Packet::copy() const
{
	Packet* p = alloc();
	memcpy(p->bits(), bits_, hdrlen_);
	if (datalen_) {
		p->datalen_ = datalen_;
		p->data_ = new unsigned char[datalen_];
		memcpy(p->data_, data_, datalen_);
	}
	p->txinfo_.init(&txinfo_);
	return (p);
}

#endif



