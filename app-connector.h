/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
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
// Application connector
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/app-connector.h,v 1.1 1999/02/18 22:55:58 haoboy Exp $

#ifndef ns_app_connector_h
#define ns_app_connector_h

#include <assert.h>
#include <string.h>
#include <tclcl.h>

#include "app.h"

class AppConnector;

class AppData {
public:
	AppData() { d_ = NULL; size_ = 0; } 
	AppData(int size) {
		if (size > 0) {
			size_ = size;
			d_ = new char[size];
			assert(d_ != NULL);
		} else {
			d_ = NULL;
			size_ = 0;
		}
	}
	AppData(int size, const void* data) {
		if (size > 0) {
			size_ = size;
			d_ = new char[size];
			assert(d_ != NULL);
			assert(data != NULL);
			memcpy(d_, data, size);
		} else {
			d_ = NULL;
			size_ = 0;
		}
	}
	AppData(AppData *a) {
		if ((a == NULL) || (a->size == 0)) {
			d_ = NULL;
			size_ = 0;
		} else {
			size_ = a->size_;
			d_ = new char[size_];
			assert(d_ != NULL);
			memcpy(d_, a->d_, size_);
		}
	}
	// Persistence
	AppData(char *buf) {
		assert(buf != 0);
		size_ = ((hdr *)buf)->size_;
		buf += sizeof(hdr);
		if (size_ > 0) {
			d_ = new char[size_];
			assert(d_ != NULL);
			memcpy(d_, buf, size_);
		} else {
			d_ = NULL;
			size_ = 0;
		}
	}
	~AppData() {
		if (d_ != NULL)
			delete[] d_;
	}

	// Handlers
	void* data() { return d_; }
	int size() { return sizeof(hdr) + size_; }

	// Persistence
	void pack(char *buf) {
		((hdr *)buf)->size_ = size_;
		buf += sizeof(hdr);
		memcpy(buf, d_, size_);
	}

private:
	void* d_;
	int size_;
	struct hdr {
		int size_;
	};
};

class AppConnector {
public: 
	AppConnector() : target_(0) {}
	inline AppConnector*& target() { return target_; }

	virtual void process_data(AppData* data) = 0;
	virtual void send_data(AppData* data = 0) {
		if (target_)
			((AppConnector*)target_)->process_data(data);
	}

protected:
	AppConnector* target_;
};

#endif // ns_app_connector_h
