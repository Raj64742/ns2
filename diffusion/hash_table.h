#ifndef ns_hash_table_h
#define ns_hash_table_h

#include "config.h"
#include "tclcl.h"
#include "diff_header.h"

//#include "diffusion.h"

#include "iflist.h"

class InterestTimer;

class Pkt_Hash_Entry {
 public:
  ns_addr_t    forwarder_id;
  bool        is_forwarded;
  bool        has_list;
  int         num_from;
  From_List  *from_agent;
  InterestTimer *timer;

  Pkt_Hash_Entry() { 
    forwarder_id.addr_ = 0;
    forwarder_id.port_ = 0;
    is_forwarded = false;
    has_list = false; 
    num_from=0; 
    from_agent=NULL; 
    timer=NULL;
  }

  ~Pkt_Hash_Entry();
  void clear_fromagent(From_List *);
};


class Pkt_Hash_Table {
 public:
  Tcl_HashTable htable;

  Pkt_Hash_Table() {
    Tcl_InitHashTable(&htable, 3);
  }

  void reset();
  void put_in_hash(hdr_diff *);
  Pkt_Hash_Entry *GetHash(ns_addr_t sender_id, unsigned int pkt_num);
};


class Data_Hash_Table {
 public:
  Tcl_HashTable htable;

  Data_Hash_Table() {
    Tcl_InitHashTable(&htable, MAX_ATTRIBUTE);
  }

  void reset();
  void PutInHash(int *attr);
  Tcl_HashEntry *GetHash(int *attr);
};

#endif
