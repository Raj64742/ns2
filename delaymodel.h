/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * ctrMcast.cc
 * Copyright (C) 1997 by USC/ISI
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
 * Contributed by Polly Huang (USC/ISI), http://www-scf.usc.edu/~bhuang
 * 
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/delaymodel.h,v 1.5 1998/06/27 01:23:43 gnguyen Exp $ (UCB)
 */

#ifndef ns_delaymodel_h
#define ns_delaymodel_h

#include "connector.h"
#include "ranvar.h"


class DelayModel : public Connector {
public:
	DelayModel();
	void recv(Packet*, Handler*);
	inline double txtime(Packet* p) {
		hdr_cmn *hdr = (hdr_cmn*)p->access(off_cmn_);
		return bandwidth_ ? (hdr->size() * 8. / bandwidth_) : 0;
	}
	double bandwidth() const { return bandwidth_; }

protected:
	int command(int argc, const char*const* argv);
	RandomVariable* ranvar_;
	double bandwidth_;	/* bandwidth of underlying link (bits/sec) */
	//Event intr_;
};

#endif
