//
// Copyright (c) 1997 by the University of Southern California
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
//	Author:		Kannan Varadhan	<kannan@isi.edu>
//	Version Date:	Mon Jun 30 15:51:33 PDT 1997
//
// @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/srm-headers.h,v 1.4 1998/07/01 18:58:12 yaxu Exp $ (USC/ISI)
//

#ifndef ns_srm_headers_h
#define ns_srm_headers_h

struct hdr_srm {

#define SRM_DATA 1
#define SRM_SESS 2
#define SRM_RQST 3
#define SRM_REPR 4

	int	type_;
	int	sender_;
	int	seqnum_;
	int	round_;
	
	// per field member functions
	int& type()	{ return type_; }
	int& sender()	{ return sender_; }
	int& seqnum()	{ return seqnum_; }
	int& round()	{ return round_; }
};

#define SRM_NAMES "SRM_DATA", "SRM_DATA", "SRM_SESS", "SRM_RQST", "SRM_REPR"

struct hdr_asrm {
	double	distance_;

	// per field member functions
	double& distance()	{ return distance_; }
};

#endif /* ns_srm_headers_.h */
