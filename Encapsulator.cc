#include "packet.h"
#include "ip.h"
#include "Encapsulator.h"

static class EncapsulatorClass : public TclClass {
public:
	EncapsulatorClass() : TclClass("Agent/Encapsulator") {}
	TclObject* create(int, const char*const*) {
		return (new Encapsulator());
	}
} class_encapsulator;

Encapsulator::Encapsulator() : Agent(PT_ENCAPSULATED), status_(0), dest_(0) 
{
    bind("dst_", &dest_);
    bind("status_", &status_);

};
void Encapsulator::recv(Packet* p, Handler* h)
{
       if (status_ > 0) {
 	   Packet* encap_p= p->copy();
 	   encap_p->allocdata(sizeof(Packet*));
//	   Packet* encap_p= allocpkt(sizeof(Packet*));
	   
	   Packet** pp= (Packet**) encap_p->accessdata();
	   *pp= p;

	   hdr_cmn* ch_e= (hdr_cmn*)encap_p->access(off_cmn_);
	   hdr_cmn* ch_p= (hdr_cmn*)p->access(off_cmn_);
	  
//	   *ch_e= *ch_p;
	   ch_e->ptype()= PT_ENCAPSULATED;
	   ch_e->size()+=  ENCAPSULATION_OVERHEAD;
//	   ch_e->uid()= ch_p->uid();
//	   ch_e->timestamp()= ch_p->timestamp(); // should decapsulator do smthng about timestamp?
//	   ch_e->iface()= ch_p->iface();
//	   ch_e->ref_count()= ch_p->ref_count();

	   hdr_ip* ih_e= (hdr_ip*)encap_p->access(hdr_ip::offset_);
	   ih_e->dst()= dest_; 
	   ih_e->flowid()= Agent::fid_;
	   send(encap_p, h);

       }
       else
	   send(p, h); //forward the packet as it is
}
