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
// Multimedia caches
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/mcache.h,v 1.6 1999/11/18 23:14:34 haoboy Exp $

#ifndef ns_mcache_h
#define ns_mcache_h

#include "config.h"
#include "agent.h"
#include "pagepool.h"
#include "http.h"
#include "rap/media-app.h"


//----------------------------------------------------------------------
// Priority list for hit counts at each layer
//----------------------------------------------------------------------
class HitCount : public DoubleListElem {
public:
	HitCount(ClientPage *pg, short layer) : 
		DoubleListElem(), pg_(pg), layer_(layer), hc_(0) {}

	void update(float hc) { hc_ += hc; }
	float hc() { return hc_; }
	void reset() { hc_ = 0; }
	ClientPage* page() { return pg_; }
	short layer() { return layer_; }

	HitCount* next() { return (HitCount *)DoubleListElem::next(); }
	HitCount* prev() { return (HitCount *)DoubleListElem::prev(); }

private:
	ClientPage* pg_; 	// page
	short layer_; 		// layer id
	float hc_;		// hit count
};

class HitCountList : public DoubleList {
public:
	HitCountList() : DoubleList() {}

	void update(HitCount *h); 	// Update layer hit count
	void add(HitCount *h);		// Add a new layer

	HitCount* detach_tail() {
		HitCount* tmp = (HitCount *)tail_;
		if (tmp) {
			tail_ = tail_->prev();
			tmp->detach();
		}
		return tmp;
	}

	// Debug only
	void print();
	void check_integrity(); 
};



//----------------------------------------------------------------------
// Multimedia web objects
//----------------------------------------------------------------------
class MediaPage : public ClientPage {
public:
	MediaPage(const char *n, int s, double mt, double et, double a, int l);
	virtual ~MediaPage();

	virtual WebPageType type() const { return MEDIA; }
	virtual void print_info(char *buf);
	int num_layer() const { return num_layer_; }

	HitCount* get_hit_count(int layer) { 
		assert((layer >= 0) && (layer < num_layer_));
		return hc_[layer]; 
	}
	void hit_layer(int layer) {
		assert((layer >= 0) && (layer < num_layer_));
		hc_[layer]->update((double)(layer_[layer].length()*num_layer_)
				   / (double)size_); 
	}

	int layer_size(int layer) { 
		assert((layer >= 0) && (layer < num_layer_));
		return layer_[layer].length();
	}
	void add_segment(int layer, const MediaSegment& s);
	int evict_tail_segment(int layer, int size);

	void add_layer(int layer) {
		assert((layer >= 0) && (layer < num_layer_));
		num_layer_ = (num_layer_ < layer) ? layer : num_layer_;
	}
	char* print_layer(int layer) {
		assert((layer >= 0) && (layer < num_layer_));
		return layer_[layer].dump2buf();
	}
	MediaSegmentList is_available(int layer, const MediaSegment& s) {
		assert((layer >= 0) && (layer < MAX_LAYER));
		return layer_[layer].check_holes(s);
	}
	// Return a media segment which is the closest one after 's'.
	// Used by MediaApps to request data.
	// Do NOT check if layer >= num_layer_. If it's empty, 
	// an empty MediaSegment will be returned. 
	MediaSegment next_available(int layer, const MediaSegment& s) {
		assert((layer >= 0) && (layer < MAX_LAYER));
		return layer_[layer].get_nextseg(s);
	}
	MediaSegment next_overlap(int layer, const MediaSegment& s) {
		assert((layer >= 0) && (layer < MAX_LAYER));
		MediaSegment s1 = layer_[layer].get_nextseg(s);
		if ((s1.end() <= s.start()) || (s1.start() >= s.end())) {
			MediaSegment s;
			if (s1.is_last())
				s.set_last();
			return s;
		} else
			return s1;
	}

	enum {FETCHLOCK = 1, XMITLOCK = 2};

	// 1st type of lock: it is being fetched from the server
	void lock() { locked_ |= FETCHLOCK; }
	void unlock() { locked_ &= ~FETCHLOCK; }
	int is_locked() { return (locked_ & FETCHLOCK); }
	// 2nd type of lock: it is being transmitted to a client
	void tlock() { locked_ |= XMITLOCK; }
	void tunlock() { locked_ &= ~XMITLOCK; }
	int is_tlocked() { return (locked_ & XMITLOCK); }

	// Whether all layers are marked as "completed"
	int is_complete(); 
	// Set all layers as "completed"
	void set_complete();

