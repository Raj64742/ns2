// 
// events.cc     : Provides the eventQueue
// authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: events.cc,v 1.1 2001/11/08 17:42:31 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#include <stdio.h>
#include <stdlib.h>

#include "events.hh"


// Safe malloc
void *check_malloc(size_t s)
{
  void *p = malloc(s);
  if (!p){
    printf("Error: malloc returned 0\n");
    abort();
  }
  return p;
}

//
//  Timeval utility routines
//

//  returns -1, 0, 1  for < == > 
int timeval_cmp(struct timeval *x, struct timeval *y)
{
  if (x->tv_sec > y->tv_sec) return 1;
  if (x->tv_sec < y->tv_sec) return -1;
  if (x->tv_usec > y->tv_usec) return 1;
  if (x->tv_usec < y->tv_usec) return -1;
  return 0;
}

// returns a pointer to a static structure.  returns {0,0} if x<y.
struct timeval *timeval_sub(struct timeval *x, struct timeval *y)
{
  static struct timeval tv;

  if (timeval_cmp(x,y) < 0) 
    tv.tv_sec = tv.tv_usec = 0;
  else{
    tv = *x;
    // borrow..
    if (tv.tv_usec < y->tv_usec){
      tv.tv_usec += 1000000;
      tv.tv_sec--;
    }
    // sub
    tv.tv_usec -= y->tv_usec;
    tv.tv_sec -= y->tv_sec;
  }
  return &tv;
}

//  adds usecs delay to tv
void timeval_addusecs(struct timeval *tv, int usecs)
{
  tv->tv_usec += usecs;
  if (tv->tv_usec > 1000000){
    tv->tv_sec += tv->tv_usec / 1000000;
    tv->tv_usec = tv->tv_usec % 1000000;
  }
}


//
//  Event utility routines
//


//  compares times of two events, returns -1, 0, 1 for < == >
int event_cmp(event *x, event *y)
{
  return timeval_cmp(&(x->tv),&(y->tv));
}

//
//  eventQueue Methods
//

void eventQueue::eq_new()
{
  head = NULL;
}

void eventQueue::eq_add(event *n)
{
  event *ptr = head;
  event *last = ptr;
  while (ptr && (event_cmp(n,ptr) > 0)){
    last = ptr; 
    ptr = ptr->next;
  }
  if (last == ptr){
    n->next = head;
    head = n;
  }
  else{
    n->next = ptr;
    last->next = n;
  }
}

event * eventQueue::eq_pop()
{
  event *e = head;
  if (e){
    head = head->next;
    e->next = NULL;
  }
  return e;
}

event * eventQueue::eq_findEvent(int type)
{
  return eq_findNextEvent(type, head);
}

event * eventQueue::eq_findNextEvent(int type, event *e)
{
  while (e){

    if (e->type == type)
      return e;

    e = e->next;
  }

  return e;
}

void eventQueue::eq_addAfter(int type, void *payload, int delay_msec)
{
  event *e = MALLOC(event *,sizeof(event));
  e->type = type;
  e->payload = payload;
  event_setDelay(e,delay_msec);
  eq_add(e);
}

void eventQueue::event_setDelay(event *e, int delay_msec)
{
  getTime(&(e->tv));
  timeval_addusecs(&(e->tv),delay_msec*1000);
}

// caution: returns pointer to statically allocated timeval
struct timeval * eventQueue::eq_nextTimer()
{
  static struct timeval tv;

  if (head){
    struct timeval now;
    
    getTime(&now);
    tv = *timeval_sub(&(head->tv),&now);
    return &tv;
  }
  else
    return NULL;
}


int eventQueue::eq_topInPast()
{
  if (head){
    struct timeval now;
    getTime(&now);
    return (timeval_cmp(&(head->tv),&now) <= 0);
  }
  return 0;
}

void eventQueue::eq_print()
{
  event *e = head;
  for (; (e); e = e->next){
    printf("  time=%ld:%06ld type=%d\n",e->tv.tv_sec,e->tv.tv_usec,e->type);
  }
}

int eventQueue::eq_remove(event *e)
{
  event *ce, *pe;

  if (e){
    if (head == e){
      head = e->next;
      return 0;
    }

    pe = head;
    ce = head->next;

    while (ce){
      if (ce == e){
	pe->next = e->next;
	return 0;
      }
      else{
	pe = ce;
	ce = ce->next;
      }
    }
  }

  return -1;
}

// computes a randomized delay, measured in milliseconds.
// note that most OS timers have granularities on order of 5-15 ms.
// timer[2] = { base_timer, variance }

int eventQueue::randDelay(int timer[2])
{
  return (int) (timer[0] + ((((float)rand())/((float)RAND_MAX)) - 0.5)*timer[1]);
}

