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
// Definition of the HTTP agent
// 
// $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/webcache/http.h,v 1.1 1998/08/18 23:42:41 haoboy Exp $

#ifndef ns_http_h
#define ns_http_h

#include <stdlib.h>
#include <tcl.h>
#include "config.h"
#include "agent.h"
#include "app.h"

#include "pagepool.h"
#include "inval-agent.h"
#include "tcpapp.h"
#include "http-aux.h"

class HttpApp : public TclObject {
public:
	HttpApp();
	virtual ~HttpApp();

	virtual int command(int argc, const char*const* argv);
#ifdef JOHNH_CLASSINSTVAR
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName);
#endif /* JOHNH_CLASSINSTVAR */

	void log(const char *fmt, ...);
	int id() const { return id_; }

protected:
	int add_cnc(HttpApp *client, TcpApp *agt);
	void delete_cnc(HttpApp *client);
	TcpApp* lookup_cnc(HttpApp *client);

	void set_pagepool(ClientPagePool* pp) { pool_ = pp; }

	Tcl_HashTable *tpa_;	// TcpApp hash table
	int id_;		// Node id
	ClientPagePool *pool_;	// Page repository
	Tcl_Channel log_;	// Log file descriptor
};


//----------------------------------------------------------------------
// Servers
//----------------------------------------------------------------------
class HttpServer : public HttpApp {
public: 
	// All methods are in TCL
};

class HttpInvalServer : public HttpServer {
public:
	// All methods are in TCL
};

// Http server with periodic unicast heartbeat invalidations
class HttpYucInvalServer : public HttpInvalServer {
public:
	HttpYucInvalServer();

#ifdef JOHNH_CLASSINSTVAR
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName);
#endif /* JOHNH_CLASSINSTVAR */
	virtual int command(int argc, const char*const* argv);

protected:
	int Ca_, Cb_, push_thresh_, enable_upd_;
	double hb_interval_;		// Heartbeat interval (second)
};


//----------------------------------------------------------------------
// Caches
//----------------------------------------------------------------------

class HttpCache : public HttpApp {
};

class HttpInvalCache : public HttpCache {
};

// Invalidations embedded in periodic heartbeats
// Used by recv_inv() and recv_inv_filter() to filter invalidations.
const int HTTP_INVALCACHE_FILTERED 	= 0;
const int HTTP_INVALCACHE_UNFILTERED 	= 1;

// Http cache with periodic multicast heartbeat invalidation
class HttpMInvalCache : public HttpInvalCache {
public:
	HttpMInvalCache();
	virtual ~HttpMInvalCache();

#ifdef JOHNH_CLASSINSTVAR
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName);
#endif /* JOHNH_CLASSINSTVAR */

	virtual int command(int argc, const char*const* argv);
	virtual void recv_pkt(int size, char* data);
	virtual void timeout(int reason);

	void handle_node_failure(int cid);
	void invalidate_server(int sid);
	void add_inv(const char *name, double mtime);

protected:
	HBTimer hb_timer_;		// Heartbeat/Inval timer
	HttpInvalAgent **inv_sender_;	// Heartbeat/Inval sender agents
	int num_sender_;		// # of heartbeat sender agents
	int size_sender_;		// Maximum size of array inv_sender_
	InvalidationRec *invlist_;	// All invalidations to be sent
	int num_inv_;			// # of invalidations in invlist_

	void send_heartbeat();
	HttpHbData* pack_heartbeat();
	virtual void send_hb_helper(int size, int datasize, const char *data);

	int recv_inv(HttpHbData *d);
	virtual void process_inv(int n, InvalidationRec *ivlist, int cache);
	virtual int recv_inv_filter(ClientPage* pg, InvalidationRec *p) {
		return ((pg == NULL) || (pg->mtime() >= p->mtime()) ||
			!pg->is_valid()) ? 
			HTTP_INVALCACHE_FILTERED : HTTP_INVALCACHE_UNFILTERED;
	}
	InvalidationRec* get_invrec(const char *name);

	// Maintaining SState(Server, NextCache)
	// Use the shadow names of server and cache as id
	struct SState {
		SState(NeighborCache* c) : down_(0), cache_(c) {}
		int is_down() { return down_; }
		void down() { down_ = 1; }
		void up() { down_ = 0; }
		NeighborCache* cache() { return cache_; }
		int down_;		// If the server is disconnected
		NeighborCache *cache_;	// NextCache
	};

	Tcl_HashTable sstate_;
	void add_sstate(int sid, SState* sst);
	SState* lookup_sstate(int sid);
	// check & establish sstate
	void check_sstate(int sid, int cid); 

	// Maintaining liveness of neighbor caches
	Tcl_HashTable nbr_;
	void add_nbr(HttpMInvalCache* c);
	NeighborCache* lookup_nbr(int id);

	HttpUInvalAgent *inv_parent_;	// Heartbeat/Inval to parent cache
	
	void recv_heartbeat(int id);
	void recv_leave(HttpLeaveData *d);
	void send_leave(HttpLeaveData *d);

	double hb_interval_;		// Heartbeat interval (second)
	int enable_upd_;		// Whether enable push
	int Ca_, Cb_, push_thresh_;

	HttpInvalAgent **upd_sender_;	// Agents to push updates to
	int num_updater_;		// # number of update agents
	int size_updater_;		// Size of array upd_sender_

	void add_update(const char *name, double mtime);
	void send_upd(ClientPage *pg);
	int recv_upd(HttpUpdateData *d);
	virtual void send_upd_helper(int pgsize, int size, const char* data);
	HttpUpdateData* pack_upd(ClientPage *pg);

	// Use a static mapping to convert cache id to cache pointers
	static HttpMInvalCache** CacheRepository_;
	static int NumCache_;
	static void add_cache(HttpMInvalCache* c);
	static HttpMInvalCache* map_cache(int id) {
		return CacheRepository_[id];
	}
};

//----------------------------------------------------------------------
// Multicast invalidation + two way liveness messages + 
// invalidation filtering. 
//----------------------------------------------------------------------
class HttpPercInvalCache : virtual public HttpMInvalCache {
public:
	HttpPercInvalCache();

#ifdef JOHNH_CLASSINSTVAR
	virtual void delay_bind_init_all();
	virtual int delay_bind_dispatch(const char *varName, const char *localName);
#endif /* JOHNH_CLASSINSTVAR */
	int command(int argc, const char*const* argv);

protected: 
	virtual int recv_inv_filter(ClientPage *pg, InvalidationRec *ir) {
		// If we already have an invalid page, don't forward the
		// invalidation any more.
		return ((pg == NULL) || (pg->mtime() >= ir->mtime()) ||
			!pg->is_header_valid()) ? 
			HTTP_INVALCACHE_FILTERED : HTTP_INVALCACHE_UNFILTERED;
	}

	// Flag: if we allow direct request, and hence pro formas
	int direct_request_;
};

#endif // ns_http_h
