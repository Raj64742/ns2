// *********************************************************
// 
// rpc_stats.hh  : Collect statistics from the RPC radio
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// $Id: rpc_stats.hh,v 1.1 2002/05/06 22:41:09 haldar Exp $
//
// *********************************************************

#ifndef rpc_stats_hh
#define rpc_stats_hh

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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