	// Given the number of layers, evenly distribute the size among all
	// layers and create all segments.
	void create();
	int realsize() const { return realsize_; }

protected:
	void set_complete_layer(int layer) {
		flags_[layer] = 1;
	}
	int is_complete_layer(int layer) {
		return flags_[layer] == 1; 
	}

	MediaSegmentList layer_[MAX_LAYER];
	int flags_[MAX_LAYER]; // If 1, marks the layer as completed
	HitCount* hc_[MAX_LAYER];
	int num_layer_; 
	int locked_;  // If 1, then no segment can be replaced
	int realsize_; // The size of stream data in this page.
};

// XXX Later we should add a generic replacement algorithm hook into 
// ClientPagePool. But now we'll leave it there and get this media 
// page pool work first. 

// ClientPagePool enhanced with support for multimedia objects, and 
// with replacement algorithms
class MClientPagePool : public ClientPagePool {
public:
	MClientPagePool();

	virtual ClientPage* enter_page(int argc, const char*const* argv);
	virtual int remove_page(const char *name);
	virtual int force_remove(const char *name);

	int add_segment(const char *name, int layer, const MediaSegment& s);
	void hc_update(const char *name, int max_layer);

	int maxsize() { return max_size_; }
	int usedsize() { return used_size_; }

	void fill_page(const char* pgname);

	// Debug only
	void dump_hclist() { hclist_.print(); }

protected:
	virtual int command(int argc, const char*const* argv);
	virtual int cache_replace(ClientPage* page, int size);

	// Fine-grain replacement
	int repl_finegrain(ClientPage* p, int size);
	int repl_atomic(ClientPage* p, int size);

	// XXX Should change to quad_t, or use MB as unit
	int max_size_; 		// PagePool size
	int used_size_;		// Available space size
	HitCountList hclist_; 
	// Replacement style
	enum { FINEGRAIN, ATOMIC } repl_style_;
};

// Provide page data and generate requests for servers and clients
class MediaPagePool : public PagePool {
public:
	MediaPagePool();
protected:
	virtual int command(int argc, const char*const* argv);
	int layer_;	// Number of layers of pages
	int *size_; 	// Page sizes
	RandomVariable *rvReq_;
};



//--------------------
// Multimedia caches
//--------------------

// Plain multimedia cache, which can interface with RapAgent
class MediaCache : public HttpCache {
public:
	MediaCache();
	~MediaCache();

	// Handle app-specific data in C++
	virtual void process_data(int size, AppData* data);
	// Handle data request from RAP
	virtual AppData* get_data(int& size, AppData* data);

protected:
	virtual int command(int argc, const char*const* argv);

	// Do we need to maintain links to MediaApp?? I doubt not
	// MediaApp* mapp_; // An array of incoming/outgoing RAP agents
	MClientPagePool* mpool() { return (MClientPagePool *)pool_; }

	// Information and statistics related to clients
	struct RegInfo {
		RegInfo() : client_(NULL), hl_(-1) {
			memset(pb_, 0, sizeof(unsigned int)*MAX_LAYER);
			memset(db_, 0, sizeof(unsigned int)*MAX_LAYER);
			memset(eb_, 0, sizeof(unsigned int)*MAX_LAYER);
		}
		~RegInfo() {
			for (int i = 0; i < MAX_LAYER; i++)
				pref_[i].destroy();
		}

		char name_[20];
		HttpApp* client_;
		int hl_;		// Highest layer this client has asked
		// Prefetched bytes
		unsigned int pb_[MAX_LAYER]; 
		// Prefetched bytes that were delivered
		unsigned int eb_[MAX_LAYER];
		// Total delivered bytes
		unsigned int db_[MAX_LAYER];
		MediaSegmentList pref_[MAX_LAYER];

		// Return the number of prefetched bytes in the given segment
		void add_pref(int layer, const MediaSegment& s) {
			assert((layer >= 0) && (layer < MAX_LAYER));
			pref_[layer].add(s);
		}
		int pref_size(int layer, const MediaSegment &s) const { 
			assert((layer >= 0) && (layer < MAX_LAYER));
			return pref_[layer].overlap_size(s);
		}
	};
	Tcl_HashTable *cmap_;	// client map

	// Prefetching/No-prefetching/Offline-prefetching flag
	enum {NOPREF, ONLINE_PREF, OFFLINE_PREF} pref_style_;
};


//----------------------------------------------------------------------
// Multimedia web client
//----------------------------------------------------------------------
class MediaClient : public HttpClient {
public:
	MediaClient() : HttpClient() {}
	virtual void process_data(int size, AppData* data);
	virtual int command(int argc, const char*const* argv);
private:
	MClientPagePool* mpool() { return (MClientPagePool *)pool_; }
};



