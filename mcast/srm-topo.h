/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) Xerox Corporation 1997. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 *
 * This file contributed by Suchitra Raman <sraman@parc.xerox.com>, June 1997.
 */

#ifndef srm_topo_h
#define srm_topo_h
#include "scheduler.h"
#include "random.h"

#define tprintf(x) { \
	Scheduler &_s = Scheduler::instance(); \
	double _now = _s.clock(); \
	printf("%f : ", _now); \
	printf x;\
	fflush(stdout);\
}              
#define SRM_DATA 0
#define SRM_RREQ 1
#define SRM_PENDING_RREQ 2


#define SRM_SUPPRESS 0
#define SRM_NO_SUPPRESS 1
#define SRM_NOIF -1


/* 
 * Events -- passed on from node to node 
 */
class SRM_Event : public Event {
 public:
	SRM_Event(int s=0, int t=0, int i=0) : seqno_(s), iif_(i), type_(t) {}
	SRM_Event(SRM_Event *e);
	int seqno() { return seqno_; }
	int iif() { return iif_; }

	int type() { return type_; }	
	void type(int t) { type_ = t; }
	void iif(int i) { iif_ = i; }

 protected:
	int seqno_;
	int iif_;
	int type_;
};


/* 
 * SRM_Request - Stored version of request to cancel when a similar
 * request is heard. Right now, a request only has an ID.
 * A real implementation would have a sequence range, if ADUs
 * were stacked.
 */

class SRM_Request {
 public:
	SRM_Request(SRM_Event *e) : event_(e), next_(0) {};
	~SRM_Request();
	void cancel_timer();

	SRM_Event *event_;
	SRM_Request *next_;
};


/* 
 * Light-weight node abstraction only to store SRM 
 * protocol state information.
 */
class SrmNode : public Handler {
 public:
	SrmNode() : id_(0), expected_(0), pending_(0) {}
	void id(int i) { id_ = i; }
	void handle(Event *);
	void send(SRM_Event *);

	void append(SRM_Event *);
	void remove(int , int);


 protected:
	void sched_nack(int);
	void dump_packet(SRM_Event *e);

	int id_;
	int expected_;
	SRM_Request *pending_;
};


/*
 * Interface -- Contains the node id of a node and 
 * a pointer to the next Interface 
 */
class Interface {
 public: 
	Interface(int in) : in_(in), next_(0) { }

	int in_;
	Interface *next_;
};

class Interface_List {
 public: 
	Interface_List() : head_(0) { }
	~Interface_List();
	void append(int in);

	Interface* head_;
};


/* 
 * Topology -- Line, Tree, Star derived from this base class.
 */
class Topology *topology; 

class Topology : public TclObject {
 public:
	Topology(int nn, int src);
	~Topology();
	virtual void flood(int, int) = 0;
	virtual Interface_List *oif(int node, int iif) = 0;

	int command(int argc, const char*const* argv);
	inline int idx() { return idx_; }
	SrmNode *node(int nn);
	virtual double backoff(int dst) = 0;
	inline double delay() { return delay_;}
	inline double D() { return D_;}
	virtual double delay(int src, int dst) = 0;
	int rtt_estimated() { return rtt_est_; }

 protected:
	SrmNode *node_; 
	int idx_;
	int src_;

	double delay_;
	double D_;
	double frac_;
	double det_;
	double rand_;
	int rtt_est_;
};

/* 
 * Line -- Chain with 'nn' nodes 
 */
class Line : public Topology {
 public: 
	Line(int n, int src) : Topology(n, src) { 
		topology = this; 
		bind("c_", &c_);
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);		
		bind("c2func_", &c2func_);		
	}
	void flood(int, int);
	Interface_List *oif(int node, int iif);
	double backoff(int dst);
	double delay(int src, int dst); 
 protected:
	int src_;
	int c_;
	double alpha_;
	double beta_;
	int c2func_;
};

#define LOG  0
#define SQRT 1
#define LINEAR 2
#define CONSTANT 3

/* 
 * BTree -- Binary Tree with 'nn' nodes 
 */
class BTree : public Topology {
 public: 
	BTree(int n, int src) : Topology(n, src) { 
		topology = this; 
		bind("c_", &c_);
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);
		bind("c2func_", &c2func_);
	}
	void flood(int, int);
	Interface_List *oif(int node, int iif);
	double backoff(int dst);
	double delay(int src, int dst); 

 protected:
	int src_;
	int c_;
	double alpha_;
	double beta_;
	int c2func_;
};


/* 
 * Star -- Complete graph with (nn-1) receivers and 1 sender 
 * at a distance 'Delay_' from each receiver. 
 */
class Star : public Topology {
 public:
	Star(int n, int src) : Topology(n, src) { 
		topology = this; 

		bind("c_", &c_);
		bind("alpha_", &alpha_);
		bind("beta_", &beta_);
	}

	void flood(int, int);
	Interface_List *oif(int node, int iif);
	double backoff(int dst);
	inline int c() { return c_; }
	double delay(int src, int dst); 

 protected:
	int src_;
	int c_;
	double alpha_;
	double beta_;
};
#endif 





