/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Ported from CMU/Monarch's code, nov'98 -Padma.*/

/* -*- c++ -*- 
   path.h

   handles source routes
   $Id: path.h,v 1.5 1999/10/13 22:53:04 heideman Exp $
*/
#ifndef _path_h
#define _path_h

extern "C" {
#include <stdio.h>
#include <assert.h>
	   }

#include <packet.h>
#include <dsr/hdr_sr.h>

class Path;			// forward declaration

// state used for tracing the performance of the caches
enum Link_Type {LT_NONE = 0, LT_TESTED = 1, LT_UNTESTED = 2};
enum Log_Status {LS_NONE = 0, LS_UNLOGGED = 1, LS_LOGGED = 2};

// some type conversion between exisiting NS code and old DSR sim
typedef double Time;
enum ID_Type {NONE = NS_AF_NONE, MAC = NS_AF_ILINK, IP = NS_AF_INET };

struct ID {
  friend class Path; 
  ID() : type(NONE), t(-1), link_type(LT_NONE), log_stat(LS_NONE) {}
  //  ID():addr(0),type(NONE) {}	// remove for speed? -dam 1/23/98
  ID(unsigned long name, ID_Type t):addr(name), type(t), t(-1), 
    link_type(LT_NONE),log_stat(LS_NONE)
	{
		assert(type == NONE || type == MAC || type == IP);
	}
  inline ID(const struct sr_addr &a): addr(a.addr), 
    type((enum ID_Type) a.addr_type), t(-1), link_type(LT_NONE),
	  log_stat(LS_NONE)
	{
		assert(type == NONE || type == MAC || type == IP);
	}
  inline void fillSRAddr(struct sr_addr& a) {
    a.addr_type = (int) type;
    a.addr = addr;
  }    
  inline nsaddr_t getNSAddr_t() const {
    assert(type == IP); return addr;
  }
  inline bool operator == (const ID& id2) const {
    return (type == id2.type) && (addr == id2.addr); }
  inline bool operator != (const ID& id2) const {return !operator==(id2);}
  inline int size() const {return (type == IP ? 4 : 6);} 
  void unparse(FILE* out) const;
  char* dump() const;

  unsigned long addr;
  ID_Type type;

  Time t;			// when was this ID added to the route
  Link_Type link_type;
  Log_Status log_stat;
};

extern ID invalid_addr;
extern ID IP_broadcast;

class Path {
friend void compressPath(Path& path);
friend void CopyIntoPath(Path& to, const Path& from, int start, int stop);
public:
  Path();
  Path(int route_len, const ID *route = NULL);
  Path(const Path& old);
  Path(const struct sr_addr *addrs, int len);
  Path(const struct hdr_sr *srh);

	~Path();
	
	void fillSR(struct hdr_sr *srh);
	
	inline ID& next() {assert(cur_index < len); return path[cur_index++];}
	inline void resetIterator() {  cur_index = 0;}
	inline void reset() {len = 0; cur_index = 0;}
	
	inline void setIterator(int i) {assert(i>=0 && i<len); cur_index = i;}
	inline void setLength(int l) {assert(l>=0 && l<=MAX_SR_LEN); len = l;}
	inline ID& operator[] (int n) const {  
		//debug 
		//if (n >= len || n < 0) {
		//printf("..........n-%d,len-%d\n",n,len);
		//}
		//assert(n < len && n >= 0);
		return path[n];
	}
	void operator=(const Path& rhs);
	bool operator==(const Path& rhs);
	inline void appendToPath(const ID& id) { 
		assert(len < MAX_SR_LEN); 
		path[len++] = id;}
	void appendPath(Path& p);
  bool member(const ID& id) const;
  bool member(const ID& net_id, const ID& MAC_id) const;
  Path copy() const;
  void copyInto(Path& to) const;
  Path reverse() const;
  void reverseInPlace();
  void removeSection(int from, int to);
  // the elements at indices from -> to-1 are removed from the path

  inline bool full() const {return (len >= MAX_SR_LEN);}
  inline int length() const {return len;}
  inline int index() const {return cur_index;}
  int size() const; // # of bytes needed to hold path in packet
  void unparse(FILE *out) const;
  char *dump() const;
  inline ID &owner() {return path_owner;}

  void checkpath(void) const;
private:
  int len;
  int cur_index;
  ID* path;
  ID path_owner;
};

void compressPath(Path& path);
// take a path and remove any double backs from it
// eg:  A B C B D --> A B D

void CopyIntoPath(Path& to, const Path& from, int start, int stop);
// sets to[0->(stop-start)] = from[start->stop]

#endif // _path_h
