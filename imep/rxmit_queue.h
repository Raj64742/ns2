/* -*- c++ -*-
   rexmit_queue.h
   $Id: rxmit_queue.h,v 1.1 1999/08/03 04:12:40 yaxu Exp $
   */

#ifndef imep_rexmit_queue_h
#define imep_rexmit_queue_h

#include <packet.h>
#include <list.h>

typedef double Time;

struct rexent {
  double rexmit_at;
  int rexmits_left;
  Packet *p;
  LIST_ENTRY(struct rexent) next;
};

LIST_HEAD(rexent_head, rexent);

class ReXmitQ;

class ReXmitQIter {
  friend ReXmitQ;
  
public:
  inline Packet * next() {
    if (0 == iter) return 0;
    struct rexent *t = iter;
    iter = iter->next.le_next;
    return t->p;
  }
  
private:
  ReXmitQIter(rexent *r) : iter(r) {};
  struct rexent * iter;
};

class ReXmitQ {
public:
  ReXmitQ();
  
  void insert(Time rxat, Packet *p, int num_rexmits);
  void peekHead(Time *rxat, Packet **pp, int *rexmits_left);
  void removeHead();
  void remove(Packet *p);
  inline ReXmitQIter iter() {
    return ReXmitQIter(head.lh_first); 
  }

private:
  rexent_head head;
};

#endif
