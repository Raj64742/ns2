//
// ping_receiver.hh : Ping Receiver Include File
// author           : Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: ping_receiver.hh,v 1.2 2002/03/20 22:49:39 haldar Exp $
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

class MyReceiverReceive;

class PingReceiverApp : public DiffApp {
public:
#ifdef NS_DIFFUSION
  PingReceiverApp();
  int command(int argc, const char*const* argv);
#else
  PingReceiverApp(int argc, char **argv);
#endif // NS_DIFFUSION

  void recv(NRAttrVec *data, NR::handle my_handle);
  void run();

private:
  // NR Specific variables
  MyReceiverReceive *mr;
  handle subHandle;
  handle pubHandle;

  // Ping App variables
  int last_seq_recv;
  int num_msg_recv;
  int first_msg_recv;

  handle setupSubscription();
  handle setupPublication();
};

class MyReceiverReceive : public NR::Callback {
public:
  PingReceiverApp *app;

  MyReceiverReceive(PingReceiverApp *_app) : app(_app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);
};
