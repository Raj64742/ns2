
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
	format(type_, iph->src(), iph->dst(), p);
	dump();
	/* hack: if trace object not attached to anything free packet */
	if (target_ == 0)
		Packet::free(p);
	else
		send(p, h);
}
