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


// Written by Chalermek Intanagonwiwat

#include "tclcl.h"
#include "diff_header.h"
#include "hash_table.h"
#include "diff_prob.h"


Pkt_Hash_Entry::~Pkt_Hash_Entry() {
    clear_fromagent(from_agent);
    if (timer != NULL)
      delete timer;
}


void Pkt_Hash_Entry::clear_fromagent(From_List *list)
{
  From_List *cur=list;
  From_List *temp = NULL;

  while (cur != NULL) {
    temp = FROM_NEXT(cur);
    delete cur;
    cur = temp;
  }
}

void Pkt_Hash_Table::reset()
{
  Pkt_Hash_Entry *hashPtr;
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    hashPtr = (Pkt_Hash_Entry *)Tcl_GetHashValue(entryPtr);
    delete hashPtr;
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}

Pkt_Hash_Entry *Pkt_Hash_Table::GetHash(ns_addr_t sender_id, 
					unsigned int pk_num)
{
  unsigned int key[3];

  key[0] = sender_id.addr_;
  key[1] = sender_id.port_;
  key[2] = pk_num;

  Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&htable, (char *)key);

  if (entryPtr == NULL )
     return NULL;

  return (Pkt_Hash_Entry *)Tcl_GetHashValue(entryPtr);
}


void Pkt_Hash_Table::put_in_hash(hdr_cdiff *dfh)
{
    Tcl_HashEntry *entryPtr;
    Pkt_Hash_Entry    *hashPtr;
    unsigned int key[3];
    int newPtr;

    key[0]=(dfh->sender_id).addr_;
    key[1]=(dfh->sender_id).port_;
    key[2]=dfh->pk_num;

    entryPtr = Tcl_CreateHashEntry(&htable, (char *)key, &newPtr);
    if (!newPtr)
      return;

    hashPtr = new Pkt_Hash_Entry;
    hashPtr->forwarder_id = dfh->forward_agent_id;
    hashPtr->from_agent = NULL;
    hashPtr->is_forwarded = false;
    hashPtr->num_from = 0;
    hashPtr->timer = NULL;

    Tcl_SetHashValue(entryPtr, hashPtr);
}


void Data_Hash_Table::reset()
{
  Tcl_HashEntry *entryPtr;
  Tcl_HashSearch searchPtr;

  entryPtr = Tcl_FirstHashEntry(&htable, &searchPtr);
  while (entryPtr != NULL) {
    Tcl_DeleteHashEntry(entryPtr);
    entryPtr = Tcl_NextHashEntry(&searchPtr);
  }
}


Tcl_HashEntry *Data_Hash_Table::GetHash(int *attr)
{
  return Tcl_FindHashEntry(&htable, (char *)attr);
}


void Data_Hash_Table::PutInHash(int *attr)
{
    int newPtr;

    Tcl_CreateHashEntry(&htable, (char *)attr, &newPtr);
}





