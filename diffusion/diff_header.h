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
//  Copyright (C) 1998 by Mingzhou Sun. All rights reserved.
//  This software is developed at Rensselaer Polytechnic Institute under 
//  DARPA grant No. F30602-97-C-0274
//  Redistribution and use in source and binary forms are permitted
//  provided that the above copyright notice and this paragraph are
//  duplicated in all such forms and that any documentation, advertising
//  materials, and other materials related to such distribution and use
//  acknowledge that the software was developed by Mingzhou Sun at the
//  Rensselaer  Polytechnic Institute.  The name of the University may not 
//  be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/diffusion/diff_header.h,v 1.3 2001/11/08 18:16:32 haldar Exp $

/******************************************************/
/* diff_header.h : Chalermek Intanagonwiwat  08/16/99 */
/******************************************************/

#ifndef ns_diff_header_h
#define ns_diff_header_h

#include "ip.h"

#define INTEREST      1
#define DATA          2
#define DATA_READY    3
#define DATA_REQUEST  4
#define POS_REINFORCE 5
#define NEG_REINFORCE 6
#define INHIBIT       7
#define TX_FAILED     8
#define DATA_STOP     9

#define MAX_ATTRIBUTE 3
#define MAX_NEIGHBORS 30
#define MAX_DATA_TYPE 30

#define ROUTING_PORT 255

#define ORIGINAL    100        
#define SUB_SAMPLED 1         

// For positive reinforcement in simple mode. 
// And for TX_FAILED of backtracking mode.

struct extra_info {
  ns_addr_t sender;            // For POS_REINFORCE and TX_FAILED
  unsigned int seq;           // FOR POS_REINFORCE and TX_FAILED
  int size;                   // For TX_FAILED only.
};


struct hdr_cdiff {
	unsigned char mess_type;
	unsigned int pk_num;
	ns_addr_t sender_id;
	nsaddr_t next_nodes[MAX_NEIGHBORS];
	int      num_next;
	unsigned int data_type;
	ns_addr_t forward_agent_id;

	struct extra_info info;

	double ts_;                       // Timestamp when pkt is generated.
	int    report_rate;               // For simple diffusion only.
	int    attr[MAX_ATTRIBUTE];
	
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_cdiff* access(const Packet* p) {
		return (hdr_cdiff*) p->access(offset_);
	}
};



#endif
