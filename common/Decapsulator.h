
#ifndef ns_Decapsulator_h
#define ns_Decapsulator_h

#include "agent.h"
#include "config.h"

class Decapsulator : public Agent {
public:
	Decapsulator();
        Packet* const decapPacket(Packet* p); //return pointer to the encapsulated packet
protected:
	void recv(Packet*, Handler* callback = 0);
// 	nsaddr_t src_;        //bound variables: real source of a packet
//      nsaddr_t star_value_; //  replaces src_ before sending (e.g. to emulate (*,G)
                              //  alternatively, need to change classifier-mcast... later on
        int decapsulated_; //      decapsulated, or passed unchanged
	int off_encap_;
};

#endif
