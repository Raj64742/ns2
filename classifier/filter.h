#ifndef ns_filter_h
#define ns_filter_h

#include "connector.h"

class Filter : public Connector {
public:
	Filter();
	inline NsObject* filter_target() { return filter_target_; }
protected:
	enum filter_e { DROP, PASS, FILTER, DUPLICATE };
	virtual filter_e filter(Packet* p);

	int command(int argc, const char*const* argv);
	void recv(Packet*, Handler* h= 0);
	NsObject* filter_target_; // target for the matching packets
};

class FieldFilter : public Filter {
public:
	FieldFilter(); 
protected:
	filter_e filter(Packet *p);
	int offset_; // offset of the field
	int match_;
};
#endif
