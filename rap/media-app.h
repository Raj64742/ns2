/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
//
// Copyright (c) 1997 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Media applications: wrappers of transport agents for multimedia objects
// 
// Their tasks are to receive 'data' from underlying transport agents, 
// and to notify "above" applications that such data has arrived. 
//
// When required (by RapAgent), it also provides callbacks to the 
// transport agent, and contact the above application on behalf of the 
// transport agent.
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/rap/media-app.h,v 1.4 1999/07/02 17:07:10 haoboy Exp $

#ifndef ns_media_app_h
#define ns_media_app_h

#include <stdlib.h>
#include <tcl.h>
#include "config.h"
#include "agent.h"
#include "app.h"
#include "webcache/http-aux.h"
#include "rap/rap.h"


class DoubleListElem {
public:
	DoubleListElem() : prev_(0), next_(0) {}

	DoubleListElem* next() const { return next_; }
	DoubleListElem* prev() const { return prev_; }

	virtual void detach() {
		if (prev_ != 0) prev_->next_ = next_;
		if (next_ != 0) next_->prev_ = prev_;
		prev_ = next_ = 0;
	}
	// Add new element s before this one
	virtual void insert(DoubleListElem *s) {
		s->next_ = this;
		s->prev_ = prev_;
		if (prev_ != 0) prev_->next_ = s;
		prev_ = s;
	}
	// Add new element s after this one
	virtual void append(DoubleListElem *s) {
		s->next_ = next_;
		s->prev_ = this;
		if (next_ != 0) next_->prev_ = s;
		next_ = s;
	}

private:
	DoubleListElem *prev_, *next_;
};

class DoubleList {
public:
	DoubleList() : head_(0), tail_(0) {}
	virtual void destroy();
	DoubleListElem* head() { return head_; }
	DoubleListElem* tail() { return tail_; }

	void detach(DoubleListElem *e) {
		if (head_ == e)
			head_ = e->next();
		if (tail_ == e)
			tail_ = e->prev();
		e->detach();
	}
	void insert(DoubleListElem *src, DoubleListElem *dst) {
		dst->insert(src);
		if (dst == head_)
			head_ = src;
	}
	void append(DoubleListElem *src, DoubleListElem *dst) {
		dst->append(src);
		if (dst == tail_)
			tail_ = src;
	}
protected:
	DoubleListElem *head_, *tail_;
};


class HttpMediaData;

// Fixed length segment in a multimedia stream
class MediaSegment : public DoubleListElem {
public: 
	MediaSegment() : start_(0), end_(0), flags_(0) {}
	MediaSegment(int start, int end) : start_(start),end_(end),flags_(0) {}
	MediaSegment(const HttpMediaData& d);
	MediaSegment(const MediaSegment& s) {
		// XXX Don't copy pointers (prev_,next_)?
		start_ = s.start_;
		end_ = s.end_;
		flags_ = s.flags_;
	}

	int start() const { return start_; }
	int end() const { return end_; }
	int datasize() const { return end_ - start_; }
	MediaSegment* next() const { 
		return (MediaSegment *)DoubleListElem::next(); 
	}
	MediaSegment* prev() const { 
		return (MediaSegment *)DoubleListElem::prev(); 
	}

	void set_start(int d) { start_ = d; }
	void set_end(int d) { end_ = d; }
	void set_datasize(int d) { end_ = start_ + d; }
	// Advance the segment by size
	void advance(int size) { start_ += size, end_ = start_ + size; }

	int in(const MediaSegment& s) const {
		return ((start_ >= s.start_) && (end_ <= s.end_));
	}
	int before(const MediaSegment& s) const {
		return (end_ <= s.start_);
	}
	int overlap(const MediaSegment& s) const {
		assert(s.start_ <= s.end_);
		return ((s.end_ >= start_) && (s.start_ <= end_));
	}
	// Return the overlapping size between the two
	int merge(const MediaSegment& s) {
		if ((s.end_ < start_) || (s.start_ > end_))
			// No overlap
			return 0;
		int size = datasize() + s.datasize();
		if (s.start_ < start_) start_ = s.start_;
		if (s.end_ > end_) end_ = s.end_;
		assert(size >= datasize());
		return (size - datasize());
	}
	// Return the amount of data evicted
	int evict_tail(int sz) {
		// Adjust both the size and playout time
#ifdef MCACHE_DEBUG
		fprintf(stderr, "evicted (%d, %d) ", end_-sz, end_);
#endif
		if (datasize() >= sz) 
			end_ -= sz;
		else {
			sz = datasize();
			end_ = start_;
		}
		return sz;
	}
	// Return the amount of data evicted
	int evict_head(int sz) {
		// Adjust both the size and playout time
		if (datasize() >= sz) 
			start_ += sz;
		else {
			sz = datasize();
			end_ = start_;
		}
		return sz;
	}
	int is_empty() const { return end_ == start_; }

