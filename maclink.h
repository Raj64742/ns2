
#ifndef ns_maclink_h
#define ns_maclink_h

#include "delay.h"
#include "errmodel.h"


class MacLink;
class IFQueue;

class MacLink : public LinkDelay {
public:
	MacLink();
	virtual void recv(Packet* p, Handler* h);

protected:
	int command(int argc, const char*const* argv);
	IFQueue* ifq_;		// interface queue
	ErrorModel* em_;	// error model
};

#endif
