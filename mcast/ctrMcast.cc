#include "agent.h"
#include "ctrMcast.h"

static class CtrMcastHeaderClass : public PacketHeaderClass {
public:
        CtrMcastHeaderClass() : PacketHeaderClass("PacketHeader/CtrMcast",
                                              sizeof(hdr_CtrMcast)) { } 
} class_CtrMcast_hdr;

static class CtrMcastEncapclass : public TclClass {
public:
        CtrMcastEncapclass() : TclClass("Agent/CtrMcast/Encap") {}
        TclObject* create(int, const char*const*) {
                return (new CtrMcastEncap);
        }
} class_CtrMcastEncap;

static class CtrMcastDecapclass : public TclClass {
public:
        CtrMcastDecapclass() : TclClass("Agent/CtrMcast/Decap") {}
        TclObject* create(int, const char*const*) {
                return (new CtrMcastDecap);
        }
} class_CtrMcastDecap;


int CtrMcastEncap::command(int argc, const char*const* argv)
{
       return Agent::command(argc, argv);
}

int CtrMcastDecap::command(int argc, const char*const* argv)
{
       return Agent::command(argc, argv);
}

void CtrMcastEncap::recv(Packet* p, Handler*)
{
        hdr_CtrMcast* ch = (hdr_CtrMcast*)p->access(off_CtrMcast_);
        hdr_ip* ih = (hdr_ip*)p->access(off_ip_);

	ch->src() = ih->src();
	ch->group() = ih->dst();
	ch->flowid() = ih->flowid();
	ih->src() = addr_;
	ih->dst() = dst_;
	ih->flowid() = fid_;

	target_->recv(p);
}

void CtrMcastDecap::recv(Packet* p, Handler*)
{
        hdr_CtrMcast* ch = (hdr_CtrMcast*)p->access(off_CtrMcast_);
        hdr_ip* ih = (hdr_ip*)p->access(off_ip_);

	ih->src() = ch->src();
	ih->dst() = ch->group();
	ih->flowid() = ch->flowid();

	target_->recv(p);
}