	// Whether this is the last segment of the available data
	int is_last() const { return flags_ == 1; }
	void set_last() { flags_ = 1; }

private: 
	int start_;
	int end_;
	int flags_;
};

// Maintains received segments of every layer
class MediaSegmentList : public DoubleList {
public:
	MediaSegmentList() : length_(0), DoubleList() {}

	int length() const { return length_; }
	void add(const MediaSegment& s);
	int in(const MediaSegment& s);
	MediaSegment get_nextseg(const MediaSegment& s);
	int evict_tail(int size);
	int evict_head(int size);
	int evict_head_offset(int offset);
	MediaSegmentList check_holes(const MediaSegment& s);

	char* dump2buf();
	virtual void destroy() {
		length_ = 0;
		DoubleList::destroy();
	}

	// Debug only
	void print(void);
	int getsize();

protected:
	void merge_seg(MediaSegment* seg);

private:
	int length_;
};


// Represents a multimedia segment transmitted through the network
class HttpMediaData : public HttpData {
private:
	char page_[HTTP_MAXURLLEN];	// Page ID
	char sender_[HTTP_MAXURLLEN];	// Sender name
	int layer_;			// Layer id. 0 if no layered encoding
	int st_;			// Segment start time
	int et_; 			// Segment end time
	int flags_; 			// flags: end of all data, etc.
public:
	struct hdr : HttpData::hdr {
		char sender_[HTTP_MAXURLLEN];
		char page_[HTTP_MAXURLLEN];
		int layer_;
		int st_, et_;
		int flags_;
	};

public:
	HttpMediaData(const char* sender, const char* name, int layer, 
		      int st, int et);
	HttpMediaData(char *b) : HttpData(b) {
		layer_ = ((hdr *)b)->layer_;
		st_ = ((hdr *)b)->st_;
		et_ = ((hdr *)b)->et_;
		flags_ = ((hdr *)b)->flags_;
		strcpy(page_, ((hdr *)b)->page_);
		strcpy(sender_, ((hdr *)b)->sender_);
	}

	virtual int hdrlen() const { return sizeof(hdr); }
	virtual int size() const { return hdrlen(); }
	int st() const { return st_; }
	int et() const { return et_; }
	int datasize() const { return et_ - st_; }
	int layer() const { return layer_; }
	const char* page() const { return page_; }
	const char* sender() const { return sender_; }

	// flags
	// MD_LAST: last segment of this layre
	// MD_FINISH: completed the entire stream
	enum {MD_LAST = 1, MD_FINISH = 2}; 

	// Whether this is the last data segment of the layer
	int is_last() const { return (flags_ & MD_LAST); }
	void set_last() { flags_ |= MD_LAST; }
	int is_finished() const { return (flags_ & MD_FINISH); }
	void set_finish() { flags_ |= MD_FINISH; }

	virtual void pack(char *buf) const { 
		HttpData::pack(buf);
		((hdr *)buf)->layer_ = layer_;
		((hdr *)buf)->st_ = st_;
		((hdr *)buf)->et_ = et_;
		((hdr *)buf)->flags_ = flags_;
		strcpy(((hdr *)buf)->page_, page_);
		strcpy(((hdr *)buf)->sender_, sender_);
	}
};


const int MEDIAREQ_GETSEG   	= 1;
const int MEDIAREQ_CHECKSEG 	= 2;

const int MEDIAREQ_SEGAVAIL 	= 3;
const int MEDIAREQ_SEGUNAVAIL 	= 4;

// It provides a MediaApp two types of requests: check data availability and 
// request data. It won't be serialized.
class MediaRequest : public AppData {
private:
	int request_; 			// Request code
	char name_[HTTP_MAXURLLEN];	// Page URL
	int layer_;			// Layer ID
	u_int st_;			// Start playout time
	u_int et_;			// End playout time
	Application* app_;		// Calling application
public:
	MediaRequest(int rc) : AppData(MEDIA_REQUEST), request_(rc) {}
	MediaRequest(const MediaRequest& r) : AppData(MEDIA_REQUEST) {
		request_ = r.request();
		st_ = r.st();
		et_ = r.et();
		layer_ = r.layer();
		strcpy(name_, r.name());
	}

	int request() const { return request_; }
	int st() const { return st_; }
	int et() const { return et_; }
	int datasize() const { return et_ - st_; }
	int layer() const { return layer_; }
	const char* name() const { return name_; }
	Application* app() const { return app_; }

	// These functions allow the caller to fill in only the 
	// necessary fields
	void set_st(int d) { st_ = d; }
	void set_et(int d) { et_ = d; }
	void set_datasize(int d) { et_ = st_ + d; }
	void set_name(const char *s) { strcpy(name_, s); }
	void set_layer(int d) { layer_ = d; }
	void set_app(Application* a) { app_ = a; }
};



