#ifndef ns_conpers_h
#define ns_conpers_h

#include <stdio.h>
#include <limits.h>
#include <tcl.h>
#include <ranvar.h>
#include <tclcl.h>
#include "config.h"

#define IDLE 0
#define INUSE 1


// Abstract page pool, used for interface only
class PagePool : public TclObject {
public: 
	PagePool() : num_pages_(0), start_time_(INT_MAX), end_time_(INT_MIN) {}
	int num_pages() const { return num_pages_; }
protected:
	virtual int command(int argc, const char*const* argv);
	int num_pages_;
	double start_time_;
	double end_time_;
	int duration_;

	// Helper functions
	TclObject* lookup_obj(const char* name) {
		TclObject* obj = Tcl::instance().lookup(name);
		if (obj == NULL) 
			fprintf(stderr, "Bad object name %s\n", name);
		return obj;
	}
};

class PersConn : public TclObject {
public: 
	PersConn() : status_(IDLE), ctcp_(NULL), csnk_(NULL), client_(NULL), server_(NULL) {}
protected:
	int status_ ;
	TcpAgent* ctcp_ ;
	TcpSink* csnk_ ;
	Node* client_ ;
	Node* server_ ;

};

#endif //ns_conpers_h
