/* -*- c++ -*-
   requesttable.h

   implement a table to keep track of the most current request
   number we've heard from a node in terms of that node's id

   implemented as a circular buffer

   $Id: requesttable.h,v 1.1 1998/12/08 19:17:25 haldar Exp $
*/

#ifndef _requesttable_h
#define _requesttable_h

#include "path.h"

struct Entry;

enum LastType { LIMIT0, UNLIMIT};

class RequestTable {
public:
  RequestTable(int size = 30);
  ~RequestTable();
  void insert(const ID& net_id, int req_num);
  void insert(const ID& net_id, const ID& MAC_id, int req_num);
  int get(const ID& id) const;
  // rtns 0 if id not found
  Entry* getEntry(const ID& id);  
private:
  Entry *table;
  int size;
  int ptr;
  int find(const ID& net_id, const ID& MAC_id ) const;
};

struct Entry {
  ID MAC_id;
  ID net_id;
  int req_num;
  Time last_arp;
  int rt_reqs_outstanding;
  Time last_rt_req;
  LastType last_type;
};

#endif //_requesttable_h
