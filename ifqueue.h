
#ifndef ns_ifqueue_h
#define ns_ifqueue_h

#include "queue.h"
#include "maclink.h"
#include "mac.h"


class IFQueue;

class IFQueueHandlerRs : public Handler {
public:
	IFQueueHandlerRs(IFQueue& q) : ifq_(q) {}
	virtual void handle(Event*);
protected:
	IFQueue& ifq_;
};


class IFQueue : public Queue {
public:
	IFQueue();
	virtual void recv(Packet*, Handler*, NsObject* target, MacLink*);
	void resume();

protected:
	int command(int argc, const char*const* argv);
	void enque(Packet*);
	Packet* deque();
	void reset();
	Mac* mac_;		// MAC to send to
	PacketQueue q_;
	/* TargetQueue tq_; */
	IFQueueHandlerRs hRs_;	// resume handler
};

#endif
