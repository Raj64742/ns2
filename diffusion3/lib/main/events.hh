//
// events.hh     : Implements a queue of timer-based events
// Authors       : Lewis Girod, John Heidemann and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: events.hh,v 1.7 2003/07/09 17:50:02 haldar Exp $
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

typedef long handle;
#define MAXVALUE 0x7ffffff // No events in the queue

class QueueEvent {
public:
  struct timeval tv_;
  handle handle_;
  void *payload_;

  QueueEvent *next_;
};

class EventQueue {

//
//  Methods
//
//  eqAdd         inserts an event into the queue
//  eqAddAfter    creates a new event and inserts it
//  eqPop         extracts the first event and returns it
//  eqNextTimer   returns the time of expiration for the next event
//  eqTopInPast   returns true if the head of the timer queue is in the past
//  eqRemoveEvent removes an event from the queue
//  eqRemove      removes the event with the given handle from the queue
//

public:
  EventQueue(){
    // Empty
    head_ = NULL;
  };
  virtual ~EventQueue(){
    // Empty
  };

  virtual void eqAddAfter(handle hdl, void *payload, int delay_msec);
  QueueEvent * eqPop();
  QueueEvent * eqFindEvent(handle hdl);
  void eqNextTimer(struct timeval *tv);
  virtual bool eqRemove(handle hdl);

private:

  int eqTopInPast();
  void eqPrint();
  bool eqRemoveEvent(QueueEvent *e);
  QueueEvent * eqFindNextEvent(handle hdl, QueueEvent *e);
  void eqAdd(QueueEvent *e);

//
//  QueueEvent methods
//
//  setDelay   sets the expiration time to a delay after present time
//  randDelay  computes a delay given a vector {base delay, variance}
//             delay times are chosen uniformly within the variance.
//  eventCmp   compares the expiration times from two events and returns
//             -1, 0 1 for <, == and >
//

  void setDelay(QueueEvent *e, int delay_msec);
  int randDelay(int timer[2]);
  int eventCmp(QueueEvent *x, QueueEvent *y);

  QueueEvent *head_;

};

// Extra utility functions for managing struct timeval

// Compares two timeval structures, returning -1, 0 or 1 for <, == or >
int TimevalCmp(struct timeval *x, struct timeval *y);

// tv will be x - y (or 0,0 if x < y)
void TimevalSub(struct timeval *x, struct timeval *y, struct timeval *tv);

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
