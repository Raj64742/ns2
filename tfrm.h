#ifndef ns_tfrm_h
#define ns_tfrm_h

#include "agent.h"
#include "packet.h"


class TfrmAgent : public Agent {
public:
  	TfrmAgent();
  	virtual void recv(Packet*, Handler*);
   	int command(int argc, const char*const* argv);
  	void trace(TracedVar* v);
protected:
   	double rate;
};

class TfrmSinkAgent : public Agent {
public:
  	TfrmSinkAgent();
   	void recv(Packet*, Handler*);
protected:
  	double rate;
  	int *pvec;
};

#endif
