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
// Multimedia cache implementation
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/mcache.cc,v 1.2 1999/07/02 21:02:19 haoboy Exp $

#include <assert.h>
#include <stdio.h>

#include "rap/media-app.h"
#include "mcache.h"


MediaPage::MediaPage(const char *n, int s, double mt, double et, 
		     double a, int l) :
	ClientPage(n, s, mt, et, a), num_layer_(l), locked_(0), realsize_(0)
{
	for (int i = 0; i < num_layer_; i++) {
		hc_[i] = new HitCount(this, i);
		flags_[i] = 0;
	}
}

MediaPage::~MediaPage()
{
	for (int i = 0; i < num_layer_; i++) {
		// XXX This does not reset the head_ and tail_ correctly!! 
		// Must correct these !!!!
		// hc_[i]->detach();
		assert((hc_[i]->prev() == NULL) && (hc_[i]->next() == NULL));
		delete hc_[i];
	}
}

void MediaPage::print_info(char *buf) 
{
	ClientPage::print_info(buf);
	buf += strlen(buf);
	sprintf(buf, " pgtype MEDIA layer %d", num_layer_);
}

// Make the page full with stream data
void MediaPage::create()
{
	assert((num_layer_ >= 0) && (num_layer_ < MAX_LAYER));
	int i, sz = size_ / num_layer_;
	for (i = 0; i < num_layer_; i++) {
		add_segment(i, MediaSegment(0, sz));
		set_complete_layer(i);
	}
	realsize_ = size_;
}

void MediaPage::add_segment(int layer, const MediaSegment& s) 
{
	assert((layer >= 0) && (layer < MAX_LAYER));
	layer_[layer].add(s);
	realsize_ += s.datasize();
	if (s.is_last())
		set_complete_layer(layer);
}

int MediaPage::is_complete()
{
	// Consider a page finished when all NON-EMPTY layers are 
	// marked as "completed"
	for (int i = 0; i < num_layer_; i++)
		if (!is_complete_layer(i) && (layer_[i].length() > 0))
			return 0;
	return 1;
}

void MediaPage::set_complete()
{
	for (int i = 0; i < num_layer_; i++)
		set_complete_layer(i);
}

// Used for cache replacement
int MediaPage::evict_tail_segment(int layer, int size)
{
	if (is_locked() || is_tlocked())
		return 0;

	assert((layer >= 0) && (layer < MAX_LAYER));
#if 1
	char buf[20];
	name(buf);
	fprintf(stderr, "Page %s evicted layer %d: ", buf, layer);
#endif
	int sz = layer_[layer].evict_tail(size);
	realsize_ -= sz;

	fprintf(stderr, "\n");

	return sz;
}


//----------------------------------------------------------------------
// Classes related to a multimedia client page pool
//
// HitCountList and MClientPagePool
//----------------------------------------------------------------------

void HitCountList::update(HitCount *h)
{
	HitCount *tmp = h->prev();
	if ((tmp != NULL) && (tmp->hc() < h->hc())) {
		// Hit count increased, need to move this one up
		detach(h);
		while ((tmp != NULL) && (tmp->hc() < h->hc())) {
			if ((tmp->page() == h->page()) &&
			    (tmp->layer() < h->layer()))
				// XXX Don't violate layer encoding within the
				// same page!
				break;
			tmp = tmp->prev();
		}
		if (tmp == NULL) 
			// Insert it at the head
			insert(h, head_);
		else 
			append(h, tmp);
	} else if ((h->next() != NULL) && (h->hc() < h->next()->hc())) {
		// Hit count decreased, need to move this one down
		tmp = h->next();
		detach(h);
		while ((tmp != NULL) && (h->hc() < tmp->hc())) {
			if ((h->page() == tmp->page()) && 
			    (h->layer() < tmp->layer()))
				// XXX Don't violate layer encoding within 
				// the same page!
				break;
			tmp = tmp->next();
		}
		if (tmp == NULL)
			// At the tail
			append(h, tail_);
		else
			insert(h, tmp);
	}
	// We may end up with two cases here:
	//
	// (1) tmp->hc()>h->hc() && tmp->layer()<h->layer(). This is
	// the normal case, where both hit count ordering and layer 
	// ordering are preserved;
	//
	// (2) tmp->hc()>h->hc() && tmp->layer()>h->layer(). In this
	// case, we should move h BEFORE tmp so that the layer 
	// ordering is not violated. We basically order the list using 
	// layer number as primary key, and use hit count as secondary
	// key. 
	// Note that the hit count ordering is only violated when more packets 
	// in layer i are dropped than those in layer i+1.
}