// Helper data structure

template <class T> class Queue; // forward declaration

template <class T> class QueueElem {
public:
        QueueElem(T* d) : next_(0), data_(d) {}

        QueueElem<T>* next() const { return next_; }
        T* data() const { return data_; }
        void detachData() { data_ = NULL; }
        void append(QueueElem<T>* e) {
                e->next_ = next_;
                next_ = e;
        }

protected:
        QueueElem<T>* next_;
        T* data_;
	friend class Queue<T>; 
};

template <class T> class Queue {
public:
        Queue() : head_(0), tail_(0), size_(0) {}
        virtual ~Queue() {
                QueueElem<T> *p = head_, *q;
                while (p != NULL) {
                        q = p;
                        p = p->next();
                        delete q;
                }
                head_ = NULL;
        }
        virtual void reset() {
                QueueElem<T> *p = head_, *q;
                while (p != NULL) {
                        q = p;
                        p = p->next();
                        delete q;
                }
                head_ = NULL;
        }
        virtual void destroy() {
                QueueElem<T> *p = head_, *q;
                while (p != NULL) {
                        q = p;
                        p = p->next();
                        delete q->data();
                        delete q;
                }
                head_ = NULL;
        }

        void enqueue(QueueElem<T> *e) {
                if (tail_ == 0)
                        head_ = tail_ = e;
                else {
                        tail_->append(e);
			tail_ = e;
		}
                size_++;
        }
        QueueElem<T>* dequeue() {
                QueueElem<T> *p = head_;
                if (head_ != 0) 
                        head_ = head_->next();
                if (head_ == 0)
                        tail_ = 0;
                p->next_ = 0;
                size_--;
		if (size_ == 0) 
			assert((head_ == 0) && (tail_ == 0));
                return p;
        }
	void detach(QueueElem<T>* e) {
		assert(head_ != 0);
		if (head_ == e) {
			dequeue();
			return;
		}
		QueueElem<T> *p = head_;
		while (p != NULL) {
			if (p->next_ != e)
				p = p->next_;
			else
				break;
		}
		assert(p != NULL);
		p->next_ = e->next_;
		if (tail_ == e)
			tail_ = p;
		size_--;
		if (size_ == 0) 
			assert((head_ == 0) && (tail_ == 0));
	}
        QueueElem<T>* getHead() { return head_; }
	int is_empty() const { return (size_ == 0); }
	int size() const { return size_; }

protected:
        QueueElem<T> *head_, *tail_;
        int size_;
};



//----------------------------------------------------------------------
// Multimedia server
//----------------------------------------------------------------------
class MediaServer : public HttpServer {
public:
	MediaServer();
	~MediaServer();
	virtual AppData* get_data(int& size, AppData* d);

protected:
	virtual int command(int argc, const char*const* argv);
	MediaSegment get_next_segment(MediaRequest *r, Application*& ci);

	// Prefetching list
	struct RegInfo {
		char name_[20];
		HttpApp* client_;
	};
	struct PrefInfo {
		MediaSegmentList* sl_;
		Application* conid_;
	};
	typedef Queue<PrefInfo> PrefInfoQ;
	typedef QueueElem<PrefInfo> PrefInfoE;
	PrefInfoE* find_prefinfo(PrefInfoQ* q, Application* conid) {
		for (PrefInfoE *e = q->getHead(); e != NULL; e = e->next())
			if (e->data()->conid_ == conid)
				return e;
		return NULL;
	}

	Tcl_HashTable *pref_; // Mapping <cache>:<pagenum> to PrefInfoQ
	Tcl_HashTable *cmap_; // Mapping MediaApps to clients

	PrefInfoQ* get_piq(const char* pgname, HttpApp* client) {
		PageID id;
		ClientPage::split_name(pgname, id);
		int tmp[2];
		tmp[0] = (int)(client);
		tmp[1] = id.id_;
		Tcl_HashEntry* he = 
			Tcl_FindHashEntry(pref_, (const char*)tmp);
		if (he == NULL) 
			return NULL;
		return (PrefInfoQ*)Tcl_GetHashValue(he);
	}
	RegInfo* get_reginfo(Application* app) {
		Tcl_HashEntry *he = 
			Tcl_FindHashEntry(cmap_, (const char *)app);
		if (he == NULL) {
			fprintf(stderr, "Unknown connection!\n");
			abort();
		} 
		return (RegInfo *)Tcl_GetHashValue(he);
	}
};


#endif // ns_mcache_h
