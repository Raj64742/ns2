#include "Decapsulator.h"
#include "ip.h"
#include "packet.h"

static class DecapsulatorClass : public TclClass {
public:
	DecapsulatorClass() : TclClass("Agent/Decapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Decapsulator());
	}
} class_decapsulator;

Decapsulator::Decapsulator()  : Agent(PT_ENCAPSULATED) { }

Packet* const Decapsulator::decapPacket(Packet* p) 
{
	hdr_cmn* ch= (hdr_cmn*)p->access(hdr_cmn::offset_);
	if (ch->ptype() == PT_ENCAPSULATED) {
	    Packet** decap_pp= (Packet**)p->accessdata();
	    return *decap_pp;
	}
	return 0;
}
void Decapsulator::recv(Packet* p, Handler* h)
{
        Packet *decap_p= decapPacket(p);
	if (decap_p) {
	    //upcall: let the user examine decapsulated?_ and src_ and set 
	    //  up target_ and dst_ appropriately
	    //  call TCL "Agent/Decapsulator instproc announce-arrival src grp format"
	    //  where format is "encapsulated" or "native"
//	    Tcl::instance().evalf("%s announce-arrival %u %u encapsulated", 
//				  name(), src_, ih_p->dst()); 
	  
	    send(decap_p, h);
	    Packet::free(p);
	    return;
	}
	hdr_ip* ih_p= (hdr_ip*)p->access(hdr_ip::offset_);
	Tcl::instance().evalf("%s announce-arrival %u %u d native", 
			      name(), ih_p->src(), ih_p->dst()); 
	send(p, h);
}
