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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/linkstate/hdr-ls.h,v 1.1 2000/08/18 18:34:03 haoboy Exp $

// Link state header should be present in ns regardless if the link state
// module is included (it may be omitted if standard STL is not supported
// by the compiler). The reason is we do not want a ns-packet.tcl.in, 
// and we cannot initialize a packet header in ns-stl.tcl.in. 
// Mysteriously the latter solution does not work; the only victim is DSR
// tests (e.g., DSR tests in wireless-lan, wireless-lan-newnode, wireless-tdma)

#ifndef ns_ls_hdr_h
#define ns_ls_hdr_h

#include "config.h"
#include "packet.h"

struct hdr_LS {
        u_int32_t mv_;  // metrics variable identifier
	int msgId_;

        u_int32_t& metricsVar() { return mv_; }
	int& msgId() { return msgId_; }

	// Header access methods
	static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_LS* access(Packet* p) {
		return (hdr_LS*) p->access(offset_);
	}
};

#endif // ns_ls_hdr_h
