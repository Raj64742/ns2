// Copyright (c) Xerox Corporation 1998. All rights reserved.
//
// License is granted to copy, to use, and to make and to use derivative
// works for research and evaluation purposes, provided that Xerox is
// acknowledged in all documentation pertaining to any such copy or
// derivative work. Xerox grants no other licenses expressed or
// implied. The Xerox trade name should not be used in any advertising
// without its written permission. 
//
// XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
// MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
// FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
// express or implied warranty of any kind.
//
// These notices must be retained in any copies of any part of this
// software. 
//
// Auxiliary classes for HTTP multicast invalidation proxy cache
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/http-aux.h,v 1.1 1998/08/18 23:42:40 haoboy Exp $

#ifndef ns_http_aux_h
#define ns_http_aux_h

#include <tclcl.h>
#include "random.h"
#include "pagepool.h"
#include "timer-handler.h"

const int HTTP_HBEXPIRE_COUNT	= 5; // How many lost HBs mean disconnection?
const int HTTP_HBINTERVAL	= 60;// Heartbeat intervals (seconds)
const int HTTP_UPDATEINTERVAL	= 5; // Delays to push/v1 (seconds)

class HttpMInvalCache;

// Used for caches to keep track of invalidations
class InvalidationRec {
public:
	InvalidationRec(const char *pid, double mtime, char updating = 0) {
		pg_ = new char[strlen(pid) + 1];
		strcpy(pg_, pid);
		mtime_ = mtime;
		scount_ = HTTP_HBEXPIRE_COUNT;
		updating_ = updating;
		next_ = 0, prev_ = 0;
	}
	virtual ~InvalidationRec() {
		delete []pg_;
	}

	const char* pg() const { return pg_; }
	double mtime() const { return mtime_; }
	char updating() const { return updating_; }
	int scount() const { return scount_; }
	InvalidationRec* next() { return next_; } 

	void reset(double mtime) {
		scount_ = HTTP_HBEXPIRE_COUNT;
		mtime_ = mtime;
	}
	int dec_scount() { return --scount_; }
	void set_updating() { updating_ = 1; }
	void clear_updating() { updating_ = 0; }

	void insert(InvalidationRec **head) {
		next_ = *head;
		if (next_ != 0)
			next_->prev_ = &next_;
		prev_ = head;
		*head = this;
	}
	void detach() {
		if (prev_ != 0)
			*prev_ = next_;
		if (next_ != 0)
			next_->prev_ = prev_;
	}

	friend class HttpMInvalCache;

protected:
	char *pg_;
	double mtime_;
	char updating_;	// 1 if an update is going to be sent soon
	int scount_;	// Times that an invalidation needs to be multicast
	InvalidationRec *next_;
	InvalidationRec **prev_;
};

class HBTimer : public TimerHandler {
public: 
	HBTimer(HttpMInvalCache *a, double interval) : TimerHandler() { 
		a_ = a, interval_ = interval; 
	}
	void set_interval(double interval) { interval_ = interval; }
	double get_interval() const { return interval_; }
	double next_interval() {
		return interval_ * (1 + Random::uniform(-0.1,0.1));
	}
	void sched() { TimerHandler::sched(interval_); }
	void resched() {
		TimerHandler::resched(next_interval());
	}
protected: 
	virtual void expire(Event *e);
	virtual void handle(Event *e) {
		TimerHandler::handle(e);
		resched();
	}
	HttpMInvalCache *a_;
	double interval_;
};

class PushTimer : public TimerHandler {
public:
	PushTimer(HttpMInvalCache *a, double itv) : 
		TimerHandler(), a_(a), interval_(itv) {}
	void set_interval(double itv) { interval_ = itv; } 
	double get_interval() const { return interval_; }
	void sched() {
		TimerHandler::sched(interval_);
	}
protected:
	virtual void expire(Event *e);
	HttpMInvalCache *a_;
	double interval_;
};

class LivenessTimer : public TimerHandler {
public:
	LivenessTimer(HttpMInvalCache *a, double itv, int nbr) :
		TimerHandler(), a_(a), interval_(itv), nbr_(nbr) {}
	void sched() { TimerHandler::sched(interval_); }
	void resched() { TimerHandler::resched(interval_); }
protected:
	virtual void expire(Event *e);
	HttpMInvalCache *a_;	// The cache to be alerted
	int nbr_;		// Neighbor cache id
	double interval_;
};