// Check the integrity of the resulting hit count list
void HitCountList::check_integrity()
{
	HitCount *p = (HitCount*)head_, *q;
	while (p != NULL) {
		q = p->next();
		while (q != NULL) {
			// Check layer ordering 
			if ((p->page() == q->page()) && 
			    (p->layer() > q->layer())) {
				fprintf(stderr, "Wrong hit count list.\n");
				abort();
			}
			q = q->next();
		}
		p = p->next();
	}
}

void HitCountList::add(HitCount *h)
{
	HitCount *tmp = (HitCount*)head_;

	// XXX First, ensure that the layer ordering within the same page
	// is not violated!!
	while ((tmp != NULL) && (tmp->hc() > h->hc())) {
		if ((tmp->page() == h->page()) && (tmp->layer() > h->layer()))
			break;
		tmp = tmp->next();
	}
	// Then order according to layer number
	while ((tmp != NULL) && (tmp->hc() == h->hc()) && 
	       (tmp->layer() < h->layer()))
		tmp = tmp->next();

	if (tmp == NULL) {
		if (head_ == NULL) 
			head_ = tail_ = h;
		else
			append(h, tail_);
		return;
	} else if ((tmp == head_) && 
		   ((tmp->hc() < h->hc()) || (tmp->layer() > h->layer()))) {
		insert(h, head_);
		return;
	}

	// Now tmp->hc()<=h->hc(), or tmp->hc()>h->hc() but 
	// tmp->layer()>h->layer(), insert h BEFORE tmp
	insert(h, tmp);
}

// Debug only
void HitCountList::print() 
{
	HitCount *p = (HitCount *)head_;
	int i = 0;
	char buf[20];
	while (p != NULL) {
		p->page()->name(buf);
	        printf("(%s %d %f) ", buf, p->layer(), p->hc());
		if (++i % 4 == 0)
			printf("\n");
		p = p->next();
	}
	if (i % 4 != 0)
		printf("\n");
}

//------------------------------
// Multimedia client page pool
//------------------------------
static class MClientPagePoolClass : public TclClass {
public:
        MClientPagePoolClass() : TclClass("PagePool/Client/Media") {}
        TclObject* create(int, const char*const*) {
		return (new MClientPagePool());
	}
} class_mclientpagepool_agent;

MClientPagePool::MClientPagePool() : ClientPagePool()
{
#ifdef JOHNH_CLASSINSTVAR
#else /* ! JOHNH_CLASSINSTVAR */
	bind("max_size_", &max_size_);
#endif
	used_size_ = 0;
}

#ifdef JOHNH_CLASSINSTVAR
void MClientPagePool::delay_bind_init_all()
{
	delay_bind_init_one("max_size_");
	ClientPagePool::delay_bind_init_all();
}

int MClientPagePool::delay_bind_dispatch(const char *varName, 
					 const char *localName)
{
	DELAY_BIND_DISPATCH(varName, localName, "max_size_", 
			    delay_bind, &max_size_);
	return MClientPagePool::delay_bind_dispatch(varName, localName);
}
#endif


void MClientPagePool::hc_update(const char *name, int max_layer)
{
	MediaPage *pg = (MediaPage*)get_page(name);
	assert(pg != NULL);

	int i;
	HitCount *h;
	// First we update the hit count of each layer of the given page
	for (i = 0; i <= max_layer; i++)
		pg->hit_layer(i);
	// Then we update the position of these hit count records
	for (i = 0; i <= max_layer; i++) {
		h = pg->get_hit_count(i);
		hclist_.update(h);
	}
#if 1
	hclist_.check_integrity();
#endif
}

