//
// ping_sender.hh : Ping Sender Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// $Id: 2pp_ping_sender.hh,v 1.1 2004/01/08 22:56:40 haldar Exp $
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

#ifndef _PING_SENDER_HH_
#define _PING_SENDER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ping.hh"

class TPPPingSenderReceive;
class TPPPingSenderApp;

#define SEND_DATA_INTERVAL 5

#ifdef NS_DIFFUSION
class TPPPingSendDataTimer : public TimerHandler {
  public:
  TPPPingSendDataTimer(TPPPingSenderApp *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  TPPPingSenderApp *a_;
};
#endif //NS_DIFFUSION

class TPPPingSenderApp : public DiffApp {
public:
#ifdef NS_DIFFUSION
  TPPPingSenderApp();
  int command(int argc, const char*const* argv);
  void send();
#else
  TPPPingSenderApp(int argc, char **argv);
#endif // NS_DIFFUSION

  virtual ~TPPPingSenderApp()
  {
    // Nothing to do
  };

  void run();
  void recv(NRAttrVec *data, NR::handle my_handle);

private:
  // NR Specific variables
  TPPPingSenderReceive *mr_;
  handle subHandle_;
  handle pubHandle_;

  // Ping App variables
  int num_subscriptions_;
  int last_seq_sent_;
  NRAttrVec data_attr_;
  NRSimpleAttribute<void *> *timeAttr_;
  NRSimpleAttribute<int> *counterAttr_;
  EventTime *lastEventTime_;

  handle setupSubscription();
  handle setupPublication();
#ifdef NS_DIFFUSION
  TPPPingSendDataTimer sdt_;
#endif // NS_DIFFUSION
};

class TPPPingSenderReceive : public NR::Callback {
public:
  TPPPingSenderReceive(TPPPingSenderApp *app) : app_(app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);

  TPPPingSenderApp *app_;
};

#endif // !_PING_SENDER_HH_
