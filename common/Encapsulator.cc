#include "packet.h"
#include "ip.h"
#include "Encapsulator.h"
#include "encap.h"

static class EncapsulatorClass : public TclClass {
public:
	EncapsulatorClass() : TclClass("Agent/Encapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Encapsulator());
	}
} class_encapsulator;

Encapsulator::Encapsulator() : 
	Agent(PT_ENCAPSULATED), 
	d_target_(0)
{
	bind("status_", &status_);
	bind("off_encap_", &off_encap_);
	bind("overhead_", &overhead_);
}

int Encapsulator::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "decap-target") == 0) {
			if (d_target_ != 0) tcl.result(d_target_->name());
			return (TCL_OK);
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1], "decap-target") == 0) {
			d_target_ = (NsObject*)TclObject::lookup(argv[2]);
			// even if it's zero, it's OK, we'll just not send to such
			// a target then.
			return (TCL_OK);
		}
	}
	return (Agent::command(argc, argv));
}

void Encapsulator::recv(Packet* p, Handler* h)
{
	if (d_target_) {
		Packet *copy_p= p->copy();
		d_target_->recv(copy_p, h);
	}
	if (status_) {
		Packet* ep= allocpkt(); //sizeof(Packet*));
		hdr_encap* eh= (hdr_encap*)ep->access(off_encap_);
		eh->encap(p);
		//Packet** pp= (Packet**) encap_p->accessdata();
		//*pp= p;

		hdr_cmn* ch_e= (hdr_cmn*)ep->access(off_cmn_);
		hdr_cmn* ch_p= (hdr_cmn*)p->access(off_cmn_);
	  
		ch_e->ptype()= PT_ENCAPSULATED;
 		ch_e->size()= ch_p->size() + overhead_;
		ch_e->timestamp()= ch_p->timestamp();
		send(ep, h);
	}
	else send(p, h); //forward the packet as it is
}