// Add a segment to an object, and adjust hit counts accordingly
// XXX Call cache replacement algorithm if necessary
int MClientPagePool::add_segment(const char* name, int layer, 
				 const MediaSegment& s)
{
	MediaPage* pg = (MediaPage *)get_page(name);
	if (pg == NULL)
		return -1;
	if (layer >= pg->num_layer()) {
		if (s.datasize() == 0)
			return 0;
		else {
			fprintf(stderr, 
				"MClientPagePool: cannot add a new layer.\n");
			abort();
		}
	}

	// Check space availability
	if (used_size_ + s.datasize() > max_size_) {
		cache_replace(pg, s.datasize());
#if 1
		fprintf(stderr, 
			"Replaced for page %s segment (%d %d) layer %d\n",
			name, s.start(), s.end(), layer);
#endif
	} else 
		// Add page
		used_size_ += s.datasize();

	// If this layer was not 'in' before, add its hit count block
	if (pg->layer_size(layer) == 0)
		hclist_.add(pg->get_hit_count(layer));

	// Add new segment
	pg->add_segment(layer, s);

	return 0;
}

ClientPage* MClientPagePool::enter_page(int argc, const char*const* argv)
{
	double mt = -1, et, age = -1, noc = 0;
	int size = -1, media_page = 0, layer;
	for (int i = 3; i < argc; i+=2) {
		if (strcmp(argv[i], "modtime") == 0)
			mt = strtod(argv[i+1], NULL);
		else if (strcmp(argv[i], "size") == 0) 
			size = atoi(argv[i+1]);
		else if (strcmp(argv[i], "age") == 0)
			age = strtod(argv[i+1], NULL);
		else if (strcmp(argv[i], "noc") == 0)
			// non-cacheable flag
			noc = 1;
		else if (strcmp(argv[i], "pgtype") == 0) {
			if (strcmp(argv[i+1], "MEDIA") == 0)
				media_page = 1;
		} else if (strcmp(argv[i], "layer") == 0)
			layer = atoi(argv[i+1]);
	}
	// XXX allow mod time < 0 and age < 0!
	if (size < 0) {
		fprintf(stderr, "%s: wrong page information %s\n",
			name_, argv[2]);
		return NULL;
	}
	et = Scheduler::instance().clock();
	ClientPage *pg;
	if (media_page)
		pg = new MediaPage(argv[2], size, mt, et, age, layer);
	else 
		pg = new ClientPage(argv[2], size, mt, et, age);
	if (add_page(pg) < 0) {
		delete pg;
		return NULL;
	}
	if (noc) 
		pg->set_uncacheable();
	if (media_page) 
		((MediaPage *)pg)->lock();
	return pg;
}

int MClientPagePool::cache_replace(ClientPage *, int size)
{
	// Traverse through hit count table, evict segments from the tail
	// of a layer with minimum hit counts
	HitCount *h, *p;
	int sz;

	// Repeatedly evict pages/segments until get enough space
	h = (HitCount*)hclist_.tail();
	while (h != NULL) {
		MediaPage *pg = (MediaPage *)h->page();
		// XXX Don't touch locked pages
		if (pg->is_tlocked() || pg->is_locked()) {
			h = h->prev();
			continue;
		}
		// Try to get "size" space by evicting other segments
		sz = pg->evict_tail_segment(h->layer(), size);
		// If we have not got enough space, we must have got rid of 
		// the entire layer
		assert((sz == size) || 
		       ((sz < size) && (pg->layer_size(h->layer()) == 0)));

		// If we don't have anything of this layer left, get rid of 
		// the hit count record. 
		// XXX Must do this BEFORE removing the page
		p = h;
		h = h->prev();
		if (pg->layer_size(p->layer()) == 0) {
			// XXX Should NEVER delete a hit count record!!
			hclist_.detach(p);
			p->reset();
		}
		// Furthermore, if the page has nothing left, get rid of it
		if (pg->realsize() == 0) {
			// If this page is gone, 
			char tmp[HTTP_MAXURLLEN];
			pg->name(tmp);
			remove_page(tmp);
		}
		// If we've got enough space, return; otherwise continue
		if (sz == size)
			return sz;
		size -= sz;	// Evict to fill the rest
	}
	fprintf(stderr, "cache_replace: cannot get enough space.\n");
	Tcl::instance().eval("[Test instance] flush-trace");
	abort();
	return 0; // Make msvc happy
}