// Packet types
const int HTTP_DATA		= 0;
const int HTTP_INVALIDATION 	= 1; // Heartbeat that may contain invalidation
const int HTTP_UPDATE		= 2; // Pushed page updates (version 1)
const int HTTP_PROFORMA		= 3; // Pro forma sent when a direct request 
				     // is sent
const int HTTP_JOIN		= 4;
const int HTTP_LEAVE		= 5;
const int HTTP_PUSH		= 6; // Selectively pushed pages (v2)

// User-level packets
class HttpData {
private:
	int type_;	// Packet type
	int id_;	// ID of the sender
protected:
	// used to extract data from a byte stream
	struct hdr {
		int type_;
		int id_;
	};
public:
	HttpData() { type() = HTTP_DATA; }
	HttpData(int t, int d) {
		type() = t;
		id() = d;
	}
	HttpData(char *b) {
		hdr* h = (hdr *)b;
		type() = h->type_;
		id_ = h->id_;
	}
	inline int& type() { return type_; }
	inline int& id() { return id_; }
	virtual int size() { return sizeof(hdr); }
	// Pack type and id into buf, and return a pointer to next 
	// available area
	virtual int hdrlen() { return sizeof(hdr); }
	virtual void pack(char *buf) {
		((hdr*)buf)->type_ = type_;
		((hdr*)buf)->id_ = id_;
	}
};

const int HTTPDATA_MAXURLLEN = 20;

// Struct used to pack invalidation records into packets
class HttpHbData : public HttpData {
protected:
	struct hdr : public HttpData::hdr {
		int num_inv_;
	};
	struct InvalRec {
		char pg_[HTTPDATA_MAXURLLEN];	// Maximum page id length
		double mtime_;
		// Used only to mark that this page will be send in the 
		// next multicast update. The updating field in this agent 
		// will only be set after it gets the real update.
		char updating_; 
		void copy(InvalidationRec *v) {
			strcpy(pg_, v->pg());
			mtime_ = v->mtime();
			updating_ = v->updating();
		}
		InvalidationRec* copyto() {
			return new InvalidationRec(pg_, mtime_, updating_);
		}
	};

private:
	int num_inv_;
	InvalRec* inv_rec_;
	inline InvalRec* inv_rec() { return inv_rec_; }

public:
	HttpHbData(int id, int n) : 
		HttpData(HTTP_INVALIDATION, id), num_inv_(n) {
		inv_rec_ = new InvalRec[num_inv_];
	}
	HttpHbData(char *data) : HttpData(data) {
		num_inv_ = ((hdr *)data)->num_inv_;
		inv_rec_ = new InvalRec[num_inv_];
		memcpy(inv_rec_, data+sizeof(hdr), num_inv_*sizeof(InvalRec));
	}
	virtual ~HttpHbData() {
		delete []inv_rec_;
	}

	virtual int size() { 
		return (num_inv_*sizeof(InvalRec) + sizeof(hdr));
	}
	virtual int hdrlen() { return sizeof(hdr); }
	virtual void pack(char *buf) {
		HttpData::pack(buf);
		((hdr*)buf)->num_inv_ = num_inv_;
		buf += sizeof(hdr);
		memcpy(buf, inv_rec_, num_inv_*sizeof(InvalRec));
	}

	inline int& num_inv() { return num_inv_; }
	inline void add(int i, InvalidationRec *r) {
		inv_rec()[i].copy(r);
	}
	inline char* rec_pg(int i) { return inv_rec()[i].pg_; }
	inline double& rec_mtime(int i) { return inv_rec()[i].mtime_; }
	void extract(InvalidationRec*& ivlist);
};

class HttpUpdateData : public HttpData {
protected:
	struct hdr : public HttpData::hdr {
		int num_;
		int pgsize_;	// Sum of page sizes
	};
	// Pack page updates to be pushed to caches
	struct PageRec {
		char pg_[HTTPDATA_MAXURLLEN];
		double mtime_;
		double age_;
		int size_;
		void copy(ClientPage *p) {
			strcpy(pg_, p->name());
			mtime_ = p->mtime();
			age_ = p->age();
			size_ = p->size();
		}
	};
private:
	int num_;
	int pgsize_;
	PageRec *rec_;
	inline PageRec* rec() { return rec_; }
public:
	HttpUpdateData(int id, int n) : HttpData(HTTP_UPDATE, id) {
		num() = n;
		pgsize() = 0;
		rec_ = new PageRec[num_];
	}
	HttpUpdateData(char *data) : HttpData(data) {
		num_ = ((hdr*)data)->num_;
		pgsize_ = ((hdr*)data)->pgsize_;
		rec_ = new PageRec[num_];
		memcpy(rec_, data+sizeof(hdr), num_*sizeof(PageRec));
	}
	virtual ~HttpUpdateData() {
		delete []rec_;
	}

