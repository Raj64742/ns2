
#ifndef ns_error_h
#define ns_error_h

#include <fstream.h>
#include "connector.h"

#define EuName "pkt", "bit", "time"
enum ErrorUnit { EuPacket, EuBit, EuTime };


class ErrorModel : public Connector {
public:
	ErrorModel() : unit_(EuPacket), time_(0), loss_(0), good_(0) {}
	virtual int corrupt(Packet*);
	double rate() { return rate_; }
	void dump();

protected:
	int command(int argc, const char*const* argv);
	ErrorUnit unit_;
	double rate_;
	double time_;
	double loss_;
	double good_;
	ofstream ofs_;
};

#endif
