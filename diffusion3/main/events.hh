//
// events.hh     : Implements a queue of timer-based events
// Authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: events.hh,v 1.2 2001/11/20 22:28:17 haldar Exp $
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

#ifdef NS_DIFFUSION

#ifndef __EVENT_H
#define __EVENT_H

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

class eventQueue {

//
//  Methods
//
//  eq_new        initializes the event queue
//  eq_add        inserts an event into the queue
//  eq_addAfter   creates a new event and inserts it
//  eq_pop        extracts the first event and returns it
//  eq_nextTimer  returns the time of expiration for the next event
//  eq_topInPast  returns true if the head of the timer queue is in the past
//  eq_remove     remove an event from the queue
//

public:
  eventQueue(){
    // Empty
  };
  ~eventQueue(){
    // Empty
  };

  virtual void eq_new();
  virtual void eq_add(event *n);
  virtual void eq_addAfter(int type, void *payload, int delay_msec);
  virtual event * eq_pop();
  virtual event * eq_findEvent(int type);
  virtual event * eq_findNextEvent(int type, event *e);
  virtual struct timeval * eq_nextTimer();
  virtual int eq_topInPast();
  void eq_print();
  virtual int eq_remove(event *e);

//
//  Event methods
//
//  setDelay   sets the expiration time to a delay after present time
//  randDelay  computes a delay given a vector {base delay, variance}
//             delay times are chosen uniformly within the variance.
//

private:

  void event_setDelay(event *e, int delay_msec);
  int randDelay(int timer[2]);

  event *head;

};


// timeval routines
int timeval_cmp(struct timeval *x, struct timeval *y);
struct timeval *timeval_sub(struct timeval *x, struct timeval *y);
void timeval_addusecs(struct timeval *tv, int usecs);

// event routines
int event_cmp(event *x, event *y);

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif

// MALLOC hack (thanks jeremy)
#ifndef MALLOC
void *check_malloc(size_t s);
#define MALLOC(type,size)   (type)check_malloc(size)
#endif

#endif

#endif // NS
