// -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
//
// Time-stamp: <2000-09-15 12:53:32 haoboy>
//
// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// Diffusion-event handler object, Padma, nov 2001.

#ifdef NS_DIFFUSION

#ifndef diffevent_handler_h
#define diffevent_handler_h

#include "scheduler.h"

/* This handler class keeps track of diffusion specific events. 
   It schedules multiple events and irrespective of the event types, 
   always calls back the callback function of its agent.
*/

class DiffAppAgent;

class DiffEventHandler : public Handler {
 public:
  DiffEventHandler(DiffAppAgent *agent) { a_ = agent; }
  void handle(Event *);
  //bool findEvent(Event *); FUTURE WORK
  //void removeEvent(Event *e); FUTURE WORK
 private:
  DiffAppAgent *a_;
};

class DiffRoutingAgent;

class CoreDiffEventHandler : public Handler {
 public:
  CoreDiffEventHandler(DiffRoutingAgent *agent) { a_ = agent; }
  void handle(Event *);
  //bool findEvent(Event *);
  //void removeEvent(Event *e);
 private:
  DiffRoutingAgent *a_;
};

#endif //diffusion_timer_h
#endif // NS
