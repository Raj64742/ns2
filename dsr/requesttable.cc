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

/* requesttable.h

   implement a table to keep track of the most current request
   number we've heard from a node in terms of that node's id

*/

#include "path.h"
#include "constants.h"
#include "requesttable.h"

RequestTable::RequestTable(int s): size(s)
{
  table = new Entry[size];
  size = size;
  ptr = 0;
}

RequestTable::~RequestTable()
{
  delete[] table;
}

int
RequestTable::find(const ID& net_id, const ID& MAC_id) const
{
  for (int c = 0 ; c < size ; c++)
	  if (table[c].net_id == net_id || table[c].MAC_id == MAC_id)
		  return c;
  return size;
}

int
RequestTable::get(const ID& id) const
{
  int existing_entry = find(id, id);

  if (existing_entry >= size)    
    {
      return 0;
    }
  return table[existing_entry].req_num;
}


Entry*
RequestTable::getEntry(const ID& id)
{
  int existing_entry = find(id, id);

  if (existing_entry >= size)    
    {
      table[ptr].MAC_id = ::invalid_addr;
      table[ptr].net_id = id;
      table[ptr].req_num = 0;
      table[ptr].last_arp = 0.0;
      table[ptr].rt_reqs_outstanding = 0;
      table[ptr].last_rt_req = -(rt_rq_period + 1.0);
      existing_entry = ptr;
      ptr = (ptr+1)%size;
    }
  return &(table[existing_entry]);
}

void
RequestTable::insert(const ID& net_id, int req_num)
{
  insert(net_id,::invalid_addr,req_num);
}


void
RequestTable::insert(const ID& net_id, const ID& MAC_id, int req_num)
{
  int existing_entry = find(net_id, MAC_id);

  if (existing_entry < size)
    {
      if (table[existing_entry].MAC_id == ::invalid_addr)
	table[existing_entry].MAC_id = MAC_id; // handle creations by getEntry
      table[existing_entry].req_num = req_num;
      return;
    }

  // otherwise add it in
  table[ptr].MAC_id = MAC_id;
  table[ptr].net_id = net_id;
  table[ptr].req_num = req_num;
  table[ptr].last_arp = 0.0;
  table[ptr].rt_reqs_outstanding = 0;
  table[ptr].last_rt_req = -(rt_rq_period + 1.0);
  ptr = (ptr+1)%size;
}
