/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
#ifndef ns_ump_h
#define ns_ump_h

#include "config.h"
#include "object.h"
#include "packet.h"
#include "ip.h"
#include "classifier.h"

// UMP option
struct hdr_ump {
	char isSet;
	int32_t umpID_,
		RP_;
	char *oif;
	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_ump* access(Packet* p) {
		return (hdr_ump*) p->access(offset_);
	} // access
};

static class UmpHeaderClass : public PacketHeaderClass {
public: 
	UmpHeaderClass() : PacketHeaderClass("PacketHeader/UMP",
					     sizeof(hdr_ump)) {}
} class_umphdr;

#endif