int MClientPagePool::remove_page(const char *name)
{
	// XXX Bad hack. Needs to integrate this into ClientPagePool.
	ClientPage *pg = (ClientPage*)get_page(name);
	// We should not remove a non-existent page!!
	assert(pg != NULL);
	if (pg->type() == MEDIA)
		used_size_ -= ((MediaPage *)pg)->realsize();
	else if (pg->type() == HTML)
		used_size_ -= pg->size();
	return ClientPagePool::remove_page(name);
}


//------------------------------------------------------------
// MediaPagePool
// Generate requests and pages for clients and servers 
//------------------------------------------------------------
static class MediaPagePoolClass : public TclClass {
public:
        MediaPagePoolClass() : TclClass("PagePool/Media") {}
        TclObject* create(int, const char*const*) {
		return (new MediaPagePool());
	}
} class_mediapagepool_agent;

MediaPagePool::MediaPagePool() : PagePool()
{
	size_ = NULL;
	duration_ = 0;
	layer_ = 1;
}

// For now, only one page, fixed size, fixed layer
int MediaPagePool::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.resultf("%d", num_pages_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%.17g", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			// Generating requested page id
			if (rvReq_ == NULL) {
				tcl.add_errorf("no page id ranvar.");
				return TCL_ERROR;
			}
			int p = (int)rvReq_->value();
			assert((p >= 0) && (p < num_pages_));
			tcl.resultf("%d", p);
			return TCL_OK;
		} else if (strcmp(argv[1], "is-media-page") == 0) {
			// XXX Currently all pages are media pages. Should
			// be able to allow both normal pages and media pages
			// in the future
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-layer") == 0) {
			// XXX Currently all pages have the same number of 
			// layers. Should be able to change this in future.
			tcl.resultf("%d", layer_); 
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ = st + duration_;
			return TCL_OK;
		} else if (strcmp(argv[1], "set-duration") == 0) {
			// XXX Need this info to set page mod time!!
			duration_ = atoi(argv[2]);
			end_time_ = start_time_ + duration_;
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-init-modtime") == 0) {
			// XXX We are not interested in page consistency here,
			// so never change this page.
			tcl.resultf("%d", -1);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-size") == 0) {
			int pagenum = atoi(argv[2]);
			if (pagenum >= num_pages_) {
				tcl.add_errorf("Invalid page id %d", pagenum);
				return TCL_ERROR;
			}
			tcl.resultf("%d", size_[pagenum]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-layer") == 0) {
			layer_ = atoi(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-num-pages") == 0) {
			if (size_ != NULL) {
				tcl.add_errorf("can't change number of pages");
				return TCL_ERROR;
			}
			num_pages_ = atoi(argv[2]);
			size_ = new int[num_pages_];
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-req") == 0) {
			rvReq_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			// This should never be called, because we never
			// deals with page modifications!!
			fprintf(stderr, "%s: gen-modtime called!\n", name());
			abort();
		} else if (strcmp(argv[1], "set-pagesize") == 0) {
			// <pagepool> set-pagesize <pagenum> <size>
			int pagenum = atoi(argv[2]);
			if (pagenum >= num_pages_) {
				tcl.add_errorf("Invalid page id %d", pagenum);
				return TCL_ERROR;
			}
			size_[pagenum] = atoi(argv[3]);
			return TCL_OK;
		}
	}
	return PagePool::command(argc, argv);
}



//----------------------------------------------------------------------
// Multimedia web applications: cache, etc.
//----------------------------------------------------------------------

static class MediaCacheClass : public TclClass {
public:
	MediaCacheClass() : TclClass("Http/Cache/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaCache());
	}
} class_mediacache;

MediaCache::MediaCache() : HttpCache()
{
	cmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(cmap_, TCL_ONE_WORD_KEYS);
}

MediaCache::~MediaCache()
{
	if (cmap_) {
		Tcl_DeleteHashTable(cmap_);
		delete cmap_;
	}
}

