/* -*- c++ -*-
   hdr_sr.h

   source route header
   $Id: hdr_sr.h,v 1.1 1998/12/08 19:17:22 haldar Exp $
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
};

struct link_down {
  int addr_type;		/* same as hdr_cmn in packet.h */
  nsaddr_t tell_addr;		// tell this host
  nsaddr_t from_addr;		// that from_addr host can no longer
  nsaddr_t to_addr;		// get packets to to_addr host
};

struct hdr_sr {
  int valid_;			/* is this header actually in the packet? 
				   and initialized? */
  int num_addrs_;
  int cur_addr_;
  struct sr_addr addrs[MAX_SR_LEN];
  
  int route_request_;		/* is this a route request? */
  int rtreq_seq_;		// route request sequence number
  int max_propagation_;		// how many times can RTreq be forwarded?

  int route_reply_;		// is the reply below valid?
  int route_reply_len_;
  struct sr_addr reply_addrs_[MAX_SR_LEN];

  int route_error_;		// are we carrying a route reply?
  int num_route_errors_;
  struct link_down down_links_[MAX_ROUTE_ERRORS];

  static int offset_;		/* offset for this header */
  inline int& offset() { return offset_; }

  inline int& valid() {return valid_;}
  inline int& num_addrs() {return num_addrs_;}
  inline int& cur_addr() {return cur_addr_;}

  inline int& route_request() {return route_request_;}
  inline int& rtreq_seq() {return rtreq_seq_;}
  inline int& max_propagation() {return max_propagation_;}

  inline int& route_reply() {return route_reply_;}
  inline int& route_reply_len() {return route_reply_len_;}
  inline struct sr_addr* reply_addrs() {return reply_addrs_;}

  inline int& route_error() {return route_error_;}
  inline int& num_route_errors() {return num_route_errors_;}
  inline struct link_down* down_links() {return down_links_;}

  inline nsaddr_t& get_next_addr() { 
    assert(cur_addr_ < num_addrs_);
    return (addrs[cur_addr_ + 1].addr); }

  inline int& get_next_type() {
    assert(cur_addr_ < num_addrs_);
    return (addrs[cur_addr_ + 1].addr_type); }

  inline void append_addr(nsaddr_t a, int type) {
    assert(num_addrs_ < MAX_SR_LEN-1);
    addrs[num_addrs_].addr_type = type;
    addrs[num_addrs_++].addr = a; }

  inline void init() {
    valid_ = 1;
    num_addrs_ = 0;
    cur_addr_ = 0;
    route_request_ = 0;
    route_reply_ = 0;
    route_reply_len_ = 0;
    route_error_ = 0;
    num_route_errors_ = 0;}

  inline int size() { 
	int sz = SR_HDR_SZ +
		 4 * (num_addrs_ - 1) +
		 4 * (route_reply_ ? route_reply_len_ : 0) +
		 8 * (route_error_ ? num_route_errors_ : 0);
	assert(sz >= 0);
	return sz;
  }
  void dump(char *);
  char* dump();
};


#endif sr_hdr_h
