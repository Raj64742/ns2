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

#ifndef ns_srm_h
#define ns_srm_h

#include "config.h"
#include "heap.h"
#include "srm-state.h"

struct hdr_srm {

#define SRM_DATA 1
#define SRM_SESS 2
#define SRM_RQST 3
#define SRM_REPR 4

	int	type_;
	int	sender_;
	int	seqnum_;
	
	// per field member functions
	int& type()	{ return type_; }
	int& sender()	{ return sender_; }
	int& seqnum()	{ return seqnum_; }
};

class SRMAgent : public Agent {
	int	dataCtr_;	          /* # of data packets sent */
	int	sessCtr_;		  /* # of session messages sent */
	int	packetSize_;	          /* size of data messages for repr */
	SRMinfo* sip_;		          /* sender info ptr. */
	int	groupSize_;
	int off_srm_;
	int off_cmn_;

	SRMinfo* get_state(int sender) {
		assert(sip_);

		SRMinfo* ret;
		for (ret = sip_; ret; ret = ret->next_)
			if (ret->sender_ == sender)
				break;
		if (! ret) {
			ret = new SRMinfo(sender);
			ret->next_ = sip_->next_;
			sip_->next_ = ret;
			groupSize_++;
		}
		return ret;
	}
	void recv_data(int sender, int id, u_char* data);
        void recv_repr(int sender, int msgid, u_char* data);
        void recv_rqst(int requestor, int sender, int msgid);
	void recv_sess(int sessCtr, int* data);

	void send_ctrl(int type, int sender, int msgid, int size);
	void send_sess();
public:
	SRMAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
};


#endif