AppData* MediaCache::get_data(int& size, const AppData* req)
{
	assert(req != NULL);
	if (req->type() != MEDIA_REQUEST) {
		return HttpApp::get_data(size, req);
	}

	MediaRequest *r = (MediaRequest *)req;

	if (r->request() == MEDIAREQ_GETSEG) {
		// Get a new data segment
		MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
		assert(pg != NULL);
		MediaSegment s1(r->st(), r->et());
		MediaSegment s2 = pg->next_overlap(r->layer(), s1);
		HttpMediaData *p;
		if (s2.datasize() == 0) {
			// No more data available for this layer, allocate
			// an ADU with data size 0 to signal the end
			// of transmission for this layer
			size = 0;
			p = new HttpMediaData(name(), r->name(),
					      r->layer(), 0, 0);
		} else {
			size = s2.datasize();
			p = new HttpMediaData(name(), r->name(),
					   r->layer(), s2.start(), s2.end());
		}
		// XXX If we are still receiving the stream, don't 
		// ever say that this is the last segment. If the 
		// page is not locked, it's still possible that we
		// return a NULL segment because the requested one
		// is not available. Don't set the 'LAST' flag in this 
		// case.
// 		if (!pg->is_locked() && s2.is_last()) {
		if (s2.is_last()) {
			p->set_last();
			if (!pg->is_locked() && (s2.datasize() == 0) && 
			    (r->layer() == 0)) 
				p->set_finish();
		}
		// Update the highest layer that this client has requested
		Tcl_HashEntry *he = 
			Tcl_FindHashEntry(cmap_, (const char *)(r->app()));
		assert(he != NULL);
		RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);
		if (ri->hl_ < r->layer())
			ri->hl_ = r->layer();
		return p;
	} else if (r->request() == MEDIAREQ_CHECKSEG) {
		// Check the availability of a new data segment
		// And do refetch if this is not available
		MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
		assert(pg != NULL);
		if (pg->is_locked()) 
			// If we are still transmitting it, don't prefetch
			return NULL;
		MediaSegmentList ul = pg->is_available(r->layer(),
				      MediaSegment(r->st(),r->et()));
		if (ul.length() == 0)
			// All segments are available
			return NULL;
		// Otherwise do prefetching on these "holes"
		char *buf = ul.dump2buf();
		Tcl::instance().evalf("%s pref-segment %s %d %s", 
				      name(), r->name(), r->layer(), buf);
		log("E PREF p %s l %d %s\n", r->name(), r->layer(), buf);
		delete []buf;
		ul.destroy();

		// Update the highest layer that this client has requested
		Tcl_HashEntry *he = 
			Tcl_FindHashEntry(cmap_, (const char *)(r->app()));
		assert(he != NULL);
		RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);
		if (ri->hl_ < r->layer())
			ri->hl_ = r->layer();
		return NULL;
	}
		
	fprintf(stderr, 
		"MediaCache %s gets an unknown MediaRequest type %d\n",
		name(), r->request());
	abort();
	return NULL; // Make msvc happy
}

// Add received media segment into page pool
void MediaCache::process_data(int size, char* data) 
{
	switch (AppData::type(data)) {
	case MEDIA_DATA: {
		HttpMediaData d(data);
		// Cache this segment, do replacement if necessary
		if (mpool()->add_segment(d.page(), d.layer(), 
					 MediaSegment(d)) == -1) {
			fprintf(stderr, "MediaCache %s gets a segment for an \
unknown page %s\n", name(), d.page());
			abort();
		}
		// XXX debugging only
		log("E RSEG p %s l %d s %d e %d z %d\n", 
		    d.page(), d.layer(), d.st(), d.et(), d.datasize());
		break;
	} 
	default:
		HttpCache::process_data(size, data);
	}
}

int MediaCache::command(int argc, const char*const* argv) 
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "dump-page") == 0) {
			// Dump segments of a given page
			ClientPage *p=(ClientPage*)mpool()->get_page(argv[2]);
			if (p->type() != MEDIA)
				// Do nothing for non-media pages
				return TCL_OK;
			MediaPage *pg = (MediaPage *)p;
			char *buf;
			for (int i = 0; i < pg->num_layer(); i++) {
				buf = pg->print_layer(i);
				if (strlen(buf) > 0)
					log("E SEGS p %s l %d %s\n", argv[2], 
					    i, buf);
				delete []buf;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "stream-received") == 0) {
			// We've got the entire page, unlock it
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[2]);
			assert(pg != NULL);
			pg->unlock();
			// XXX Should we clear all "last" flag of segments??
#if 0
			// Printing out current buffer status of the page
			char *buf;
			for (int i = 0; i < pg->num_layer(); i++) {
				buf = pg->print_layer(i);
				log("E SEGS p %s l %d %s\n", argv[2], i, buf);
				delete []buf;
			}
