
#ifndef ns_baseLL_h
#define ns_baseLL_h

#include "delay.h"
#include "errmodel.h"


class IFQueue;

class BaseLL : public LinkDelay {
public:
	BaseLL();
	virtual void recv(Packet* p, Handler* h);

protected:
	int command(int argc, const char*const* argv);
	IFQueue* ifq_;		// interface queue
	ErrorModel* em_;	// error model
        NsObject* sendtarget_;  // usually the link layer of the peer
	NsObject* recvtarget_;  // usually the classifier of the same node
};

#endif
