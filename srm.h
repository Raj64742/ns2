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
// @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/srm.h,v 1.7 1997/10/23 20:53:26 kannan Exp $ (USC/ISI)
//

#ifndef ns_srm_h
#define ns_srm_h

#include <math.h>

#include "config.h"
#include "heap.h"
#include "srm-state.h"

#include "srm-headers.h"

class SRMAgent : public Agent {
protected:
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
	virtual void addExtendedHeaders(Packet*) {}
	virtual void parseExtendedHeaders(Packet*) {}
	virtual int request(SRMinfo* sp, int hi) {
		int miss = 0;
		if (sp->ldata_ >= hi)
			return miss;
		
		int maxsize = ((int)log10(hi + 1) + 2) * (hi - sp->ldata_);
				// 1 + log10(msgid) bytes for the msgid
				// msgid could be 0, if first pkt is lost.
				// 1 byte per msg separator
				// hi - sp->ldata_ msgs max missing
		char* msgids = new char[maxsize + 1];
		*msgids = '\0';
		for (int i = sp->ldata_ + 1; i <= hi; i++)
			if (! sp->ifReceived(i)) {
				(void) sprintf(msgids, "%s %d", msgids, i);
				miss++;
			}
		assert(miss);
		Tcl::instance().evalf("%s request %d %s", name_,
				      sp->sender_, msgids);
		delete[] msgids;
		return miss;
	}

        void recv_data(int sender, int msgid, u_char* data);
        void recv_repr(int round, int sender, int msgid, u_char* data);
        void recv_rqst(int requestor, int round, int sender, int msgid);
	void recv_sess(int sessCtr, int* data);

	void send_ctrl(int type, int round, int sender, int msgid, int size);
	void send_sess();
public:
	SRMAgent();
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
};

class ASRMAgent : public SRMAgent {
	double pdistance_;
	int    requestor_;
	int    off_asrm_;
public:
	ASRMAgent() {
		bind("pdistance_", &pdistance_);
		bind("requestor_", &requestor_);
		bind("off_asrm_", &off_asrm_);
	}
protected:
	void addExtendedHeaders(Packet* p) {
		SRMinfo* sp;
		hdr_srm*  sh = (hdr_srm*) p->access(off_srm_);
		hdr_asrm* seh = (hdr_asrm*) p->access(off_asrm_);
		switch (sh->type()) {
		case SRM_RQST:
			sp = get_state(sh->sender());
			seh->distance() = sp->distance_;
			break;
		case SRM_REPR:
			sp = get_state(requestor_);
			seh->distance() = sp->distance_;
			break;
		case SRM_DATA:
		case SRM_SESS:
			seh->distance() = 0.;
			break;
		default:
			assert(0);
			/*NOTREACHED*/
		}
	}
	void parseExtendedHeaders(Packet* p) {
		hdr_asrm* seh = (hdr_asrm*) p->access(off_asrm_);
		pdistance_ = seh->distance();
	}
};

#endif
