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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/packet.h,v 1.33 1998/08/12 23:41:10 gnguyen Exp $ (LBL)
 */

#ifndef ns_packet_h
#define ns_packet_h

#include "config.h"
#include "scheduler.h"

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
#define PT_MESSAGE	10
#define PT_RTCP		11
#define PT_RTP		12
#define PT_RTPROTO_DV	13
#define PT_CtrMcast_Encap 14
#define PT_CtrMcast_Decap 15
#define PT_SRM		16
/* simple signalling messages */
#define PT_REQUEST	17
#define PT_ACCEPT	18
#define PT_CONFIRM	19
#define PT_TEARDOWN	20
#define	PT_LIVE		21		// packet from live network
#define PT_REJECT	22

#define PT_TELNET	23		// not needed: telnet use TCP
#define PT_NTYPE	24

#define PT_NAMES "tcp", "udp", "cbr", "audio", "video", "ack", \
	"start", "stop", "prune", "graft", "message", "rtcp", "rtp", \
	"rtProtoDV", "CtrMcast_Encap", "CtrMcast_Decap", "SRM", \
	"sa_req","sa_accept","sa_conf","sa_teardown", "live", "sa_reject", \
	"telnet"

#define OFFSET(type, field)	((int) &((type *)0)->field)

class Packet : public Event {
private:
	unsigned char* bits_;	// header bits
	unsigned char* data_;	// variable size buffer for 'data'
	unsigned int datalen_;	// length of variable size buffer
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
};


struct hdr_cmn {
	int	ptype_;		// packet type (see above)
	int	size_;		// simulated packet size
	int	uid_;		// unique id
	int	error_;		// error flag
	double	ts_;		// timestamp: for q-delay measurement
	int	iface_;		// receiving interface (label)
	int	ref_count_;	// free the pkt until count to 0

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
	inline int& ref_count() { return (ref_count_); }
};


class PacketHeaderClass : public TclClass {
protected:
	PacketHeaderClass(const char* classname, int hdrsize);
	virtual int method(int argc, const char*const* argv);
	void field_offset(const char* fieldname, int offset);
	inline void bind_offset(int* off) { offset_ = off; }
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
	if (p != 0)
		free_ = p->next_;
	else {
		p = new Packet;
		p->bits_ = new unsigned char[hdrlen_];
		if (p == 0 || p->bits_ == 0)
			abort();
//		p->data_ = 0;
//		p->datalen_ = 0;
	}
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

	if (ch->ref_count() == 0) {
		p->next_ = free_;
		free_ = p;
		if (p->datalen_) {
			delete[] p->data_;
//			p->data_ = 0;
			p->datalen_ = 0;
		}
	} else {
		ch->ref_count() = ch->ref_count() - 1;
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
	return (p);
}

#endif
