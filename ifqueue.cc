
#include "ifqueue.h"


static class IFQueueClass : public TclClass {
public:
	IFQueueClass() : TclClass("IFQueue") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new IFQueue);
	}
} class_ifqueue;


IFQueue::IFQueue() : Queue(), mac_(0), hRs_(*this)
{
}


int
IFQueue::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "mac") == 0) {
			mac_ = (Mac*) TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (Connector::command(argc, argv));
}


void
IFQueue::recv(Packet* p, Handler* h, NsObject* target, MacLink* ml)
{
	q_.enque(p);
	// tq_.enque(t);
	if (!blocked_) {
		p = deque();
		if (p) {
			blocked_ = 1;
			mac_->recv(p, &qh_, target);
		}
	}
}


void
IFQueue::enque(Packet* p)
{
	q_.enque(p);
}


Packet*
IFQueue::deque()
{
	// target_ = tq_.deque();
	return q_.deque();
}


void
IFQueue::resume()
{
	Packet* p = q_.deque();
	NsObject* target = 0;	// tq_.deque();
	if (p)
		mac_->recv(p, &qh_, target);
	else
		blocked_ = 0;
	
}


void
IFQueue::reset()
{
	while (Packet* p = q_.deque())
		drop(p);
//	while (tq_.deque());
}


void
IFQueueHandlerRs::handle(Event* e)
{
	ifq_.resume();
}
