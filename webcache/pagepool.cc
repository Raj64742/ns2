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
// Load a trace statistics file, and randomly generate requests and 
// page lifetimes from the trace.
//
// Trace statistics file format:
// <URL> <size> {<modification time>}
//
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/pagepool.cc,v 1.1 1998/08/18 23:42:42 haoboy Exp $

#include <stdio.h>
#include <limits.h>
#include "pagepool.h"
#include "http.h"

Page::Page(const char *name, int size)
{
	size_ = size;
	name_ = new char[strlen(name) + 1];
	strcpy(name_, name);
}

void ServerPage::set_mtime(int *mt, int n)
{
	if (mtime_ != NULL) 
		delete []mtime_;
	mtime_ = new int[n];
	memcpy(mtime_, mt, sizeof(int)*n);
}

ClientPage::ClientPage(const char *n, int s, double mt, double et, double a) :
		Page(n, s), age_(a), mtime_(mt), etime_(et), 
		status_(HTTP_VALID_PAGE), counter_(0), 
		mandatoryPush_(0), mpushTime_(0)
{
	// Parse name to get server and page id
	char *buf = new char[strlen(n) + 1];
	strcpy(buf, n);
	char *tmp = strtok(buf, ":");
	server_ = (HttpApp*)TclObject::lookup(tmp);
	if (server_ == NULL) {
		fprintf(stderr, "Non-exitent server name for page %s", n);
		abort();
	}
	tmp = strtok(NULL, ":");
	id_ = atol(tmp);
	delete []buf;
}


static class PagePoolClass : public TclClass {
public:
        PagePoolClass() : TclClass("PagePool") {}
        TclObject* create(int, const char*const*) {
		return (new PagePool());
	}
} class_pagepool_agent;

static class TracePagePoolClass : public TclClass {
public:
        TracePagePoolClass() : TclClass("PagePool/Trace") {}
        TclObject* create(int argc, const char*const* argv) {
		if (argc >= 5)
			return (new TracePagePool(argv[4]));
		return 0;
	}
} class_tracepagepool_agent;

TracePagePool::TracePagePool(const char *fn) : 
	ranvar_(0), PagePool()
{
	FILE *fp = fopen(fn, "r");
	if (fp == NULL) {
		fprintf(stderr, 
			"TracePagePool: couldn't open trace file %s\n", fn);
		abort();	// What else can we do?
	}

	namemap_ = new Tcl_HashTable;
	Tcl_InitHashTable(namemap_, TCL_STRING_KEYS);
	idmap_ = new Tcl_HashTable;
	Tcl_InitHashTable(idmap_, TCL_ONE_WORD_KEYS);

	ServerPage *pg;
	while (pg = load_page(fp))
		if (add_page(pg)) {
			fprintf(stderr, 
				"TracePagePool: couldn't add page %s\n",
			       pg->name());
			abort();
		}
	change_time();
}

TracePagePool::~TracePagePool()
{
	if (namemap_ != NULL) {
		Tcl_DeleteHashTable(namemap_);
		delete namemap_;
	}
	if (idmap_ != NULL) {
		Tcl_DeleteHashTable(idmap_);
		delete idmap_;
	}
}

void TracePagePool::change_time()
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	ServerPage *pg;
	int i, j;

	for (i = 0, he = Tcl_FirstHashEntry(idmap_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) {
		pg = (ServerPage *) Tcl_GetHashValue(he);
		for (j = 0; j < pg->num_mtime(); j++) 
			pg->mtime(j) -= (int)start_time_;
	}
	end_time_ -= start_time_;
	start_time_ = 0;
	duration_ = (int)end_time_;
}

