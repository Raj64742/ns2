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


void Pkt_Hash_Table::put_in_hash(hdr_diff *dfh)
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





