#include "packet.h"
#include "ip.h"

struct hdr_DV {
        u_int32_t       mv_;            // metrics variable identifier

	// per field member functions
        u_int32_t& metricsVar() { return mv_;   }
};

class rtProtoDV : public Agent {
public:
        rtProtoDV() : Agent(PT_RTPROTO_DV) { 
		bind("off_DV_", &off_DV_);
	}
        int command(int argc, const char*const* argv);
        void sendpkt(nsaddr_t dst, u_int32_t z, u_int32_t mtvar);
        void recv(Packet* p, Handler*);
protected:
	int off_DV_;
};
