#ifndef net_interface_cc
#define net_interface_cc

#include "connector.h"
#include "packet.h"
#include "ip.h"

class NetworkInterface : public Connector {
public:
	interfaceLabel label;
	NetworkInterface() : label(-1) { bind("off_ip_", &off_ip_); }
	NetworkInterface(interfaceLabel lbl) : label(lbl) { bind("off_ip_", &off_ip_); }

	~NetworkInterface() {}

	command(int argc, const char*const* argv);

	void recv(Packet* pkt, Handler*);
protected:
        int off_ip_;
};

void NetworkInterface::recv(Packet* pkt, Handler* h) 
{
        hdr_ip *iph = (hdr_ip*)pkt->access(off_ip_);
	iph->iface() = label;
	//printf ("networkinterface: %u\n", iph->iface());
	send(pkt,h);
}

static class NetworkInterfaceClass : public TclClass {
public:
	NetworkInterfaceClass() : TclClass("networkinterface") {}
	TclObject* create(int argc, const char*const* argv) {
                return (new NetworkInterface);
	}
} class_networkinterface;

int NetworkInterface::command(int argc, const char*const* argv)
{
        if (argc == 3) {
                if (strcmp(argv[1], "label") == 0) {
                        label = atoi(argv[2]);
                return (TCL_OK);
                }
        }
        return (Connector::command(argc, argv));
}

#endif 
