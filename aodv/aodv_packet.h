/* -*- c++ -*-
  aodv_packet.h
  $Id: aodv_packet.h,v 1.1 1999/09/30 20:30:08 yaxu Exp $
  */

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
#define HDR_AODV(p)             ((struct hdr_aodv*)(p)->access(off_AODV_))
#define HDR_AODV_REQUEST(p)     ((struct hdr_aodv_request*)(p)->access(off_AODV_))
#define HDR_AODV_REPLY(p)       ((struct hdr_aodv_reply*)(p)->access(off_AODV_))

/*
 * General AODV Header - shared by all formats
 */
struct hdr_aodv {
        u_int8_t        ah_type;
        u_int8_t        ah_reserved[2];
        u_int8_t        ah_hopcount;
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
