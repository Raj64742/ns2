/* -*- c++ -*-
   $Id: tora_packet.h,v 1.1 1999/08/03 04:07:15 yaxu Exp $
   
   basic formats, constants and datatypes for TORA pkts.
   */
#ifndef __tora_packet_h__
#define __tora_packet_h__

#include <config.h>

/*
 * TORA Routing Protocol Header Macros
 */
#define HDR_TORA_QRY(p)         ((struct hdr_tora_qry*)(p)->access(off_TORA_))
#define HDR_TORA_UPD(p)         ((struct hdr_tora_upd*)(p)->access(off_TORA_))
#define HDR_TORA_CLR(p)         ((struct hdr_tora_clr*)(p)->access(off_TORA_))

// why do we have Height defined here?  seems like there oughta be a better
// place. -dam 9/30/98
class Height {
public:

	Height(int ID) : tau(-1.0), oid(-1), r(-1), delta(-1), id(ID) {}

	/*
	 * Status Functions.
	 */
	inline int isZero() {
		return (tau == 0.0 && oid == 0 && r == 0 && delta == 0);
	}
	inline int isNull() {
		return (tau == -1.0 && oid == -1 && r == -1 && delta == -1);
	}
	inline int compare(Height *h) {
		if(h->tau > tau) return 1;     if(tau > h->tau) return -1;
		if(h->oid > oid) return 1;     if(oid > h->oid) return -1;
		if(h->r > r) return 1;         if(r > h->r) return -1;
		if(h->delta > delta) return 1; if(delta > h->delta) return -1;
		if(h->id > id) return 1;       if(id > h->id) return -1;

		return 0;               // heights equal
	}

	/*
	 * Update Functions
	 */
	inline void Zero() { tau = 0.0; oid = 0; r = 0; delta = 0; }

	inline void Null() { tau = -1.0; oid = -1; r = -1; delta = -1; }

	void update(Height *h)
	{
		tau = h->tau; oid = h->oid; r = h->r;
		delta = h->delta; id = h->id;
	}

	double		tau;	// Time the reference level was created
	nsaddr_t	oid;	// id of the router that created the ref level
	int		r;	// Flag indicating a reflected ref level
	int		delta;	// Value used in propagation of a ref level
	nsaddr_t	id;	// Unique id of the router
};



/* ======================================================================
   Packet Formats
   ====================================================================== */
#define	TORA_HDR_LEN            32

#define	TORATYPE_QRY		0x01
#define TORATYPE_UPD		0x02
#define TORATYPE_CLR		0x04

#define QRY_HDR_LEN		(IP_HDR_LEN + sizeof(struct hdr_tora_qry))
#define UPD_HDR_LEN		(IP_HDR_LEN + sizeof(struct hdr_tora_upd))
#define CLR_HDR_LEN		(IP_HDR_LEN + sizeof(struct hdr_tora_clr))

/*
 * General TORA Header - shared by all formats
 */
struct hdr_tora {
        u_int16_t       th_type;
	u_int16_t	reserved;
        nsaddr_t        th_dst;
};

struct hdr_tora_qry {
	u_int16_t	tq_type;
	u_int16_t	reserved;
	nsaddr_t	tq_dst;
};

struct hdr_tora_upd {
	u_int16_t	tu_type;
	u_int16_t	reserved;
	nsaddr_t	tu_dst;

	double		tu_tau;
	nsaddr_t	tu_oid;
	int		tu_r;
	int		tu_delta;
	nsaddr_t	tu_id;
};

struct hdr_tora_clr {
	u_int16_t	tc_type;
	u_int16_t	reserved;
	nsaddr_t	tc_dst;

	double		tc_tau;
	int		tc_oid;
};

#endif /* __tora_packet_h__ */

