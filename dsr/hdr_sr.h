// Copyright (c) 2000 by the University of Southern California
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
// Ported from CMU/Monarch's code, appropriate copyright applies.  

/* -*- c++ -*-
   hdr_sr.h

   source route header
   
*/
#ifndef sr_hdr_h
#define sr_hdr_h

#include <assert.h>

#include <packet.h>

#define SR_HDR_SZ 4		// size of constant part of hdr

#define MAX_SR_LEN 16		// longest source route we can handle
#define MAX_ROUTE_ERRORS 3	// how many route errors can fit in one pkt?

struct sr_addr {
        int addr_type;		/* same as hdr_cmn in packet.h */
	nsaddr_t addr;

	/*
	 * Metrics that I want to collect at each node
	 */
	double	Pt_;
};

struct link_down {
	int addr_type;		/* same as hdr_cmn in packet.h */
	nsaddr_t tell_addr;	// tell this host
	nsaddr_t from_addr;	// that from_addr host can no longer
	nsaddr_t to_addr;	// get packets to to_addr host
};


/* ======================================================================
   DSR Packet Types
   ====================================================================== */
struct route_request {
	int	req_valid_;	/* request header is valid? */
	int	req_id_;	/* unique request identifier */
	int	req_ttl_;	/* max propagation */
};

struct route_reply {
	int	rep_valid_;	/* reply header is valid? */
	int	rep_rtlen_;	/* # hops in route reply */
	struct sr_addr	rep_addrs_[MAX_SR_LEN];
};

struct route_error {
	int	err_valid_;	/* error header is valid? */
	int	err_count_;	/* number of route errors */
	struct link_down err_links_[MAX_ROUTE_ERRORS];
};

/* ======================================================================
   DSR Flow State Draft Stuff
   ====================================================================== */

struct flow_error {
	nsaddr_t  flow_src;
	nsaddr_t  flow_dst;
	u_int16_t flow_id;  /* not valid w/ default flow stuff */
};

struct flow_header {
	int flow_valid_;
	int hopCount_;
	unsigned short flow_id_;
};

struct flow_timeout {
	int flow_timeout_valid_;
	unsigned long timeout_;  // timeout in seconds...
};

struct flow_unknown {
	int flow_unknown_valid_;
	int err_count_;
	struct flow_error err_flows_[MAX_ROUTE_ERRORS];
};

// default flow unknown errors
struct flow_default_err {
	int flow_default_valid_;
	int err_count_;
	struct flow_error err_flows_[MAX_ROUTE_ERRORS];
};

/* ======================================================================
   DSR Header
   ====================================================================== */
class hdr_sr {
private:
	int valid_;		/* is this header actually in the packet? 
				   and initialized? */
	int salvaged_;	/* packet has been salvaged? */

	int num_addrs_;
	int cur_addr_;
	struct sr_addr addrs_[MAX_SR_LEN];

	struct route_request	sr_request_;
	struct route_reply	sr_reply_;
	struct route_error	sr_error_;

	struct flow_header      sr_flow_;
	struct flow_timeout	sr_ftime_;
	struct flow_unknown	sr_funk_;
	struct flow_default_err sr_fdef_unk;

public:
	static int offset_;		/* offset for this header */
	inline int& offset() { return offset_; }
        inline static hdr_sr* access(const Packet* p) {
	        return (hdr_sr*)p->access(offset_);
	}
	inline int& valid() { return valid_; }
	inline int& salvaged() { return salvaged_; }
	inline int& num_addrs() { return num_addrs_; }
	inline int& cur_addr() { return cur_addr_; }

	inline int valid() const { return valid_; }
	inline int salvaged() const { return salvaged_; }
	inline int num_addrs() const { return num_addrs_; }
	inline int cur_addr() const { return cur_addr_; }
	inline struct sr_addr* addrs() { return addrs_; }

	inline int& route_request() {return sr_request_.req_valid_; }
	inline int& rtreq_seq() {return sr_request_.req_id_; }
	inline int& max_propagation() {return sr_request_.req_ttl_; }

	inline int& route_reply() {return sr_reply_.rep_valid_; }
	inline int& route_reply_len() {return sr_reply_.rep_rtlen_; }
	inline struct sr_addr* reply_addrs() {return sr_reply_.rep_addrs_; }

