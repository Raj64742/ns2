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

/**********************************************************/
/* iflist.h : Chalermek Intanagonwiwat (USC/ISI) 06/25/99 */
/**********************************************************/


#ifndef ns_iflist_h
#define ns_iflist_h

#include <stdio.h>
#include "config.h"

#define INTF_INSERT(x,y)  x->InsertFront((Agent_List **)&x, (Agent_List *)y)
#define INTF_REMOVE(x,y)  y->Remove(x,y)
#define INTF_FIND(x,y)    x->Find((Agent_List **)&x, y)
#define INTF_FREEALL(x)   x->FreeAll((Agent_List **)&x)
#define INTF_UNION(x,y)   x->Union((Agent_List **)&x, (Agent_List *)y)

#define AGENT_NEXT(x)     x->next
#define FROM_NEXT(x)      (From_List *)(x->next)
#define OUT_NEXT(x)       (Out_List *)(x->next)
#define IN_NEXT(x)        (In_List *)(x->next)

#define AGT_ADDR(x)       x->agent_addr
#define NODE_ADDR(x)      x->agent_addr.addr_
#define PORT(x)           x->agent_addr.port_
#define RANK(x)           ((From_List *)x)->rank
#define IS_SINK(x)        ((From_List *)x)->is_sink

#define GRADIENT(x)       ((Out_List *)x)->gradient
#define GRAD_TMOUT(x)     ((Out_List *)x)->timeout
#define FROM_SLOT(x)      ((Out_List *)x)->from
#define TO_SLOT(x)        ((Out_List *)x)->to
#define NUM_DATA_SEND(x)       ((Out_List *)x)->num_data_send
#define NUM_NEG_RECV(x)       ((Out_List *)x)->num_neg_recv
#define NUM_POS_RECV(x)       ((Out_List *)x)->num_pos_recv

#define NUM_POS_SEND(x)   ((In_List *)x)->num_pos_send
#define NUM_NEG_SEND(x)   ((In_List *)x)->num_neg_send
#define LAST_TS_NEW_SUB(x) ((In_List *)x)->last_ts_new_sub
#define NEW_SUB_RECV(x)   ((In_List *)x)->new_sub_recv
#define NEW_ORG_RECV(x)   ((In_List *)x)->new_org_recv
#define OLD_ORG_RECV(x)   ((In_List *)x)->old_org_recv
#define TOTAL_NEW_SUB_RECV(x)   ((In_List *)x)->total_new_sub_recv
#define TOTAL_NEW_ORG_RECV(x)   ((In_List *)x)->total_new_org_recv
#define TOTAL_OLD_ORG_RECV(x)   ((In_List *)x)->total_old_org_recv

#define TOTAL_RECV(x)     ((In_List *)x)->total_received
#define PREV_RECV(x)      ((In_List *)x)->prev_received
#define NUM_LOSS(x)       ((In_List *)x)->num_loss
#define AVG_DELAY(x)      ((In_List *)x)->avg_delay
#define VAR_DELAY(x)      ((In_List *)x)->var_delay

#define WHERE_TO_GO(x)    x->WhereToGo()
#define FIND_MAX_IN(x)    x->FindMaxIn()
#define CAL_RANGE(x)      x->CalRange()
#define NORMALIZE(x)      x->NormalizeGradient()

class Agent_List;


class PrvCurPtr {
public:
  Agent_List **prv;
  Agent_List *cur;
};


class Agent_List {
public:
  ns_addr_t agent_addr;
  Agent_List *next;

  Agent_List() { 
    next = NULL; 
    agent_addr.addr_=0;
    agent_addr.port_=0;
  }
  
  void InsertFront(Agent_List **, Agent_List *);
  void Remove(Agent_List **, Agent_List *);
  PrvCurPtr Find(Agent_List **, ns_addr_t);
  void FreeAll(Agent_List **);
  void Union(Agent_List **, Agent_List *);

  virtual void print();
};


class From_List : public Agent_List {
public:
  int rank;
  bool  is_sink;

  From_List() : Agent_List() { rank = 0; is_sink = false; }
};


class Out_List : public From_List {
public:
  float gradient;
  double timeout;
  double   from;
  double   to;
  int   num_data_send;
  int   num_neg_recv;
  int   num_pos_recv;

  Out_List() : From_List() { gradient = 0; from=0.0; to=0.0; num_data_send=0; 
                             timeout = 0.0; num_neg_recv = 0; num_pos_recv=0;}

  Out_List *WhereToGo();
  void CalRange();
  void NormalizeGradient();
};


class In_List : public Agent_List {
public:
  double avg_delay;
  double var_delay;
  int    total_received;
  int    prev_received;
  int    num_loss;
  int    num_neg_send;
  int    num_pos_send;

  int    total_new_org_recv;
  int    total_old_org_recv;
  int    total_new_sub_recv;
  int    new_org_recv;        // for simple mode. New original sample counter.
  int    old_org_recv;        // for simple mode. Old original sample counter.
  int    new_sub_recv;        // for simple mode. New subsample counter.
  double last_ts_new_sub;     // Last timestamp in receiving a new subsample.

  In_List() : Agent_List() { 
    avg_delay =0; 
    var_delay =0; 
    total_received=0;
    prev_received =0; 
    num_loss =0; 
    num_neg_send = 0;
    num_pos_send = 0;

    total_new_org_recv=0;
    total_old_org_recv=0;
    total_new_sub_recv=0; 

    new_org_recv=0;
    old_org_recv=0;
    new_sub_recv=0; 

    last_ts_new_sub = -1.0;
  }

  In_List *FindMaxIn();
};

#endif



/* Example 

void main () {

  From_List *start, *cur;

  start=NULL;

  cur = new From_List;
  cur->agent_addr.addr_ =1;
  cur->agent_addr.port_ =1;
  INTF_INSERT(start,cur);

  cur = new From_List;
  cur->agent_addr.addr_ = 3;
  cur->agent_addr.port_ = 3;
  INTF_INSERT(start,cur);

  cur = new From_List;
  cur->agent_addr.addr_ = 5;
  cur->agent_addr.port_ = 5;
  INTF_INSERT(start,cur);

  start->print();

  PrvCurPtr RetVal;
  ns_addr_t fnd_addr;

  fnd_addr.addr_ = 1;
  fnd_addr.port_ = 1;
  RetVal= INTF_FIND(start, fnd_addr);
  INTF_REMOVE(RetVal.prv, RetVal.cur);

  start->print();

}

*/


