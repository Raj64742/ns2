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
// Definitions for class PagePool
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/pagepool.h,v 1.3 1998/10/05 17:43:44 haoboy Exp $

#ifndef ns_pagepool_h
#define ns_pagepool_h

#include <stdio.h>
#include <limits.h>
#include <tcl.h>
#include <ranvar.h>
#include <tclcl.h>
#include "config.h"

class Page {
public:
	Page(const char *name, int size);
	virtual ~Page() {
		delete []name_;
	}
	const char *name() const { return name_; }
	int size() const { return size_; }

protected:
	char *name_;
	int id_;
	int size_;
};

class ServerPage : public Page {
public:
	ServerPage(char *name, int size, int id) : Page(name, size) {
		id_ = id, mtime_ = NULL, num_mtime_ = 0;
	}
	virtual ~ServerPage() {
		if (mtime_ != NULL) 
			delete []mtime_;
	}
	const char *name() const { return name_; }
	int& id() { return id_; }
	int& size() { return size_; }
	int& mtime(int n) { return mtime_[n]; }
	int& num_mtime() { return num_mtime_; }
	void set_mtime(int *mt, int n);

protected:
	int id() const { return id_; }
	int *mtime_;
	int num_mtime_;
};

class HttpApp;

// Page states
const int HTTP_VALID_PAGE	= 1;	// Valid page
const int HTTP_INVALID_PAGE	= 2;	// Invalid page
const int HTTP_SERVER_DOWN	= 3;	// Server is down. Don't know if 
					// page is valid
const int HTTP_VALID_HEADER	= 4;	// Only meta-data is valid
const int HTTP_UNREAD_PAGE	= 5;	// Unread valid page

class ClientPage : public Page {
public:
	ClientPage(const char *n, int s, double mt, double et, double a);

	double& mtime() { return mtime_; }
	double& etime() { return etime_; }
	double& age() { return age_; }
	int id() { return id_; }
	HttpApp* server() { return server_; }
	inline int& counter() { return counter_; }

	void validate(double mtime) { 
		if (mtime_ >= mtime)
			abort(); // This shouldn't happen!
		status_ = HTTP_VALID_PAGE; 
		mtime_ = mtime;
	}
	void invalidate(double mtime) { 
		if (mtime_ >= mtime)
			return;
		status_ = HTTP_INVALID_PAGE; 
		mtime_ = mtime;
	}

	inline void server_down() {
		// Don't change mtime, only change page status
		// Useful for setting HTTP_SERVER_DOWN
		status_ = HTTP_SERVER_DOWN;
	}
	int is_valid() const { 
		return (status_ == HTTP_VALID_PAGE) || 
			(status_ == HTTP_UNREAD_PAGE);
	}
	int is_header_valid() const {
		return ((status_ == HTTP_VALID_PAGE) || 
			(status_ == HTTP_VALID_HEADER));
	}
	inline int is_server_down() { return (status_ == HTTP_SERVER_DOWN); }
	inline void set_valid_hdr() { status_ = HTTP_VALID_HEADER; }
	inline void set_unread() { status_ = HTTP_UNREAD_PAGE; }
	inline void set_read() { status_ = HTTP_VALID_PAGE; }
	inline int is_unread() { return (status_ == HTTP_UNREAD_PAGE); }
	inline int count_inval(int a) { counter_ -= a; return counter_; }
	inline int count_request(int b) { counter_ += b; return counter_; }
	inline void set_mpush(double time) { 
		mandatoryPush_ = 1, mpushTime_ = time; 
	}
	inline void clear_mpush() { mandatoryPush_ = 0; }
	inline int is_mpush() { return mandatoryPush_; }
	inline double mpush_time() { return mpushTime_; }

	friend class ClientPagePool;

protected:
	HttpApp* server_;
	int id_;
	double age_;
	double mtime_;	// modification time
	double etime_;	// entry time
	int status_;	// VALID or INVALID
	int counter_;	// counter for invalidation & request
	int mandatoryPush_;
	double mpushTime_;
};


// Abstract page pool, used for interface only
class PagePool : public TclObject {
public: 
	PagePool() : num_pages_(0), start_time_(INT_MAX), end_time_(INT_MIN) {}
	int num_pages() const { return num_pages_; }
protected:
	int num_pages_;
	double start_time_;
	double end_time_;
	int duration_;
};

// Page pool based on real server traces

const int TRACEPAGEPOOL_MAXBUF = 4096;

// This trace must contain web page names and all of its modification times
class TracePagePool : public PagePool {
public:
	TracePagePool(const char *fn);
	virtual ~TracePagePool();
	virtual int command(int argc, const char*const* argv);

protected:
	Tcl_HashTable *namemap_, *idmap_;
	RandomVariable *ranvar_;

	ServerPage* load_page(FILE *fp);
	void change_time();

	int add_page(ServerPage *pg);
	ServerPage* get_page(int id);
};

// Page pool based on mathematical models of request and page 
// modification patterns
class MathPagePool : public PagePool {
public:
	// XXX TBA: what should be here???
	MathPagePool() : rvSize_(0), rvAge_(0) { num_pages_ = 1; }

protected:
	virtual int command(int argc, const char*const* argv);
	// Single page
	RandomVariable *rvSize_;
	RandomVariable *rvAge_;
};

// Assume one main page, which changes often, and multiple component pages
class CompMathPagePool : public PagePool {
public:
	CompMathPagePool();

#ifdef JOHNH_CLASSINSTVAR
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName);
#endif /* JOHNH_CLASSINSTVAR */

protected:
	virtual int command(int argc, const char*const* argv);
	RandomVariable *rvMainAge_; // modtime for main page
	RandomVariable *rvCompAge_; // modtime for component pages
 	int main_size_, comp_size_;
};

class ClientPagePool : public PagePool {
public:
	ClientPagePool();
	virtual ~ClientPagePool();

	ClientPage* get_page(const char *name);
	ClientPage* add_page(const char *name, int size, double mt, 
			     double et, double age);
	ClientPage* add_metadata(const char *name, int size, double mt, 
				 double et, double age) {
		ClientPage *pg = add_page(name, size, mt, et, age);
		pg->set_valid_hdr();
		return pg;
	}
	void invalidate_server(int server_id);

	int get_mtime(const char *name, double &mt);
	int set_mtime(const char *name, double mt);
	int exist_page(const char *name) { return (get_page(name) != NULL); }
	int get_size(const char *name, int &size);
	int get_age(const char *name, double &age);
	int get_etime(const char *name, double &et);
	int set_etime(const char *name, double et);
	int get_page(const char *name, char *buf);

protected:
	Tcl_HashTable *namemap_;
};

// How could we load a client request trace? 
// This is *not* designed for BU trace files. We should write a script to 
// transform BU traces to a single trace file with the following format:
//
// <client id> <page id> <time> <size>
//
// Q: How would we deal with page size changes? 
// What if simulated response time
// is longer and a real client request for the same page happened before the 
// simulated request completes? 

// class ClientTracePagePool : public TracePagePool {
// public:
// 	ClientTracePagePool(const char *fn);
// 	virtual ~ClientTracePagePool();
// 	virtual int command(int argc, const char*const* argv);

// protected:
// 	struct Page {
// 	};
// 	// How would we handle different types of page modifications? How 
// 	// to integrate bimodal, and multi-modal distributions?
// 	virtual Page* load_page(FILE *fp);
// };

#endif //ns_pagepool_h
