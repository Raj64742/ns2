/* -*- c++ -*- 
   path.h

   handles source routes
   $Id: path.h,v 1.1 1998/12/08 19:17:24 haldar Exp $
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
enum ID_Type {NONE = AF_NONE, MAC = AF_LINK, IP = AF_INET };

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
    assert(n < len && n >= 0);
    return path[n];}
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
