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
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/app-connector.h,v 1.2 1999/02/18 23:15:20 haoboy Exp $

#ifndef ns_app_connector_h
#define ns_app_connector_h

#include <assert.h>
#include <string.h>
#include <tclcl.h>

#include "app.h"

// Application-level data unit types
enum AppDataType {
	// Illegal type
	ADU_ILLEGAL,

	// HTTP ADUs
	HTTP_DATA,
	HTTP_INVALIDATION,	// Heartbeat that may contain invalidation
	HTTP_UPDATE,		// Pushed page updates (version 1)
	HTTP_PROFORMA,		// Pro forma sent when a direct request is sent
	HTTP_JOIN,
	HTTP_LEAVE,
	HTTP_PUSH,		// Selectively pushed pages 
	HTTP_NORMAL,		// Normal req/resp packets

	// TcpApp ADU
	TCPAPP_STRING,

	// Last ADU
	ADU_LAST
};

// Interface for generic application-level data unit. It should know its 
// size and how to make itself persistent.
class AppData {
private:
	AppDataType type_;  // ADU type
public:
	struct hdr {
		AppDataType type_;
	};
public:
	AppData(AppDataType type) { type_ = type; }
	AppData(char* b) {
		assert(b != NULL);
		type_ = ((hdr *)b)->type_;
	}

	// Attributes
	virtual int size() const = 0;
	virtual int hdrlen() const { return sizeof(hdr); }
	virtual AppDataType type() const { return type_; }

	// Persistence
	virtual void pack(char* buf) const {
		assert(buf != NULL);
		((hdr *)buf)->type_ = type_;
	}

	// Static type checking on a persistent ADU
	// XXX hacky and dangerous!! Better ideas??
	static AppDataType type(char* buf) {
		if (buf == NULL)
			return ADU_ILLEGAL;
		AppDataType t = ((hdr *)buf)->type_;
		if ((t >= ADU_LAST) || (t <= ADU_ILLEGAL))
			return ADU_ILLEGAL;
		else
			return t;
	}
};

class AppConnector {
public: 
	AppConnector() : target_(0) {}
	inline AppConnector*& target() { return target_; }

	virtual void process_data(int size, char* data) = 0;
	virtual void send_data(int size, char* data = 0) {
		if (target_)
			((AppConnector*)target_)->process_data(size, data);
	}

protected:
	AppConnector* target_;
};

#endif // ns_app_connector_h
