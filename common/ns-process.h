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
// ADU and ADU processor
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/common/ns-process.h,v 1.1 1999/03/15 20:48:50 haoboy Exp $

#ifndef ns_process_h
#define ns_process_h

#include <assert.h>
#include <string.h>
#include <tclcl.h>

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

	// Multimedia ADU
	MEDIA_DATA,
	MEDIA_REQUEST,

	// Last ADU
	ADU_LAST
};

// Interface for generic application-level data unit. It should know its 
// size and how to make itself persistent.
class AppData {
private:
	AppDataType type_;  	// ADU type
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
	virtual int size() const { return hdrlen(); }
	virtual int hdrlen() const { return sizeof(hdr); }
	AppDataType type() const { return type_; }

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

// Models any entity that is capable of process an ADU. 
// The basic functionality of this entity is to (1) process data, 
// (2) pass data to another entity, (3) request data from another entity.
class Process : public TclObject {
public: 
	Process() : target_(0) {}
	inline Process*& target() { return target_; }

	// Process incoming data
	virtual void process_data(int size, char* data);

	// Request data from the previous application in the chain
	virtual AppData* get_data(int& size, const AppData* req_data = 0);

	// Send data to the next application in the chain
	virtual void send_data(int size, char* data = 0) {
		if (target_)
			target_->process_data(size, data);
	}

protected:
	virtual int command(int argc, const char*const* argv);
	Process* target_;
};

#endif // ns_process_h
