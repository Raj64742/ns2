//
// ping_sender.hh : Ping Sender Include File
// author         : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: ping_sender.hh,v 1.3 2002/03/20 22:49:39 haldar Exp $
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

#include "ping.hh"

class MySenderReceive;
class PingSenderApp;

#ifdef NS_DIFFUSION
#define SEND_DATA_INTERVAL 5

class sendDataTimer : public TimerHandler {
  public:
  sendDataTimer(PingSenderApp *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
protected:
  PingSenderApp *a_;
};
#endif //NS_DIFFUSION

class PingSenderApp : public DiffApp {
public:
#ifdef NS_DIFFUSION
  PingSenderApp();
  int command(int argc, const char*const* argv);
  void send();
#else
  PingSenderApp(int argc, char **argv);
#endif // NS_DIFFUSION

  void run();
  void recv(NRAttrVec *data, NR::handle my_handle);

private:
  // NR Specific variables
  MySenderReceive *mr;
  handle subHandle;
  handle pubHandle;

  // Ping App variables
  int last_seq_sent;
  NRAttrVec data_attr;
  NRSimpleAttribute<void *> *timeAttr;
  NRSimpleAttribute<int> *counterAttr;
  EventTime *lastEventTime;

  handle setupSubscription();
  handle setupPublication();
#ifdef NS_DIFFUSION
  sendDataTimer sdt;
#endif // NS_DIFFUSION
};

class MySenderReceive : public NR::Callback {
public:
  PingSenderApp *app;

  void recv(NRAttrVec *data, NR::handle my_handle);
};