#endif
			// Show cache free size
			log("E SIZ n %d z %d t %d\n", mpool()->num_pages(),
			    mpool()->usedsize(), mpool()->maxsize());
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "register-client") == 0) {
			// <server> register-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int newEntry;
			Tcl_HashEntry *he = Tcl_CreateHashEntry(cmap_, 
					(const char *)a, &newEntry);
			if (he == NULL) {
				tcl.add_errorf("cannot create hash entry");
				return TCL_ERROR;
			}
			if (!newEntry) {
				tcl.add_errorf("duplicate connection");
				return TCL_ERROR;
			}
			RegInfo *p = new RegInfo;
			p->client_ = (HttpApp*)TclObject::lookup(argv[3]);
			assert(p->client_ != NULL);
			strcpy(p->name_, argv[4]);
			Tcl_SetHashValue(he, (ClientData)p);

			// Lock the page while transmitting it to a client
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[4]);
			assert((pg != NULL) && (pg->type() == MEDIA));
			pg->tlock();

			return TCL_OK;
		} else if (strcmp(argv[1], "unregister-client") == 0) {
			// <server> unregister-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(cmap_, (const char*)a);
			if (he == NULL) {
				tcl.add_errorf("cannot find hash entry");
				return TCL_ERROR;
			}
			RegInfo *ri = (RegInfo*)Tcl_GetHashValue(he);
			// Update hit count
			mpool()->hc_update(argv[4], ri->hl_);
#if 1
			printf("Cache %d hit counts: \n", id_);
			mpool()->dump_hclist();
#endif
			delete ri;
			Tcl_DeleteHashEntry(he);

			// Lock the page while transmitting it to a client
			MediaPage *pg = (MediaPage*)mpool()->get_page(argv[4]);
			assert((pg != NULL) && (pg->type() == MEDIA));
			pg->tunlock();

			return TCL_OK;
		}
	}

	return HttpCache::command(argc, argv);
}



//----------------------------------------------------------------------
// Media web client 
//   Use C++ interface to records quality of received stream.
// NOTE: 
//   It has OTcl inheritance, but no C++ inheritance!
//----------------------------------------------------------------------

static class HttpMediaClientClass : public TclClass {
public:
	HttpMediaClientClass() : TclClass("Http/Client/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaClient());
	}
} class_httpmediaclient;

// Records the quality of stream received
void MediaClient::process_data(int size, char* data)
{
	assert(data != NULL);

	switch (AppData::type(data)) {
	case MEDIA_DATA: {
		HttpMediaData d(data);
		// XXX Don't pass any data to page pool!!
// 		if (mpool()->add_segment(d.page(), d.layer(), 
// 					 MediaSegment(d)) == -1) {
// 			fprintf(stderr, "MediaCache %s gets a segment for an \
// unknown page %s\n", name(), d.page());
// 			abort();
// 		}
		// Note: we store the page only to produce some statistics
		// later so that we need not do postprocessing of traces.
		log("C RSEG p %s l %d s %d e %d z %d\n", 
		    d.page(), d.layer(), d.st(), d.et(), d.datasize());
		break;
	}
	default:
		HttpClient::process_data(size, data);
	}
}

int MediaClient::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "stream-received") == 0) {
			// XXX This is the place to do statistics collection
			// about quality of received stream.
			//
			// then delete the stream from buffer
			mpool()->remove_page(argv[2]);
			return TCL_OK;
		}
	}
	return HttpClient::command(argc, argv);
}



//----------------------------------------------------------------------
// Multimedia web server
//----------------------------------------------------------------------

static class MediaServerClass : public TclClass {
public:
	MediaServerClass() : TclClass("Http/Server/Media") {}
	TclObject* create(int, const char*const*) {
		return (new MediaServer());
	}
} class_mediaserver;

MediaServer::MediaServer() : HttpServer() 
{
	pref_ = new Tcl_HashTable;
	// Map <App,PageID> to PartialMediaPage
	Tcl_InitHashTable(pref_, 2);
	cmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(cmap_, TCL_ONE_WORD_KEYS);
}

