//
// events.hh     : Implements a queue of timer-based events
// Authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: events.hh,v 1.8 2002/03/21 22:07:40 haldar Exp $
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

#ifndef _EVENT_H_
#define _EVENT_H_

#include "tools.hh"

//
//  An event specifies an expiration time, a type, and a payload.
//  An eventQueue is a list of events, kept in sorted order by 
//  expiration time.  
//

typedef struct _event {
  struct timeval tv;
  int type;
  void *payload;
  struct _event *next;
} event;

class EventQueue {

//
//  Methods
//
//  eqAdd        inserts an event into the queue
//  eqAddAfter   creates a new event and inserts it
//  eqPop        extracts the first event and returns it
//  eqNextTimer  returns the time of expiration for the next event
//  eqTopInPast  returns true if the head of the timer queue is in the past
//  eqRemove     remove an event from the queue
//

public:
  EventQueue(){
    // Empty
    head = NULL;
  };
  virtual ~EventQueue(){
    // Empty
  };

  void eqAdd(event *n);
  virtual void eqAddAfter(int type, void *payload, int delay_msec);
  event * eqPop();
  event * eqFindEvent(int type);
  event * eqFindNextEvent(int type, event *e);
  struct timeval * eqNextTimer();
  int eqTopInPast();
  void eqPrint();
  int eqRemove(event *e);

//
//  Event methods
//
//  setDelay   sets the expiration time to a delay after present time
//  randDelay  computes a delay given a vector {base delay, variance}
//             delay times are chosen uniformly within the variance.
//

private:

  void setDelay(event *e, int delay_msec);
  int randDelay(int timer[2]);

  event *head;

};

// timeval routines
int timeval_cmp(struct timeval *x, struct timeval *y);
struct timeval *timeval_sub(struct timeval *x, struct timeval *y);
void timeval_addusecs(struct timeval *tv, int usecs);

// event routines
int event_cmp(event *x, event *y);

#ifdef NS_DIFFUSION
#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MAX 2147483647
#else
#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif // !RAND_MAX
#endif // NS_DIFFUSION

// MALLOC hack (thanks jeremy)
#ifndef MALLOC
void *check_malloc(size_t s);
#define MALLOC(type,size)   (type)check_malloc(size)
#endif

#endif // !_EVENT_H_
