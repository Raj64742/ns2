//
// timers.hh       : Timer Management Include File
// authors         : John Heidemann, Fabio Silva and Alefiya Hussain
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: timers.hh,v 1.3 2003/07/09 17:50:02 haldar Exp $
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

#ifndef _TIMERS_H_
#define _TIMERS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef USE_THREADS
#include <pthread.h>
#endif // USE_THREADS

#include <string.h>

#include "events.hh"

#ifdef NS_DIFFUSION
class TimerManager;
class DiffEvent;
class DiffEventHandler;
#include "difftimer.h"
#endif // NS_DIFFUSION

//
// To make a new timer, subclass TimerCallback and override the
// expire() method.
//
// If you need to pass parameters to your timer, pass them in the
// constructor of your subclass.
//
// If you allocate data in your callback, free it in the destructor.
//
// When the timer fires, expire() will be called.  You can do anything
// you want there.  When you're done, return a value that indicates
// what happens to the timer:
//
// return = 0   re-add timer again to queue with same timeout as last time
//        > 0   re-add timer to queue with new timeout given by return value
//        < 0   discard timer (do not re-add it to the queue)
//

class TimerCallback {
public:
  TimerCallback() {};
  virtual ~TimerCallback() {};
  virtual int expire() = 0;
};

// This class is used to define a timer in the event queue. The
// timeout provided should be in milliseconds. cb specifies the
// function that will be called.

class TimerEntry {
public:
  handle hdl_;
  int timeout_;
  TimerCallback *cb_;

  TimerEntry(handle hdl, int timeout,TimerCallback *cb) : 
    hdl_(hdl), timeout_(timeout), cb_(cb) {};
  ~TimerEntry() {};
};

// Creates the Timer Management class. Creates the eventqueue
// The eventqueue is used by the Timer class only. 
// Use the NextTimerTime and ExecuteNextTimer functions to access
// the event queue

class TimerManager {
public:
  TimerManager();
  ~TimerManager() {};

  // Timer API functions:
	
  // add a timer to the queue, returning the handle that can be used
  // to cancel it with removeTimer timeout value deifne in ms.  When
  // the timer fires, expire() will be called.  You can do anything
  // you want there.  When you're done, return a value that indicates
  // what happens to the timer:
  //
  // return = 0   re-add timer again to queue with same timeout as last time
  //        > 0   re-add timer to queue with new timeout given by return value
  //        < 0   discard timer (do not re-add it to the queue)

  handle addTimer(int timeout, TimerCallback *cb);

  // remove a timer that's scheduled, returns if it was there.
  bool removeTimer(handle hdl);

  // returns the timer value at head of the queue
  void nextTimerTime(struct timeval *tv);

  // Executes the timer on the head of the queue
#ifdef NS_DIFFUSION
  void diffTimeout(DiffEvent *e);
#else
  void executeNextTimer();
  void executeAllExpiredTimers();
#endif // NS_DIFFUSION

protected:
  int next_handle_; // counter of handle ids
  EventQueue *eq_;  // internal list of timers
#ifdef USE_THREADS
  pthread_mutex_t *queue_mtx_;
#endif // USE_THREADS
};

#endif // _TIMERS_H_

