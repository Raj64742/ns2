#include "packet.h"
#include "ip.h"

struct hdr_CtrMcast {
        nsaddr_t       src_;            //mcast data source
	nsaddr_t       group_;          //mcast data destination group
	int            fid_;

        // per field member functions
        nsaddr_t& src() { return src_;   }
        nsaddr_t& group() { return group_;   }
        int& flowid() { return fid_;   }
};

class CtrMcastEncap : public Agent {
public:
        CtrMcastEncap() : Agent(PT_CtrMcast_Encap) { 
                bind("off_CtrMcast_", &off_CtrMcast_);

        }
        int command(int argc, const char*const* argv);
        void recv(Packet* p, Handler*);
protected:
        int off_CtrMcast_;
};

class CtrMcastDecap : public Agent {
public:
        CtrMcastDecap() : Agent(PT_CtrMcast_Decap) { 
                bind("off_CtrMcast_", &off_CtrMcast_);
        }
        int command(int argc, const char*const* argv);
        void recv(Packet* p, Handler*);
protected:
        int off_CtrMcast_;
};
