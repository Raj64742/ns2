#ifndef __dgtree_pkt_h__
#define __dgtree_pkt_h__

#include <packet.h>

#define HDR_DGTREE_PKT(p) hdr_dgtree_pkt::access(p)

struct hdr_dgtree_pkt {

	nsaddr_t pkt_src;
	inline nsaddr_t& pkt_src() {
		return pkt_src_;
	}
	static int offset_;
	inline static int& offset() {
		return offset_;
	}
	inline static hdr_protoname_pkt* access(const Packet* p) {
		return (hdr_protoname_pkt*) p->access(offset_);
	}


};

#endif
