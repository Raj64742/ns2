#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rtable.h"

static int rtent_trich(const void *a, const void *b) {
  nsaddr_t ia = ((const rtable_ent *) a)->dst;
  nsaddr_t ib = ((const rtable_ent *) b)->dst;
  if (ia > ib) return 1;
  if (ib > ia) return -1;
  return 0;
}

RoutingTable::RoutingTable() {
  // Let's start with a ten element maxelts.
  elts = 0;
  maxelts = 10;
  rtab = new rtable_ent[maxelts];
}

void
RoutingTable::AddEntry(const rtable_ent &ent)
{
  rtable_ent *it;

  assert(ent.metric <= BIG);

  if ((it = (rtable_ent*) bsearch(&ent, rtab, elts, sizeof(rtable_ent), 
				 rtent_trich))) {
    bcopy(&ent,it,sizeof(rtable_ent));
    return;
    /*
    if (it->seqnum < ent.seqnum || it->metric > (ent.metric+em)) {
      bcopy(&ent,it,sizeof(rtable_ent));
      it->metric += em;
      return NEW_ROUTE_SUCCESS_OLDENT;
    } else {
      return NEW_ROUTE_METRIC_TOO_HIGH;
    }
    */
  }

  if (elts == maxelts) {
    rtable_ent *tmp = rtab;
    maxelts *= 2;
    rtab = new rtable_ent[maxelts];
    bcopy(tmp, rtab, elts*sizeof(rtable_ent));
    delete tmp;
  }
  
  int max;
  
  for (max=0;max<elts;max++)
      if (ent.dst < rtab[max].dst)
	break;
  
  // Copy all the further out guys out yet another.
  bcopy(&rtab[max], &rtab[max+1], 
	(elts-max)*sizeof(rtable_ent));

  bcopy(&ent, &rtab[max], sizeof(rtable_ent));
  elts++;

  return;
}

void 
RoutingTable::InitLoop() {
  ctr = 0;
}

rtable_ent *
RoutingTable::NextLoop() {
  if (ctr >= elts)
    return 0;

  return &rtab[ctr++];
}

// Only valid (duh) as long as no new routes are added
int 
RoutingTable::RemainingLoop() {
  return elts-ctr;
}

rtable_ent *
RoutingTable::GetEntry(nsaddr_t dest) {
  rtable_ent ent;
  ent.dst = dest;
  return (rtable_ent *) bsearch(&ent, rtab, elts, sizeof(rtable_ent), 
				rtent_trich);
}
