// 
// events.cc     : Provides the EventQueue
// authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: events.cc,v 1.3 2002/05/13 22:33:45 haldar Exp $
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

//
//  Timeval utility routines
//

int TimevalCmp(struct timeval *x, struct timeval *y)
{
  if (x->tv_sec > y->tv_sec) return 1;
  if (x->tv_sec < y->tv_sec) return -1;
  if (x->tv_usec > y->tv_usec) return 1;
  if (x->tv_usec < y->tv_usec) return -1;
  return 0;
}

struct timeval *TimevalSub(struct timeval *x, struct timeval *y)
{
  struct timeval *tv;

  tv = new struct timeval;

  if (TimevalCmp(x, y) < 0) 
    tv->tv_sec = tv->tv_usec = 0;
  else{
    *tv = *x;
    // borrow..
    if (tv->tv_usec < y->tv_usec){
      tv->tv_usec += 1000000;
      tv->tv_sec--;
    }
    // sub
    tv->tv_usec -= y->tv_usec;
    tv->tv_sec -= y->tv_sec;
  }
  return tv;
}

void TimevalAddusecs(struct timeval *tv, int usecs)
{
  tv->tv_usec += usecs;
  if (tv->tv_usec > 1000000){
    tv->tv_sec += tv->tv_usec / 1000000;
    tv->tv_usec = tv->tv_usec % 1000000;
  }
}

//
//  EventQueue Methods
//

int EventQueue::eventCmp(DiffusionEvent *x, DiffusionEvent *y)
{
  return TimevalCmp(&(x->tv_), &(y->tv_));
}

void EventQueue::eqAdd(DiffusionEvent *n)
{
  DiffusionEvent *ptr = head_;
  DiffusionEvent *last = ptr;

  while (ptr && (eventCmp(n, ptr) > 0)){
    last = ptr; 
    ptr = ptr->next_;
  }
  if (last == ptr){
    n->next_ = head_;
    head_ = n;
  }
  else{
    n->next_ = ptr;
    last->next_ = n;
  }
}

DiffusionEvent * EventQueue::eqPop()
{
  DiffusionEvent *e = head_;

  if (e){
    head_ = head_->next_;
    e->next_ = NULL;
  }
  return e;
}

DiffusionEvent * EventQueue::eqFindEvent(int type)
{
  return eqFindNextEvent(type, head_);
}

DiffusionEvent * EventQueue::eqFindNextEvent(int type, DiffusionEvent *e)
{
  while (e){
    if (e->type_ == type)
      return e;
    e = e->next_;
  }

  return e;
}

void EventQueue::eqAddAfter(int type, void *payload, int delay_msec)
{
  DiffusionEvent *e = new DiffusionEvent;

  e->type_ = type;
  e->payload_ = payload;
  setDelay(e, delay_msec);
  eqAdd(e);
}

void EventQueue::setDelay(DiffusionEvent *e, int delay_msec)
{
  GetTime(&(e->tv_));
  TimevalAddusecs(&(e->tv_), delay_msec * 1000);
}

struct timeval * EventQueue::eqNextTimer()
{
  struct timeval *tv;
  struct timeval now;

  if (head_){
    GetTime(&now);
    tv = TimevalSub(&(head_->tv_), &now);
    return tv;
  }
  else
    return NULL;
}

int EventQueue::eqTopInPast()
{
  struct timeval now;

  if (head_){
    GetTime(&now);
    return (TimevalCmp(&(head_->tv_), &now) <= 0);
  }
  return 0;
}

void EventQueue::eqPrint()
{
  DiffusionEvent *e = head_;

  for (; (e); e = e->next_){
    fprintf(stderr, "time = %d:%06d, type = %d\n",
	    e->tv_.tv_sec, e->tv_.tv_usec, e->type_);
  }
}

int EventQueue::eqRemove(DiffusionEvent *e)
{
  DiffusionEvent *ce, *pe;

  if (e){
    if (head_ == e){
      head_ = e->next_;
      return 0;
    }

    pe = head_;
    ce = head_->next_;

    while (ce){
      if (ce == e){
	pe->next_ = e->next_;
	return 0;
      }
      else{
	pe = ce;
	ce = ce->next_;
      }
    }
  }

  return -1;
}

// Computes a randomized delay, measured in milliseconds. Note that
// most OS timers have granularities on order of 5-15 ms.
// timer[2] = { base_timer, variance }
int EventQueue::randDelay(int timer[2])
{
  return (int) (timer[0] + ((((float) GetRand()) /
			     ((float) RAND_MAX)) - 0.5) * timer[1]);
}