	inline int& route_error() {return sr_error_.err_valid_; }
	inline int& num_route_errors() {return sr_error_.err_count_; }
	inline struct link_down* down_links() {return sr_error_.err_links_; }

	// Flow state stuff, ych 5/2/01
	inline int &flow_header() { return sr_flow_.flow_valid_; }
	inline u_int16_t &flow_id() { return sr_flow_.flow_id_; }
	inline int &hopCount() { return sr_flow_.hopCount_; }

	inline int &flow_timeout() { return sr_ftime_.flow_timeout_valid_; }
	inline unsigned long &flow_timeout_time() { return sr_ftime_.timeout_; }

	inline int &flow_unknown() { return sr_funk_.flow_unknown_valid_; }
	inline int &num_flow_unknown() { return sr_funk_.err_count_; }
	inline struct flow_error *unknown_flows() { return sr_funk_.err_flows_; }

	inline int &flow_default_unknown() { return sr_fdef_unk.flow_default_valid_; }
	inline int &num_default_unknown() { return sr_fdef_unk.err_count_; }
	inline struct flow_error *unknown_defaults() { return sr_fdef_unk.err_flows_; }

	inline int size() {
		int sz = 0;
		if (num_addrs_ || route_request() || 
		    route_reply() || route_error() ||
		    flow_timeout() || flow_unknown() || flow_default_unknown())
			sz += SR_HDR_SZ;

		if (num_addrs_)			sz += 4 * (num_addrs_ - 1);
		if (route_reply())		sz += 5 + 4 * route_reply_len();
		if (route_request())		sz += 8;
		if (route_error())		sz += 16 * num_route_errors();
		if (flow_timeout())		sz += 4;
		if (flow_unknown())		sz += 14 * num_flow_unknown();
		if (flow_default_unknown())	sz += 12 * num_default_unknown();

		if (flow_header())		sz += 4;

		sz = ((sz+3)&(~3)); // align...
		assert(sz >= 0);
#if 0
		printf("Size: %d (%d %d %d %d %d %d %d %d %d)\n", sz,
			(num_addrs_ || route_request() ||
			route_reply() || route_error() ||
			flow_timeout() || flow_unknown() || 
			flow_default_unknown()) ? SR_HDR_SZ : 0,
			num_addrs_ ? 4 * (num_addrs_ - 1) : 0,
			route_reply() ? 5 + 4 * route_reply_len() : 0,
			route_request() ? 8 : 0,
			route_error() ? 16 * num_route_errors() : 0,
			flow_timeout() ? 4 : 0,
			flow_unknown() ? 14 * num_flow_unknown() : 0,
			flow_default_unknown() ? 12 * num_default_unknown() : 0,
			flow_header() ? 4 : 0);
#endif

		return sz;
	}

	// End Flow State stuff

	inline nsaddr_t& get_next_addr() { 
		assert(cur_addr_ < num_addrs_);
		return (addrs_[cur_addr_ + 1].addr);
	}

	inline int& get_next_type() {
		assert(cur_addr_ < num_addrs_);
		return (addrs_[cur_addr_ + 1].addr_type);
	}

	inline void append_addr(nsaddr_t a, int type) {
		assert(num_addrs_ < MAX_SR_LEN-1);
		addrs_[num_addrs_].addr_type = type;
		addrs_[num_addrs_++].addr = a;
	}

	inline void init() {
		valid_ = 1;
		salvaged_ = 0;
		num_addrs_ = 0;
		cur_addr_ = 0;

		route_request() = 0;
		route_reply() = 0;
		route_reply_len() = 0;
		route_error() = 0;
		num_route_errors() = 0;

		flow_timeout() = 0;
		flow_unknown() = 0;
		flow_default_unknown() = 0;
		flow_header() = 0;
	}

#if 0
#ifdef DSR_CONST_HDR_SZ
  /* used to estimate the potential benefit of removing the 
     src route in every packet */
	inline int size() { 
		return SR_HDR_SZ;
	}
#else
	inline int size() { 
		int sz = SR_HDR_SZ +
			4 * (num_addrs_ - 1) +
			4 * (route_reply() ? route_reply_len() : 0) +
			8 * (route_error() ? num_route_errors() : 0);
		assert(sz >= 0);
		return sz;
	}
#endif // DSR_CONST_HDR_SZ
#endif // 0

  void dump(char *);
  char* dump();
};


#endif // sr_hdr_h