ServerPage* TracePagePool::load_page(FILE *fp)
{
	static char buf[TRACEPAGEPOOL_MAXBUF];
	char *delim = " \t\n";
	char *tmp1, *tmp2;
	ServerPage *pg;

	// XXX Use internal variables of struct Page
	if (!fgets(buf, TRACEPAGEPOOL_MAXBUF, fp))
		return NULL;

	// URL
	tmp1 = strtok(buf, delim);
	// Size
	tmp2 = strtok(NULL, delim);
	pg = new ServerPage(tmp1, atoi(tmp2), num_pages_++);

	// Modtimes, assuming they are in ascending time order
	int num = 0;
	int *nmd = new int[5];
	while ((tmp1 = strtok(NULL, delim)) != NULL) {
		if (num >= 5) {
			int *tt = new int[num+5];
			memcpy(tt, nmd, sizeof(int)*num);
			delete []nmd;
			nmd = tt;
		}
		nmd[num] = atoi(tmp1);
		if (nmd[num] < start_time_)
			start_time_ = nmd[num];
		if (nmd[num] > end_time_)
			end_time_ = nmd[num];
		num++;
	}
	pg->num_mtime() = num;
	pg->set_mtime(nmd, num);
	delete []nmd;
	return pg;
}

int TracePagePool::add_page(ServerPage *pg)
{
	int newEntry = 1;
	Tcl_HashEntry *he = Tcl_CreateHashEntry(namemap_, 
						(const char *)pg->name(),
						&newEntry);
	if (he == NULL)
		return -1;
	if (newEntry)
		Tcl_SetHashValue(he, (ClientData)pg);
	else 
		fprintf(stderr, "TracePagePool: Duplicate entry %s\n", 
			pg->name());

	Tcl_HashEntry *hf = 
		Tcl_CreateHashEntry(idmap_, (const char *)pg->id(), &newEntry);
	if (hf == NULL) {
		Tcl_DeleteHashEntry(he);
		return -1;
	}
	if (newEntry)
		Tcl_SetHashValue(hf, (ClientData)pg);
	else 
		fprintf(stderr, "TracePagePool: Duplicate entry %d\n", 
			pg->id());

	return 0;
}

ServerPage* TracePagePool::get_page(int id)
{
	if ((id < 0) || (id >= num_pages_))
		return NULL;
	Tcl_HashEntry *he = Tcl_FindHashEntry(idmap_, (const char *)id);
	if (he == NULL)
		return NULL;
	return (ServerPage *)Tcl_GetHashValue(he);
}

int TracePagePool::command(int argc, const char *const* argv)
{
	Tcl &tcl = Tcl::instance();

	if (argc == 2) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			/* 
			 * <pgpool> gen-pageid
			 * Randomly generate a page id from page pool
			 */
			double tmp = ranvar_ ? ranvar_->value() : 
				Random::uniform();
			// tmp should be in [0, num_pages_-1]
			tmp = (tmp < 0) ? 0 : (tmp >= num_pages_) ? 
				(num_pages_-1):tmp;
			if ((int)tmp >= num_pages_) abort();
			tcl.resultf("%d", (int)tmp);
			return TCL_OK;
		} else 	if (strcmp(argv[1], "get-poolsize") == 0) {
			/* 
			 * <pgpool> get-poolsize
			 * Get the number of pages currently in pool
			 */
			tcl.resultf("%d", num_pages_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%d", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-size") == 0) {
			/*
			 * <pgpool> gen-size <pageid>
			 */
			int id = atoi(argv[2]);
			ServerPage *pg = get_page(id);
			if (pg == NULL) {
				tcl.add_errorf("TracePagePool %s: page %d doesn't exists.\n",
					       name_, id);
				return TCL_ERROR;
			}
			tcl.resultf("%d", pg->size());
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar") == 0) {
			/* 
			 * <pgpool> ranvar <ranvar> 
			 * Set a random var which is used to randomly pick 
			 * a page from the page pool.
			 */
			ranvar_ = (RandomVariable *)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			/* 
			 * <pgpool> get-modtime <pageid> <modtime>
			 * 
			 * Return next modtime that is larger than modtime
			 * To retrieve the first modtime (creation time), set 
			 * <modtime> to -1 in the request.
			 */
			int id = atoi(argv[2]);
			double mt = strtod(argv[3], NULL);
			ServerPage *pg = get_page(id);
			if (pg == NULL) {
				tcl.add_errorf("TracePagePool %s: page %d doesn't exists.\n",
					       name_, id);
				return TCL_ERROR;
			}
			for (int i = 0; i < pg->num_mtime(); i++) 
				if (pg->mtime(i) > mt) {
					tcl.resultf("%d", 
						    pg->mtime(i)+start_time_);
					return TCL_OK;
				}
			// When get to the last modtime, return -1
			tcl.resultf("%d", INT_MAX);
			return TCL_OK;
		}
	}
	return TclObject::command(argc, argv);
}



