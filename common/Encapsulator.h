
#ifndef ns_Encapsulator_h
#define ns_Encapsulator_h

#include "agent.h"


class Encapsulator : public Agent {
public:
	Encapsulator();
        static int const ENCAPSULATION_OVERHEAD= 20; // external IP header size 
protected:
	void recv(Packet*, Handler* callback = 0);
	int command(int argc, const char*const* argv);
	int status_;    // 1 (on) or 0 (off, default), bound variable
	NsObject* d_target_;
	int off_encap_;
};

#endif
