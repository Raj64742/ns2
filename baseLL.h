
#ifndef ns_baseLL_h
#define ns_baseLL_h

#include "delay.h"
#include "errmodel.h"


class BaseLL : public LinkDelay {
public:
	BaseLL();
	virtual void recv(Packet* p, Handler* h);
	virtual void handle(Event*);

protected:
	int command(int argc, const char*const* argv);
	ErrorModel* em_;	// error model
        NsObject* sendtarget_;  // usually the link layer of the peer
	NsObject* recvtarget_;  // usually the classifier of the same node
};

#endif
