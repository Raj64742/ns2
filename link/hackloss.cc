#ifndef lint
static char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/link/hackloss.cc,v 1.1 1997/07/14 08:52:04 kannan Exp $";
#endif

#include "connector.h"

#include "packet.h"
#include "queue.h"

class HackLossyLink : public Connector {
public:
	HackLossyLink() : down_(0), src_(0), dst_(0), fid_(0), ctr_(0), nth_(0){
		bind("off_ip_", &off_ip_);
	}
protected:
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
	NsObject* down_;
	int src_, dst_, fid_;
	int ctr_, nth_;
	int off_ip_;
};

static class HackLossyLinkClass : public TclClass {
public:
	HackLossyLinkClass() : TclClass("HackLossyLink") {}
	TclObject* create(int, const char*const*) {
		return (new HackLossyLink);
	}
} class_dynamic_link;


int HackLossyLink::command(int argc, const char*const* argv)
{
	if (strcmp(argv[1], "down-target") == 0) {
		NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
		if (p == 0) {
			Tcl::instance().resultf("no object %s", argv[2]);
			return TCL_ERROR;
		}
		down_ = p;
		return TCL_OK;
	}
	if (strcmp(argv[1], "set-params") == 0) {
		src_ = atoi(argv[2]);
		dst_ = atoi(argv[3]);
		fid_ = atoi(argv[4]);
		return TCL_OK;
	}
	if (strcmp(argv[1], "nth") == 0) {
		nth_ = atoi(argv[2]);
		return TCL_OK;
	}
	return Connector::command(argc, argv);
}

void HackLossyLink::recv(Packet* p, Handler* h)
{
	hdr_ip* iph = (hdr_ip*) p->access(off_ip_);
	if (nth_ && (iph->flowid() == fid_) &&
	    (iph->src() == src_) && (iph->dst() == dst_) &&
	    ((++ctr_ % nth_) == 0))
		down_->recv(p);	// XXX  Why no handler?
	else
		target_->recv(p, h);
}
