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
  void put_in_hash(hdr_cdiff *);
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
