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

/* requesttable.h

   implement a table to keep track of the most current request
   number we've heard from a node in terms of that node's id

   $Id: requesttable.cc,v 1.2 1999/01/04 20:08:41 haldar Exp $
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
