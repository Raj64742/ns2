
#include "ip.h"
#include "trace.h"


class TraceIp : public Trace {
public:
	TraceIp(int type) : Trace(type) {}
	void recv(Packet*, Handler*);
};


class TraceIpClass : public TclClass {
public:
	TraceIpClass() : TclClass("TraceIp") { }
	TclObject* create(int args, const char*const* argv) {
		if (args >= 5)
			return (new TraceIp(*argv[4]));
		else
			return NULL;
	}
} traceip_class;


void TraceIp::recv(Packet* p, Handler* h)
{
	hdr_ip *iph = (hdr_ip*)p->access(off_ip_);

	// XXX: convert IP address to node number
	format(type_, src_>=0 ? src_ : iph->src()>>8, iph->dst()>>8, p);
	dump();
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
