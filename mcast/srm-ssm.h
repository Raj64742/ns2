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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/mcast/srm-ssm.h,v 1.4 2000/09/01 03:04:07 haoboy Exp $

#ifndef ns_ssmsrm_h
#define ns_ssmsrm_h

#include "config.h"
//#include "heap.h"
#include "srm-state.h"


/* Constants for scope flags and types of session messages */
#define SRM_LOCAL 1
#define SRM_GLOBAL 2
#define SRM_RINFO  3  /* Session Message with reps information */

struct hdr_srm_ext {
	int     repid_;
	int     origTTL_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_srm_ext* access(const Packet* p) {
		return (hdr_srm_ext*) p->access(offset_);
	}

	// per field member functions
	int& repid()	{ return repid_; }
	int& ottl()    { return origTTL_; } 
};

class SSMSRMAgent : public SRMAgent 
{
  int	glb_sessCtr_;		  /* # of global session messages sent */
  int	loc_sessCtr_;		  /* # of local session messages sent */
  int	rep_sessCtr_;		  /* # of rep session messages sent */
  int   scopeFlag_;
  int   groupScope_;              /* Scope of the group, ttl */
  int   localScope_;              /* Scope of the local messages, ttl */
  int   senderFlag_;
  int   repid_;

  void recv_data(int sender, int id, int repid, u_char* data);
  //void recv_repr(int sender, int msgid, int repid, u_char* data);
  void recv_rqst(int requestor, int round, int sender, int repid, int msgid);
  void recv_sess(int sessCtr, int* data, Packet *p);
  void recv_glb_sess(int sessCtr, int* data, Packet *p);
  void recv_loc_sess(int sessCtr, int* data, Packet *p);
  void recv_rep_sess(int sessCtr, int* data, Packet *p);
  void send_ctrl(int type, int round, int sender, int msgid, int size);
  void send_sess();
  void send_glb_sess();
  void send_loc_sess();
  void send_rep_sess();
  void timeout_info();
  int is_active(SRMinfo *sp);
public:
  SSMSRMAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet* p, Handler* h);
};


#endif // ns_srm_ssm_h
