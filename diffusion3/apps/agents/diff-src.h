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
// diff-src.h 

#ifdef NS_DIFFUSION

#ifndef NS_DIFF_SRC
#define NS_DIFF_SRC

#include "diffapp.h"
#include "agent.hh"


#define SEND_DATA_INTERVAL 5
class DiffusionSource;

class sendDataTimer : public TimerHandler {
  public:
  sendDataTimer(DiffusionSource *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  DiffusionSource *a_;
};


//class DiffusionSource : public Application {
class DiffusionSource : public DiffApp {
public:
  DiffusionSource();
  handle setupInterest(NR *dr);
  handle setupPublication(NR *dr);
  int command(int argc, const char*const* argv);

  void createAttrTemplates();

  void start();
  void send();
  void run() { }
  void recv(Message *msg, handle h) { } 
 
 private:
  
	// NR *dr;

  MySrcReceive *mr;
  
  handle sendHandle;
  handle subscribeHandle;
  
  int counter;
  NRSimpleAttribute<int> *counterAttr;
  NRAttrVec data_attr;
  
  sendDataTimer sdt;

  
};

#endif 

#endif// NS
