//
// ping_receiver.hh : Ping Receiver Include File
// author           : Fabio Silva
//
// Copyright (C) 2000-2002 by the Unversity of Southern California
// $Id: ping_receiver.hh,v 1.2 2002/05/07 00:09:38 haldar Exp $
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

#ifndef _PING_RECEIVER_HH_
#define _PING_RECEIVER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ping.hh"

class PingReceiverReceive;

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
  PingReceiverReceive *mr_;
  handle subHandle_;

  // Ping App variables
  int last_seq_recv_;
  int num_msg_recv_;
  int first_msg_recv_;

  handle setupSubscription();
};

class PingReceiverReceive : public NR::Callback {
public:
  PingReceiverReceive(PingReceiverApp *app) : app_(app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);

  PingReceiverApp *app_;
};

#endif // !_PING_RECEIVER_HH_
