
#ifndef ns_telnet_h
#define ns_telnet_h

#include "tclcl.h"
#include "timer-handler.h"

class TcpAgent;
class TelnetSource;


class Source : public TclObject {
public:
	Source();
protected:
	int maxpkts_;
};


class TelnetSourceTimer : public TimerHandler {
 public:
	TelnetSourceTimer(TelnetSource* t) : TimerHandler(), t_(t) {}
	inline virtual void expire(Event*);
 protected:
	TelnetSource* t_;
};

class TelnetSource : public Source {
 public:
	TelnetSource();
	void timeout();
 protected:
	int command(int argc, const char*const* argv);
	void start();
	void stop();
	inline double next();

	TelnetSourceTimer timer_;
	TcpAgent* tcp_;
	int running_;
	double interval_;
};

#endif