// Maximum number of layers for all streams in the simulation
const int MAX_LAYER = 10;

// XXX A media app is only responsible for transmission of a single 
// media stream!! Once the transmission is done, the media app will be 
// deleted.
class MediaApp : public Application {
public:
	MediaApp(const char* page);

	// Interfaces used by UDP/TCP transports
	// XXX Not supported right now.
	virtual void recv(int nbytes);
	virtual void send(int nbytes);

	// Interfaces used by RAP
	virtual void send(int nbytes, const AppData* data) {
		// Ask the underlying agent to send data.
		agent_->send(nbytes, data);
	}
	virtual void recv(int size, char* data) {
		process_data(size, data);
	}

	void set_page(const char* pg) { strcpy(page_, pg); }
	virtual AppData* get_data(int& size, const AppData* req_data = 0);
	virtual void process_data(int size, char* data) {
		send_data(size, data);
	}

	void log(const char* fmt, ...);
	int command(int argc, const char*const* argv);

protected:
	// We use start() and stop() to 
	virtual void start();
	virtual void stop();

	// Helper function
	RapAgent* rap() { return (RapAgent*)agent_; }

	int seg_size_; 			// data segment size
	char page_[HTTP_MAXURLLEN];
	MediaSegment data_[MAX_LAYER];	// Pointer to next data to be sent
	Tcl_Channel log_; 		// log file

	// XXX assuming all streams in the simulation have the same
	// number of layers.
	int num_layer_;
private:
	int last_layer_;
};



class QA; 

class QATimer : public TimerHandler {
public:
	QATimer(QA *a) : TimerHandler() { a_ = a; }
protected:
	virtual void expire(Event *e);
	QA *a_;
};

class QA : public MediaApp { 
public:
	QA(const char *page);
	virtual ~QA();

	virtual AppData* get_data(int& size, const AppData* req_data = 0);
	void UpdateState();
	double UpdateInterval() { return rap()->srtt(); } 

	// Static members used to indicate segment availability
	static AppData MEDIASEG_AVAIL_, MEDIASEG_UNAVAIL_;

protected:
	virtual int command(int argc, const char*const* argv);

	// Helper functions
	void check_availability(int layer, const MediaSegment& s);
	RapAgent* rap() { return (RapAgent*)agent_; }

	// Misc helpers
	inline double MWM(double srtt) {
		return 2 * LAYERBW_ * srtt;
	}
	inline double rate() { 
		return (double)seg_size_ / rap()->ipg();
	}

	// Calculate area of a triangle for a given side and slope
	inline double BufNeed(float side, float slope) {
		return (side <= 0) ? 0.0 : ((side*side) / (2.0*slope));
	}
	int AllZero(double *arr, int len);
	double TotalBuf(int n, double *buffer);
	AppData* output(int& size, int layer);
	void DumpInfo(float t, float last_t, float rate, 
		      float avgrate, float srtt);
	double bufOptScen1(int layer, int layers, float currrate, 
			   float slope, int backoffs);
	double bufOptScen2(int layer, int layers, float currrate, 
			   float slope, int backoffs);
	void drain_buf(double* DrainArr, float bufToDrain, 
		       double* FinalDrainArray, double* bufAvail, 
		       int layers, float rate, float srtt);
	void DrainPacket(double bufToDrain, double* FinalDrainArray, 
			 int layers, double rate, double srtt, 
			 double* FinalBuffer);

	// Ack feedback information
	void DrainBuffers();

	// Debugging output
	void debug(const char* fmt, ...);
	void panic(const char* fmt, ...);

	// Data members
	int layer_;
	double    playTime_; // playout time of the receiver
	double    startTime_; // Absoloute time when playout started

	// Internal state info for QA
	double   buffer_[MAX_LAYER];
	double   drained_[MAX_LAYER]; 
	double   bw_[MAX_LAYER];
	int	 playing_[MAX_LAYER]; 
	int	 sending_[MAX_LAYER];
	QATimer* updTimer_;

	// Average transmission rate and its moving average weight
	// Measured whenever a segment is sent out (XXX why??)
	double   avgrate_; 
	double   rate_weight_;

	// Variables related to playout buffer management
	int      poffset_;  /* The playout offset: estimation of client 
			       playout time */
	// Linked list keeping track of holes between poffset_ and current
	// transmission pointer
	MediaSegmentList outlist_[MAX_LAYER];
	// The end offset of the prefetch requests. Used to avoid redundant
	// prefetching requests
	int pref_[MAX_LAYER];

	// OTcl-bound variables
	int debug_;  		// Turn on/off debug output
	double pref_srtt_;	// Prefetching SRTT
	int LAYERBW_;
	int MAXACTIVELAYERS_;
	double SRTTWEIGHT_;
	int SMOOTHFACTOR_;
	int MAXBKOFF_;
}; 

#endif // ns_media_app_h
