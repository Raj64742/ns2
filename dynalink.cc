#ifndef lint
static char rcsid[] =
	"@(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/Attic/dynalink.cc,v 1.3 1997/05/16 07:58:24 kannan Exp $";
#endif

#include "connector.h"

// #include "packet.h"
// #include "queue.h"

class DynamicLink : public Connector {
public:
	DynamicLink() : down_(0), status_(1) { bind("status_", &status_); }
protected:
	int command(int argc, const char*const* argv);
	void recv(Packet* p, Handler* h);
	NsObject* down_;
	int status_;
};

static class DynamicLinkClass : public TclClass {
public:
	DynamicLinkClass() : TclClass("DynamicLink") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DynamicLink);
	}
} class_dynamic_link;


int DynamicLink::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "status?") == 0) {
			Tcl::instance().result(status_ ? "up" : "down");
			return TCL_OK;
		}
		if (strcmp(argv[1], "down-target") == 0) {
			if (down_ != 0)
				Tcl::instance().resultf("%s", down_->name());
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "down-target") == 0) {
			NsObject* p = (NsObject*)TclObject::lookup(argv[2]);
			if (p == 0) {
				Tcl::instance().resultf("no object %s", argv[2]);
				return TCL_ERROR;
			}
			down_ = p;
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv);
}

void DynamicLink::recv(Packet* p, Handler* h)
{
	if (status_)
		target_->recv(p, h);
	else
		down_->recv(p);	// XXX  Why no handler?
}
