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

 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/packet.h,v 1.48 1999/01/04 19:59:03 haldar Exp $ (LBL)

 */

#ifndef ns_packet_h
#define ns_packet_h

#include <config.h>
#include <scheduler.h>
#include <string.h>

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

#define PT_TCP		0
#define PT_UDP		1
#define PT_CBR		2
#define PT_AUDIO	3
#define PT_VIDEO	4
#define PT_ACK		5
#define PT_START	6
#define PT_STOP		7
#define PT_PRUNE	8
#define PT_GRAFT	9
#define PT_GRAFTACK     10
#define PT_JOIN         11
#define PT_ASSERT       12
#define PT_MESSAGE	13
#define PT_RTCP		14
#define PT_RTP		15
#define PT_RTPROTO_DV	16
#define PT_CtrMcast_Encap 17
#define PT_CtrMcast_Decap 18
#define PT_SRM		19
/* simple signalling messages */
#define PT_REQUEST	20
#define PT_ACCEPT	21
#define PT_CONFIRM	22
#define PT_TEARDOWN	23
#define	PT_LIVE		24		// packet from live network
#define PT_REJECT	25

#define PT_TELNET	26		// not needed: telnet use TCP
#define PT_FTP		27
#define PT_PARETO	28
#define PT_EXP		29
#define PT_INVAL	30
#define PT_HTTP		31

/* new encapsulator */
#define PT_ENCAPSULATED 	32
#define PT_MFTP         33
#define PT_NTYPE	34


/* CMU/Monarch's extnsions */
#define PT_ARP		33
#define PT_MAC		34
#define PT_TORA		35
#define PT_DSR		36
#define PT_AODV         37
//
#define PT_NAMES "tcp", "udp", "cbr", "audio", "video", "ack", \
	"start", "stop", "prune", "graft", "graftAck", "join", \
	"assert","message", "rtcp", "rtp", \
	"rtProtoDV", "CtrMcast_Encap", "CtrMcast_Decap", "SRM", \
	"sa_req","sa_accept","sa_conf","sa_teardown", "live", \
	"sa_reject", \
	"telnet", "ftp", "pareto", "exp", "httpInval", "http", \
	"encap", "mftp", "ARP", "MAC", "TORA", "DSR", "AODV"

// Monarch ext
extern char* packet_names[]; /* map PT_* to string name */

#define DATA_PACKET(type) ( (type) == PT_TCP || \
			    (type) == PT_TELNET || \
			    (type) == PT_CBR || \
			    (type) == PT_AUDIO || \
			    (type) == PT_VIDEO || \
			    (type) == PT_ACK \
			    )
//end_monarch


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

        PacketStamp	txinfo;  

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
	int	ptype_;		// packet type (see above)
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
#define AF_LINK 1
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
	inline int& ptype() { return (ptype_); }
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
		free_ = p->next_;
		//p->init();
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
	p->fflag_ = 0;
	p->next_ = NULL;
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

/*void Packet::init()
{
	if (datalen_) {
		delete[] data_;
		// p->data_ = 0;
		datalen_ = 0;
	}
	uid_ = 0;
	time_ = 0;
}*/


inline void Packet::free(Packet* p)
{
	int off_cmn_ = hdr_cmn::offset_;
	hdr_cmn* ch = (hdr_cmn*)p->access(off_cmn_);
	if (p->fflag_ == 0) {
		if (ch->ref_count() == 0) {
			p->next_ = free_;
			free_ = p;
			//init();
			p->fflag_ = 1;
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
#ifdef NS_MOBILE
	p->txinfo.init(&txinfo);
#endif
	return (p);
}

#endif



