/// 
// events.cc     : Provides the eventQueue
// authors       : Lewis Girod and Fabio Silva
//
// Copyright (C) 2000-2001 by the Unversity of Southern California
// $Id: rpc_stats.hh,v 1.2 2001/11/20 22:28:18 haldar Exp $
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
// *********************************************************
// 
// rpc_stats.hh  : Collect statistics from the RPC radio
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// $Id: rpc_stats.hh,v 1.2 2001/11/20 22:28:18 haldar Exp $
//
// *********************************************************

#ifdef NS_DIFFUSION

#ifndef rpc_stats_hh
#define rpc_stats_hh

#include "stdio.h"
#include "stdlib.h"

class RPCStats {
public:
  RPCStats(int id);
  void printStats(FILE *output);

private:
  int node_id;

  // Counters
  int rpc_tx_bytes;
  int rpc_rx_bytes;
  int rpc_tx_frames;
  int rpc_rx_frames;

  // RPC Interface files
  FILE *tx_bytes;
  FILE *rx_bytes;
  FILE *tx_frames;
  FILE *rx_frames;

  void readCounters();
  void openFiles();
  void closeFiles();
};

#endif
#endif // NS
