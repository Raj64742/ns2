#include "agent.h"
#include "rtProtoDV.h"

static class rtDVHeaderClass : public PacketHeaderClass {
public:
        rtDVHeaderClass() : PacketHeaderClass("PacketHeader/rtProtoDV",
					      sizeof(hdr_DV)) { } 
} class_rtProtoDV_hdr;

static class rtProtoDVclass : public TclClass {
public:
	rtProtoDVclass() : TclClass("Agent/rtProto/DV") {}
	TclObject* create(int, const char*const*) {
		return (new rtProtoDV);
	}
} class_rtProtoDV;


int rtProtoDV::command(int argc, const char*const* argv)
{
        if (strcmp(argv[1], "send-update") == 0) {
                nsaddr_t  dst   = atoi(argv[2]);
                u_int32_t mtvar = atoi(argv[3]);
		u_int32_t size  = atoi(argv[4]);
                sendpkt(dst, mtvar, size);
                return TCL_OK;
       }
       return Agent::command(argc, argv);
}

void rtProtoDV::sendpkt(nsaddr_t dst, u_int32_t mtvar, u_int32_t size)
{
	dst_ = dst;
	size_ = size;
	
        Packet* p = Agent::allocpkt();
	hdr_DV *rh = (hdr_DV*)p->access(off_DV_);
        rh->metricsVar() = mtvar;

        target_->recv(p);               
}

void rtProtoDV::recv(Packet* p, Handler*)
{
	hdr_DV* rh = (hdr_DV*)p->access(off_DV_);
	hdr_ip* ih = (hdr_ip*)p->access(off_ip_);
        Tcl::instance().evalf("%s recv-update %d %d", name(),
			      ih->src(), rh->metricsVar());
}
