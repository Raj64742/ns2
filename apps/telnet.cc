
#include "random.h"
#include "tcp.h"
#include "telnet.h"

extern double tcplib_telnet_interarrival();

static class SourceClass : public TclClass {
 public:
	SourceClass() : TclClass("Source") {}
	TclObject* create(int, const char*const*) {
		return (new Source);
	}
} class_source;

static class TelnetSourceClass : public TclClass {
 public:
	TelnetSourceClass() : TclClass("Source/Telnet") {}
	TclObject* create(int, const char*const*) {
		return (new TelnetSource);
	}
} class_source_telnet;


Source::Source() : maxpkts_(1<<28)
{
	bind("maxpkts_", &maxpkts_);
}


TelnetSource::TelnetSource() : Source(), timer_(this), tcp_(0), running_(0)
{
	bind("interval_", &interval_);
}

int TelnetSource::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			start();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stop") == 0) {
			stop();
			return (TCL_OK);
		}
	}
	if (argc == 3) {
	        if (strcmp(argv[1], "attach-agent") == 0) {
		        tcp_ = (TcpAgent*)TclObject::lookup(argv[2]);
			if (tcp_ == 0) {
			        tcl.resultf("no such agent %s", argv[2]);
				return(TCL_ERROR);
			}
			return(TCL_OK);
		}
		
	}        
        return (TclObject::command(argc, argv));

}


void TelnetSourceTimer::expire(Event*)
{
        t_->timeout();
}


void TelnetSource::start()
{
        running_ = 1;
	double t = next();
	timer_.sched(t);
}

void TelnetSource::stop()
{
        running_ = 0;
}

void TelnetSource::timeout()
{
        if (running_) {
	        /* call the TCP advance method */
		tcp_->advanceby(1);
		/* reschedule the timer */
		double t = next();
		timer_.resched(t);
	}
}

double TelnetSource::next()
{
        if (interval_ == 0)
	        /* use tcplib */
	        return tcplib_telnet_interarrival();
	else
	        return Random::exponential() * interval_;
}
