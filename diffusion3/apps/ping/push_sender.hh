//
// push_sender.hh : Push Ping Sender Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: push_sender.hh,v 1.1 2003/07/09 17:45:02 haldar Exp $
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

#ifndef _PUSH_SENDER_HH_
#define _PUSH_SENDER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ping.hh"

class PushSenderApp;

#ifdef NS_DIFFUSION
#define SEND_DATA_INTERVAL 5

class PushSendDataTimer : public TimerHandler {
  public:
  PushSendDataTimer(PushSenderApp *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  PushSenderApp *a_;
};
#endif //NS_DIFFUSION

class PushSenderApp : public DiffApp {
public:
#ifdef NS_DIFFUSION
  PushSenderApp();
  int command(int argc, const char*const* argv);
  void send();
#else
  PushSenderApp(int argc, char **argv);
#endif // NS_DIFFUSION

  virtual ~PushSenderApp()
  {
    // Nothing to do
  };

  void run();

private:
  // NR Specific variables
  handle pubHandle_;

  // Ping App variables
  int last_seq_sent_;
  NRAttrVec data_attr_;
  NRSimpleAttribute<void *> *timeAttr_;
  NRSimpleAttribute<int> *counterAttr_;
  EventTime *lastEventTime_;

  handle setupPublication();
#ifdef NS_DIFFUSION
  PushSendDataTimer sdt_;
#endif // NS_DIFFUSION
};

#endif // !_PUSH_SENDER_HH_
