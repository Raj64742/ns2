#include "Tcl.h"
#include "timer-handler.h"
#include "random.h"
#include "tcp.h"


class TelnetSource;

class TelnetSourceTimer : public TimerHandler {
 public:
        TelnetSourceTimer(TelnetSource* t) : TimerHandler() { t_ = t;};
	inline virtual void expire(Event* e);
 protected:
	TelnetSource* t_;
};


class TelnetSource : public TclObject {
 public:
        TelnetSource();
        int command(int argc, const char*const* argv);
 protected:
	friend class TelnetSourceTimer;
	void start();
	void stop();
	void timeout();
	double interval_;
	inline double next();
	TelnetSourceTimer timer_;
	int running_;
	TcpAgent* tcp_;
};

