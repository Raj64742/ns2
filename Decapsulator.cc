#include "Decapsulator.h"
#include "ip.h"
#include "packet.h"
#include "encap.h"

static class DecapsulatorClass : public TclClass {
public:
	DecapsulatorClass() : TclClass("Agent/Decapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Decapsulator());
	}
} class_decapsulator;

int Decapsulator::off_encap_= 0; //static variable, will be set in TCL
 
Decapsulator::Decapsulator()  : Agent(PT_ENCAPSULATED)
{ 
	bind("off_encap_", &off_encap_);
}

Packet* const Decapsulator::decapPacket(Packet* const p) 
{
	hdr_cmn* ch= (hdr_cmn*)p->access(hdr_cmn::offset_);
	if (ch->ptype() == PT_ENCAPSULATED) {
		hdr_encap* eh= (hdr_encap*)p->access(off_encap_);
		return eh->decap();
	}
	return 0;
}
void Decapsulator::recv(Packet* p, Handler* h)
{
        Packet *decap_p= decapPacket(p);
	if (decap_p) {
		send(decap_p, h);
		Packet::free(p);
		return;
	}
	send(p, h);
}
