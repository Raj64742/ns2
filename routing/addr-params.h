// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:53:08 haoboy>
//
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
// Address parameters. Singleton class
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/routing/addr-params.h,v 1.3 2001/02/01 22:59:58 haldar Exp $

#ifndef ns_addr_params_h
#define ns_addr_params_h

#include <tclcl.h>

class AddrParamsClass : public TclClass{
public:
	// XXX Default hlevel_ to 1.
	AddrParamsClass() : TclClass("AddrParams"),
			    hlevel_(1), NodeMask_(NULL), 
			    NodeShift_(NULL) {
		instance_ = this;
	}
	virtual TclObject* create(int, const char*const*) {
		Tcl::instance().add_errorf("Cannot create instance of "
					   "AddrParamsClass");
		return NULL;
	}
	virtual void bind();
	virtual int method(int ac, const char*const* av);

	static inline AddrParamsClass& instance() { return *instance_; }

	inline int mcast_shift() { return McastShift_; }
	inline int mcast_mask() { return McastMask_; }
	inline int port_shift() { return PortShift_; }
	inline int port_mask() { return PortMask_; }
	inline int node_shift(int n) { return NodeShift_[n]; }
	inline int node_mask(int n) { return NodeMask_[n]; }
	inline int hlevel() { return hlevel_; }
	inline int nodebits() { return nodebits_; }

private:
	int McastShift_;
	int McastMask_;
	int PortShift_;
	int PortMask_;
	int hlevel_;
	int *NodeMask_;
	int *NodeShift_;
	int nodebits_;
	static AddrParamsClass *instance_;
};

#endif // ns_addr_params_h