	virtual int size() { 
		return sizeof(hdr) + num_*sizeof(PageRec); 
	}
	virtual int hdrlen() { return sizeof(hdr); }
	virtual void pack(char *buf) {
		HttpData::pack(buf);
		((hdr*)buf)->num_ = num();
		((hdr*)buf)->pgsize_ = pgsize();
		memcpy(buf+sizeof(hdr), rec_, num_*sizeof(PageRec));
	}

	inline int& num() { return num_; }
	inline int& pgsize() { return pgsize_; }
	inline void add(int i, ClientPage *p) {
		rec()[i].copy(p);
		pgsize() += p->size();
	}

	inline char* rec_page(int i) { return rec()[i].pg_; }
	inline int& rec_size(int i) { return rec()[i].size_; }
	inline double& rec_age(int i) { return rec()[i].age_; }
	inline double& rec_mtime(int i) { return rec()[i].mtime_; }
};

// Message: server leave
class HttpLeaveData : public HttpData {
protected:
	struct hdr : public HttpData::hdr {
		int num_; // number of servers to be invalidated
	};
private:
	int num_;
	int* rec_; // array of server ids which are out of contact
	inline int* rec() { return rec_; }
public:
	HttpLeaveData(int id, int n) : HttpData(HTTP_LEAVE, id) {
		num_ = n;
		rec_ = new int[num_];
	}
	HttpLeaveData(char *data) : HttpData(data) {
		num_ = ((hdr*)data)->num_;
		rec_ = new int[num_];
		memcpy(rec_, data+sizeof(hdr), num_*sizeof(int));
	}
	virtual ~HttpLeaveData() {
		delete []rec_;
	}

	virtual int size() { 
		return sizeof(hdr) + num_*sizeof(int);
	}
	virtual int hdrlen() { return sizeof(hdr); }
	virtual void pack(char* buf) {
		HttpData::pack(buf);
		((hdr*)buf)->num_ = num();
		memcpy(buf+sizeof(hdr), rec_, num_*sizeof(int));
	}

	inline int& num() { return num_; }
	inline void add(int i, int id) {
		rec()[i] = id;
	}
	inline int rec_id(int i) { return rec()[i]; }
};


// Auxliary class
class NeighborCache {
public:
	NeighborCache(HttpMInvalCache*c, double t, LivenessTimer *timer) : 
		cache_(c), down_(0), time_(t), timer_(timer) {}
	~NeighborCache();

	double time() { return time_; }
	double reset_timer(double time) { time_ = time, timer_->resched(); }
	int is_down() { return down_; }
	void down() { down_ = 1; }
	void up() { down_ = 0; }
	int num() { return sl_.num(); }
	HttpMInvalCache* cache() { return cache_; }
	void pack_leave(HttpLeaveData&);
	int is_server_down(int sid);
	void server_down(int sid);
	void server_up(int sid);
	void invalidate(HttpMInvalCache *c);
	void add_server(int sid);

	// Maintaining neighbor cache timeout entries
	struct ServerEntry {
		ServerEntry(int sid) : server_(sid), down_(0), next_(NULL) {}
		int server() { return server_; }
		ServerEntry* next() { return next_; }
		int is_down() { return down_; }
		void down() { down_ = 1; }
		void up() { down_ = 0; }

		int server_;
		ServerEntry *next_;
		int down_;
	};
	// We cannot use template. :(((
	struct ServerList {
		ServerList() : head_(NULL), num_(0) {}
		void insert(ServerEntry *s) {
			s->next_ = head_;
			head_ = s;
			num_++;
		}
		// We don't need a detach()
		ServerEntry* gethead() { return head_; } // For iterations
		int num() { return num_; }
		ServerEntry *head_;
		int num_;
	};

protected:
	HttpMInvalCache* cache_;
	double time_;
	int down_;
	ServerList sl_;
	LivenessTimer *timer_;
};

#endif // ns_http_aux_h