static class MathPagePoolClass : public TclClass {
public:
        MathPagePoolClass() : TclClass("PagePool/Math") {}
        TclObject* create(int, const char*const*) {
		return (new MathPagePool());
	}
} class_mathpagepool_agent;

// Use 3 ranvars to generate requests, mod times and page size

int MathPagePool::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();

	// Keep the same tcl interface as PagePool/Trace
	if (argc == 2) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			// Single page
			tcl.result("0");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%d", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-size") == 0) {
			if (rvSize_ == 0) {
				tcl.add_errorf("%s: no page size generator", 
					       name_);
				return TCL_ERROR;
			}
			int size = (int) rvSize_->value();
			tcl.resultf("%d", size);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-size") == 0) {
			rvSize_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-age") == 0) {
			rvAge_ = (RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			if (rvAge_ == 0) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			double mt = strtod(argv[3], NULL);
			tcl.resultf("%.17g", mt + rvAge_->value());
			return TCL_OK;
		}
	}

	return PagePool::command(argc, argv);
}


// Assume one main page, which changes often, and multiple component pages
static class CompMathPagePoolClass : public TclClass {
public:
        CompMathPagePoolClass() : TclClass("PagePool/CompMath") {}
        TclObject* create(int, const char*const*) {
		return (new CompMathPagePool());
	}
} class_compmathpagepool_agent;


CompMathPagePool::CompMathPagePool()
{
#ifdef JOHNH_CLASSINSTVAR
#else /* ! JOHNH_CLASSINSTVAR */
	bind("num_pages_", &num_pages_);
	bind("main_size_", &main_size_);
	bind("comp_size_", &comp_size_);
#endif
}

#ifdef JOHNH_CLASSINSTVAR
void CompMathPagePool::delay_bind_init_all()
{
	delay_bind_init_one("num_pages_");
	delay_bind_init_one("main_size_");
	delay_bind_init_one("comp_size_");
	TclObject::delay_bind_init_all();
}

int CompMathPagePool::delay_bind_dispatch(const char *varName, 
					  const char *localName)
{
	DELAY_BIND_DISPATCH(varName, localName, "num_pages_", 
			    delay_bind, &num_pages_);
	DELAY_BIND_DISPATCH(varName, localName, "main_size_", 
			    delay_bind, &main_size_);
	DELAY_BIND_DISPATCH(varName, localName, "comp_size_", 
			    delay_bind, &comp_size_);
	return TclObject::delay_bind_dispatch(varName, localName);
}
#endif /* JOHNH_CLASSINSTVAR */

int CompMathPagePool::command(int argc, const char *const* argv)
{
	Tcl& tcl = Tcl::instance();

	// Keep the same tcl interface as PagePool/Trace
	if (argc == 2) {
		if (strcmp(argv[1], "gen-pageid") == 0) {
			// Main pageid, never return id of component pages
			tcl.result("0");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-poolsize") == 0) { 
			tcl.result("1");
			return TCL_OK;
		} else if (strcmp(argv[1], "get-start-time") == 0) {
			tcl.resultf("%d", start_time_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-duration") == 0) {
			tcl.resultf("%d", duration_);
			return TCL_OK;
		} else if (strcmp(argv[1], "get-comp-pages") == 0) {
			tcl.resultf("%d", num_pages_-1);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "gen-size") == 0) {
			tcl.resultf("%d", main_size_);
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-comp-size") == 0) {
			tcl.resultf("%d", comp_size_);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-age") == 0) {
			rvMainAge_ = 
				(RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "ranvar-comp-age") == 0) {
			rvCompAge_ =
				(RandomVariable*)TclObject::lookup(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "set-start-time") == 0) {
			double st = strtod(argv[2], NULL);
			start_time_ = st;
			end_time_ += st;
		}
	} else {
		if (strcmp(argv[1], "gen-modtime") == 0) {
			if (rvMainAge_ == 0) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			double mt = strtod(argv[3], NULL);
			tcl.resultf("%.17g", mt + rvMainAge_->value());
			return TCL_OK;
		} else if (strcmp(argv[1], "gen-comp-modtime") == 0) {
			if (rvCompAge_ == 0) {
				tcl.add_errorf("%s: no page age generator", 
					       name_);
				return TCL_ERROR;
			}
			double mt = atoi(argv[3]);
			tcl.resultf("%.17g", mt + rvCompAge_->value());
			return TCL_OK;
		}
	}

	return PagePool::command(argc, argv);
}


