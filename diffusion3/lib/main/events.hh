//
// events.hh     : Implements a queue of timer-based events
// Authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: events.hh,v 1.4 2002/05/29 21:58:13 haldar Exp $
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

// This file defines lower level events (which contain a type, a
// payload and an expiration time) and an event queue, kept in sorted
// order by expiration time. This is used by both the Diffusion
// Routing Library API and the Diffusion Core module for keeping track
// of events. Application (or Filter) developers should use the Timer
// API instead.

#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "tools.hh"

class DiffusionEvent {
public:
  struct timeval tv_;
  int type_;
  void *payload_;

  DiffusionEvent *next_;
};

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
    head_ = NULL;
  };
  virtual ~EventQueue(){
    // Empty
  };

  void eqAdd(DiffusionEvent *n);
  virtual void eqAddAfter(int type, void *payload, int delay_msec);
  DiffusionEvent * eqPop();
  DiffusionEvent * eqFindEvent(int type);
  DiffusionEvent * eqFindNextEvent(int type, DiffusionEvent *e);
  struct timeval * eqNextTimer();
  int eqTopInPast();
  void eqPrint();
  int eqRemove(DiffusionEvent *e);

//
//  DiffusionEvent methods
//
//  setDelay   sets the expiration time to a delay after present time
//  randDelay  computes a delay given a vector {base delay, variance}
//             delay times are chosen uniformly within the variance.
//  eventCmp   compares the expiration times from two events and returns
//             -1, 0 1 for <, == and >
//

private:

  void setDelay(DiffusionEvent *e, int delay_msec);
  int randDelay(int timer[2]);
  int eventCmp(DiffusionEvent *x, DiffusionEvent *y);

  DiffusionEvent *head_;

};

// Extra utility functions for managing struct timeval

// Compares two timeval structures, returning -1, 0 or 1 for <, == or >
int TimevalCmp(struct timeval *x, struct timeval *y);

// Returns x - y (or 0,0 if x < y)
struct timeval *TimevalSub(struct timeval *x, struct timeval *y);

// Add usecs to tv
void TimevalAddusecs(struct timeval *tv, int usecs);

#ifdef NS_DIFFUSION
#ifdef RAND_MAX
#undef RAND_MAX
#endif // RAND_MAX
#define RAND_MAX 2147483647
#else
#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif // !RAND_MAX
#endif // NS_DIFFUSION

#endif // !_EVENT_H_
