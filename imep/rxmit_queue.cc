/* -*- c++ -*-
   rexmit_queue.cc
   $Id: rxmit_queue.cc,v 1.1 1999/08/03 04:12:39 yaxu Exp $
   */

#include <assert.h>

#include <packet.h>
#include <list.h>

#include <imep/rxmit_queue.h>

ReXmitQ::ReXmitQ()
{
  LIST_INIT(&head)
}
  
void 
ReXmitQ::insert(Time rxat, Packet *p, int num_rexmits)
{
  struct rexent *r = new rexent;
  r->rexmit_at = rxat;
  r->p = p;
  r->rexmits_left = num_rexmits;

  struct rexent *i;

  if (NULL == head.lh_first || rxat < head.lh_first->rexmit_at) 
    {
      LIST_INSERT_HEAD(&head, r, next);
      return;
    }

  for (i = head.lh_first ; i != NULL ; i = i->next.le_next )
    {
      if (rxat < i->rexmit_at) 
	{
	  LIST_INSERT_BEFORE(i, r, next);
	  return;
	}
      if (NULL == i->next.le_next)
	{
	  LIST_INSERT_AFTER(i, r, next);
	  return;
	}
    }
}

void
ReXmitQ::peekHead(Time *rxat, Packet **pp, int *rexmits_left)
{
  struct rexent *i;
  i = head.lh_first;
  if (NULL == i) {
    *rxat = -1; *pp = NULL; *rexmits_left = -1;
    return;
  }
  *rxat = i->rexmit_at;
  *pp = i->p; 
  *rexmits_left = i->rexmits_left;
}

void 
ReXmitQ::removeHead()
{
  struct rexent *i;
  i = head.lh_first;
  if (NULL == i) return;
  LIST_REMOVE(i, next);
  delete i;
}

void 
ReXmitQ::remove(Packet *p)
{
  struct rexent *i;
  for (i = head.lh_first ; i != NULL ; i = i->next.le_next )
    {
      if (p == i->p)
	{
	  LIST_REMOVE(i, next);
	  delete i;
	  return;
	}
    }
}
