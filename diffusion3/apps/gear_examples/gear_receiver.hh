//
// gear_receiver.hh : Gear Receiver Include File
// author           : Fabio Silva
//
// Copyright (C) 2000-2003 by the University of Southern California
// $Id: gear_receiver.hh,v 1.2 2003/07/10 21:18:55 haldar Exp $
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

#ifndef _GEAR_RECEIVER_HH_
#define _GEAR_RECEIVER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "gear_common.hh"

class GearReceiverReceive;

class GearReceiverApp : public DiffApp {
public:
  #ifdef NS_DIFFUSION
  GearReceiverApp();
  int command(int argc, const char*const* argv);
#else
  GearReceiverApp(int argc, char **argv);
#endif //NS_DIFFUSION
  void recv(NRAttrVec *data, NR::handle my_handle);
  void run();

private:
  // NR Specific variables
  GearReceiverReceive *mr_;
  handle subHandle_;

  // Gear Receiver App variables
  bool using_points_;
  bool using_push_;
  int last_seq_recv_;
  int num_msg_recv_;
  int first_msg_recv_;
  float lat_min_;
  float lat_max_;
  float long_min_;
  float long_max_;
  float lat_pt_;
  float long_pt_;

  handle setupSubscription();
  void parseCommandLine(int argc, char **argv);
  void readGeographicCoordinates();
  void usage(char *s);
};

class GearReceiverReceive : public NR::Callback {
public:
  GearReceiverReceive(GearReceiverApp *app) : app_(app) {};
  void recv(NRAttrVec *data, NR::handle my_handle);

  GearReceiverApp *app_;
};

#endif // !_GEAR_RECEIVER_HH_
