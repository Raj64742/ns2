/*
 * Copyright (c) 2001 University of Southern California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising
 * materials, and other materials related to such distribution and use
 * acknowledge that the software was developed by the University of
 * Southern California, Information Sciences Institute.  The name of the
 * University may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 **
 * Light-Weight Multicast Services (LMS), Reliable Multicast
 *
 * lms.h
 *
 * This holds the packet header structures, and packet type constants for
 * the LMS implementation.
 *
 * Christos Papadopoulos. 
 * christos@isi.edu
 */

#ifndef lms_h
#define lms_h

class LmsAgent;

#include "node.h"
#include "packet.h"


#define LMS_NOADDR	-1
#define LMS_NOPORT      -1
#define LMS_NOIFACE	-99
#define LMS_INFINITY	1000000

/*
 * PT_LMS packet types
 */
#define	LMS_SRC_REFRESH	1
#define	LMS_REFRESH	2
#define	LMS_LEAVE	3
#define	LMS_REQ		4
#define	LMS_DMCAST	5
#define	LMS_SETUP	6
#define	LMS_SPM		7
#define	LMS_LINKS	8

/*
 * LMS header
 */
struct hdr_lms {
    int	       	type_;		// packet type
    int	       	ttl_;		// time-to-live
    int	       	cost_;		// cost advertisements for LMS_REFRESH
    nsaddr_t    from_;		// real source of packet for DMCASTs
    nsaddr_t    src_;		// original source of mcast packet
    nsaddr_t    group_;		// mcast group
    nsaddr_t    tp_addr_;	// turning point address
    int	       	tp_port_;	// turning point port id 
    int	       	tp_iface_;	// turning point interface
    int	       	lo_, hi_;	// range of lost packets
    double     	ts_;		// timestamp for RTT estimation
    
    static int offset_;
    inline static int& offset() { return offset_; }
    inline static hdr_lms* access(Packet* p) {
        return (hdr_lms*)p->access(offset_);
    }

    /* per-field member functions */    
    int&	type ()  { return type_;  }
    nsaddr_t&	from ()  { return from_;  }
    nsaddr_t&	src ()   { return src_;   }
    nsaddr_t&   tp_addr ()  { return tp_addr_;  }
    nsaddr_t&  tp_port ()  { return (nsaddr_t&) tp_port_;  }
    nsaddr_t&	group () { return group_; }
};

struct lms_ctl {
    int		hop_cnt_;	// hop counter (similar to ttl)
    int		cost_;		// cost advertisements for LMS_REFRESH
    nsaddr_t	tp_addr_;	// turning point address
    nsaddr_t    tp_port_;       // turning point port id
    ns_addr_t	downstream_lms_;// needed when in incrDeployment       
    int		tp_iface_;	// turning point interface
    int		nak_lo_;
    int		nak_hi_;	// range of lost packets
    double      ts_;            // timestamp
};

struct lms_nak {
	int	nak_lo_;
	int	nak_hi_;
	int	nak_seqn_;
        int     dup_cnt_;    // num of dup requests for each NAK
	packet_t t;
	int	datsize;    
};

struct lms_rdl {
	int		seqn_;
	double		ts_;
	struct lms_rdl	*next_;
};

struct lms_spm {
	int		spm_seqno_;
	nsaddr_t	spm_path_;
	double		spm_ts_;
};

#endif   /* end ifndef lms_h */
