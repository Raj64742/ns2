
#ifndef ns_baselink_h
#define ns_baselink_h

#include "delay.h"
#include "errmodel.h"


class IFQueue;

class BaseLink : public LinkDelay {
public:
	BaseLink();
	virtual void recv(Packet* p, Handler* h);

protected:
	int command(int argc, const char*const* argv);
	IFQueue* ifq_;		// interface queue
	ErrorModel* em_;	// error model
};

#endif
