#ifndef ns_encap_h
#define ns_encap_h

struct hdr_encap {
	void encap(Packet* p) { ep= p; }
	Packet *decap() { return ep; }
private:
	Packet *ep;
};

static class encapHeaderClass : public PacketHeaderClass {
public:
        encapHeaderClass() : PacketHeaderClass("PacketHeader/Encap",
					     sizeof(hdr_encap)) {}
} class_encaphdr;


#endif