MediaServer::~MediaServer() 
{
	if (pref_ != NULL) {
		Tcl_DeleteHashTable(pref_);
		delete pref_;
	}
	if (cmap_ != NULL) {
		Tcl_DeleteHashTable(cmap_);
		delete cmap_;
	}
}

// Return the next segment to be sent to a particular application
MediaSegment MediaServer::get_next_segment(MediaRequest *r)
{
	MediaPage* pg = (MediaPage*)pool_->get_page(r->name());
	assert(pg != NULL);

	// XXX Extremely hacky way to map media app names to 
	// HTTP connections. Should maintain another hash table for this.
	Tcl_HashEntry *he = Tcl_FindHashEntry(cmap_, (const char *)(r->app()));
	if (he == NULL) {
		fprintf(stderr, "Unknown connection!\n");
		abort();
	} 
	RegInfo *ri = (RegInfo *)Tcl_GetHashValue(he);
	assert(ri != NULL);
	PageID id;
	ClientPage::split_name(r->name(), id);
	int tmp[2];
	tmp[0] = (int)(ri->client_);
	tmp[1] = id.id_;
	he = Tcl_FindHashEntry(pref_, (const char *)tmp);
	if (he == NULL) {
		// We are not on the prefetching list
		MediaSegment s1(r->st(), r->et());
		return pg->next_overlap(r->layer(), s1);
	}

	// Send a segment from the prefetching list. Only use the data size
	// included in the request.
	MediaSegmentList *p = (MediaSegmentList *)Tcl_GetHashValue(he);
	// Find one available segment in prefetching list if there is none
	// in the given layer
	int l = r->layer(), i = 0;
	MediaSegment res;
	while ((res.datasize() == 0) && (i < pg->num_layer())) {
		// next() doesn't work. Need a method that returns the 
		// *FIRST* non-empty segment which satisfies the size 
		// constraint.
		res = p[l].get_nextseg(MediaSegment(0, r->datasize()));
		i++;
		l = (l+1) % pg->num_layer();
	}
	if (res.datasize() > 0) {
		// XXX We may end up getting data from another layer!!
		l = (l-1+pg->num_layer()) % pg->num_layer();
		if (l != r->layer())
			r->set_layer(l);
		// We may not be able to get the specified data size, due 
		// to arbitrary stream lengths
		//assert(res.datasize() == r->datasize());
		p[r->layer()].evict_head(r->datasize());
	}
	return res;
}

// Similar to MediaCache::get_data(), but ignore segment availability checking
AppData* MediaServer::get_data(int& size, const AppData *req)
{
	assert((req != NULL) && (req->type() == MEDIA_REQUEST));
	MediaRequest *r = (MediaRequest *)req;

	if (r->request() == MEDIAREQ_GETSEG) {
		// Get a new data segment
		MediaSegment s2 = get_next_segment(r);
		HttpMediaData *p;
		if (s2.datasize() == 0) {
			// No more data available for this layer, most likely
			// it's because this layer is finished.
			size = 0;
			p = new HttpMediaData(name(), r->name(),
					      r->layer(), 0, 0);
		} else {
			size = s2.datasize();
			p = new HttpMediaData(name(), r->name(),
					   r->layer(), s2.start(), s2.end());
#if 0
			fprintf(stderr, "Server %d sends data (%d,%d,%d)\n",
				id_, r->layer(), s2.start(), s2.end());
#endif			
		}
		if (s2.is_last()) {
			p->set_last();
			// Tear down the connection after we've sent the last
			// segment of the base layer and are requested again.
			if ((s2.datasize() == 0) && (r->layer() == 0))
				p->set_finish();
		}
		return p;
	} else if (r->request() == MEDIAREQ_CHECKSEG)
		return &(QA::MEDIASEG_AVAIL_);
	else {
		fprintf(stderr, 
		       "MediaServer %s gets an unknown MediaRequest type %d\n",
			name(), r->request());
		abort();
	}
	/*NOTREACHED*/
	return NULL; // Make msvc happy
}

