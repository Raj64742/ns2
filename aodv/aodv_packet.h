// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/aodv/aodv_packet.h,v 1.2 2000/09/01 03:04:08 haoboy Exp $

#ifndef __aodv_packet_h__
#define __aodv_packet_h__

/* =====================================================================
   Packet Formats...
   ===================================================================== */
#define AODVTYPE_HELLO  0x01
#define AODVTYPE_RREQ   0x02
#define AODVTYPE_RREP   0x04
#define AODVTYPE_UREP   0x08

/*
 * AODV Routing Protocol Header Macros
 */
#define HDR_AODV(p)          ((struct hdr_aodv*)hdr_aodv::access(p))
#define HDR_AODV_REQUEST(p)  ((struct hdr_aodv_request*)hdr_aodv::access(p))
#define HDR_AODV_REPLY(p)    ((struct hdr_aodv_reply*)hdr_aodv::access(p))

/*
 * General AODV Header - shared by all formats
 *
 * XXX Since all three headers share the same header space, it is sufficient 
 * that only hdr_aodv have the header access functions.
 */
struct hdr_aodv {
        u_int8_t        ah_type;
        u_int8_t        ah_reserved[2];
        u_int8_t        ah_hopcount;

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_aodv* access(const Packet* p) {
		return (hdr_aodv*) p->access(offset_);
	}
};

struct hdr_aodv_request {
        u_int8_t        rq_type;                // Packet Type
        u_int8_t        reserved[2];
        u_int8_t        rq_hop_count;           // Hop Count
        u_int32_t       rq_bcast_id;            // Broadcast ID

        nsaddr_t        rq_dst;                 // Destination IP Address
        u_int32_t       rq_dst_seqno;           // Destination Sequence Number
        nsaddr_t        rq_src;                 // Source IP Address
        u_int32_t       rq_src_seqno;           // Source Sequence Number

        double          rq_timestamp;           // when REQUEST sent
};

struct hdr_aodv_reply {
        u_int8_t        rp_type;                // Packet Type
        u_int8_t        rp_flags;
#define RREP_LBIT 0x80
        u_int8_t        reserved;
        u_int8_t        rp_hop_count;           // Hop Count
        nsaddr_t        rp_dst;                 // Destination IP Address
        u_int32_t       rp_dst_seqno;           // Destination Sequence Number
        u_int32_t       rp_lifetime;            // Lifetime

        double          rp_timestamp;           // when corresponding REQ sent
};


#endif /* __aodv_packet_h__ */