static class ClientPagePoolClass : public TclClass {
public:
        ClientPagePoolClass() : TclClass("PagePool/Client") {}
        TclObject* create(int argc, const char*const*argv) {
		return (new ClientPagePool());
	}
} class_clientpagepool_agent;

ClientPagePool::ClientPagePool()
{
	namemap_ = new Tcl_HashTable;
	Tcl_InitHashTable(namemap_, TCL_STRING_KEYS);
}

ClientPagePool::~ClientPagePool()
{
	if (namemap_ != NULL) {
		Tcl_DeleteHashTable(namemap_);
		delete namemap_;
	}
}

ClientPage* ClientPagePool::get_page(const char *name)
{
	Tcl_HashEntry *he = Tcl_FindHashEntry(namemap_, name);
	if (he == NULL)
		return NULL;
	return (ClientPage *)Tcl_GetHashValue(he);
}

ClientPage* 
ClientPagePool::add_page(const char *name, int size, double mt, 
			 double et, double age)
{
	ClientPage *pg = new ClientPage(name, size, mt, et, age);

	int newEntry = 1;
	Tcl_HashEntry *he = Tcl_CreateHashEntry(namemap_, 
						(const char *)pg->name(),
						&newEntry);
	if (he == NULL)
		return NULL;
	if (newEntry)
		Tcl_SetHashValue(he, (ClientData)pg);
 	else {
		// Replace the old one
		ClientPage *q = (ClientPage *)Tcl_GetHashValue(he);
		// XXX must copy the counter value
		pg->counter() = q->counter();
		// XXX must copy the mpush values
		pg->mandatoryPush_ = q->mandatoryPush_;
		pg->mpushTime_ = q->mpushTime_;
		Tcl_SetHashValue(he, (ClientData)pg);
		delete q;
 		//fprintf(stderr, "ClientPagePool: Replaced entry %s\n", 
 		//	pg->name());
	}
	return pg;
}

int ClientPagePool::set_mtime(const char *name, double mt)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	pg->mtime() = mt;
	return 0;
}

int ClientPagePool::get_mtime(const char *name, double& mt)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	mt = pg->mtime();
	return 0;
}

int ClientPagePool::set_etime(const char *name, double et)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	pg->etime() = et;
	return 0;
}

int ClientPagePool::get_etime(const char *name, double& et)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	et = pg->etime();
	return 0;
}

int ClientPagePool::get_size(const char *name, int& size) 
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	size = pg->size();
	return 0;
}

int ClientPagePool::get_age(const char *name, double& age) 
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	age = pg->age();
	return 0;
}

int ClientPagePool::get_page(const char *name, char *buf)
{
	ClientPage *pg = (ClientPage *)get_page(name);
	if (pg == NULL) 
		return -1;
	sprintf(buf, "size %d modtime %.17g time %.17g age %.17g",
		pg->size(), pg->mtime(), pg->etime(), pg->age());
	return 0;
}

void ClientPagePool::invalidate_server(int sid)
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	ClientPage *pg;
	int i, j;

	for (i = 0, he = Tcl_FirstHashEntry(namemap_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) {
		pg = (ClientPage *) Tcl_GetHashValue(he);
		if (pg->server()->id() == sid)
			pg->server_down();
	}
}