int MediaServer::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "is-media-page") == 0) {
			ClientPage *pg = pool_->get_page(argv[2]);
			if (pg && (pg->type() == MEDIA))
				tcl.result("1");
			else 
				tcl.result("0");
			return TCL_OK;
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "stop-prefetching") == 0) {
			/*
			 * <server> stop-prefetching <Client> <pagenum>
			 */
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int tmp[2];
			tmp[0] = (int)a;
			tmp[1] = atoi(argv[3]);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(pref_, (const char*)tmp);
			if (he == NULL) {
				tcl.add_errorf(
				  "Server %d cannot stop prefetching!\n", id_);
				return TCL_ERROR;
			}
			MediaSegmentList *p = 
				(MediaSegmentList *)Tcl_GetHashValue(he);
			assert(p != NULL);
			for (int i = 0; i < MAX_LAYER; i++)
				p[i].destroy();
			delete []p;
			Tcl_DeleteHashEntry(he);
			return (TCL_OK);
		}
	} else { 
		if (strcmp(argv[1], "enter-page") == 0) {
			ClientPage *pg = pool_->enter_page(argc, argv);
			if (pg == NULL)
				return TCL_ERROR;
			if (pg->type() == MEDIA) 
				((MediaPage*)pg)->create();
			// Unlock the page after creation
			((MediaPage*)pg)->unlock(); 
			return TCL_OK;
		} else if (strcmp(argv[1], "register-prefetch") == 0) {
			/*
			 * <server> register-prefetch <client> <pagenum> 
			 * 	<layer> {<segments>}
			 * Registers a list of segments to be prefetched by 
			 * <client>, where each <segment> is a pair of 
			 * (start, end). <pagenum> should be pageid without 
			 * preceding [server:] prefix
			 * 
			 * <client> is the requestor of the stream.
			 */
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int newEntry = 1;
			int tmp[2];
			tmp[0] = (int)a;
			tmp[1] = (int)atoi(argv[3]);
			Tcl_HashEntry *he = Tcl_CreateHashEntry(pref_, 
					(const char*)tmp, &newEntry);
			if (he == NULL) {
				fprintf(stderr, "Cannot create entry.\n");
				return TCL_ERROR;
			}
			MediaSegmentList *p;
			int layer = atoi(argv[4]);
			if (!newEntry)
				p = (MediaSegmentList*)Tcl_GetHashValue(he);
			else {
				p = new MediaSegmentList[10];
				Tcl_SetHashValue(he, (ClientData)p);
			}
			assert(p != NULL);

#if 0
			char *buf;
			if (p[layer].length() > 0) {
				buf = p[layer].dump2list();
				fprintf(stderr, 
			  "Server %d preempted segments %s of layer %d\n",
					id_, buf, layer);
				delete []buf;
			}
#endif
			// Preempt all old requests because they 
			// cannot reach the cache "in time"
			p[layer].destroy();

			// Add segments into prefetching list
			assert(argc % 2 == 1);
			for (int i = 5; i < argc; i+=2)
				p[layer].add(MediaSegment(atoi(argv[i]), 
							  atoi(argv[i+1])));
			return TCL_OK;
		} else if (strcmp(argv[1], "register-client") == 0) {
			// <cache> register-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			int newEntry;
			Tcl_HashEntry *he = Tcl_CreateHashEntry(cmap_, 
					(const char *)a, &newEntry);
			if (he == NULL) {
				tcl.add_errorf("cannot create hash entry");
				return TCL_ERROR;
			}
			if (!newEntry) {
				tcl.add_errorf("duplicate connection");
				return TCL_ERROR;
			}
			RegInfo *p = new RegInfo;
			p->client_ = (HttpApp*)TclObject::lookup(argv[3]);
			assert(p->client_ != NULL);
			strcpy(p->name_, argv[4]);
			Tcl_SetHashValue(he, (ClientData)p);
			return TCL_OK;
		} else if (strcmp(argv[1], "unregister-client") == 0) {
			// <cache> unregister-client <app> <client> <pageid>
			TclObject *a = TclObject::lookup(argv[2]);
			assert(a != NULL);
			Tcl_HashEntry *he = 
				Tcl_FindHashEntry(cmap_, (const char*)a);
			if (he == NULL) {
				tcl.add_errorf("cannot find hash entry");
				return TCL_ERROR;
			}
			RegInfo *p = (RegInfo*)Tcl_GetHashValue(he);
			delete p;
			Tcl_DeleteHashEntry(he);
			return TCL_OK;
		}
	}
			
	return HttpServer::command(argc, argv);
}
