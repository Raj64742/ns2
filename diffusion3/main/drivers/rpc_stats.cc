// *********************************************************
// 
// rpc_stats.cc  : Collect statistics from the RPC radio
// authors       : Chalermek Intanagonwiwat and Fabio Silva
//
// $Id: rpc_stats.cc,v 1.4 2002/03/20 22:49:41 haldar Exp $
//
// *********************************************************

#include "rpc_stats.hh"

RPCStats::RPCStats(int id){
  // Zero counters
  rpc_tx_bytes = 0;
  rpc_rx_bytes = 0;
  rpc_tx_frames = 0;
  rpc_rx_frames = 0;

#ifndef __MOTE_NIC__  
  // Initialize Radiometrix counters
  openFiles();
  
  fprintf(tx_bytes, "%d", rpc_tx_bytes);
  fprintf(rx_bytes, "%d", rpc_rx_bytes);
  fprintf(tx_frames, "%d", rpc_tx_frames);
  fprintf(rx_frames, "%d", rpc_rx_frames);
  
  closeFiles();
#endif
  
  node_id = id;
}

void RPCStats::readCounters()
{
#ifndef __MOTE_NIC__
  openFiles();

  fscanf(tx_bytes, "%d", &rpc_tx_bytes);
  fscanf(rx_bytes, "%d", &rpc_rx_bytes);
  fscanf(tx_frames, "%d", &rpc_tx_frames);
  fscanf(rx_frames, "%d", &rpc_rx_frames);

  closeFiles();
#endif
}

void RPCStats::openFiles()
{
  tx_bytes = fopen("/proc/krpc/tx_bytes", "w");
  rx_bytes = fopen("/proc/krpc/rx_bytes", "w");
  tx_frames = fopen("/proc/krpc/tx_frames", "w");
  rx_frames = fopen("/proc/krpc/rx_frames", "w");

  if (!tx_bytes || !rx_bytes || !tx_frames || !rx_frames){
    printf("Error: Can't talk to RPC driver !\n");
    exit(-1);
  }
}

void RPCStats::closeFiles()
{
  fclose(tx_bytes);
  fclose(rx_bytes);
  fclose(tx_frames);
  fclose(rx_frames);
}

void RPCStats::printStats(FILE *output)
{
#ifndef __MOTE_NIC__  
  fprintf(output, "Radiometrix Stats\n");
  fprintf(output, "-----------------\n\n");
  fprintf(output, "Node id : %d\n\n", node_id);

  fprintf(output, "RPC : Bytes sent  : %d - Bytes received  : %d\n",
	  rpc_tx_bytes, rpc_rx_bytes);
  fprintf(output, "RPC : Frames sent : %d - Frames received : %d\n",
	  rpc_tx_frames, rpc_rx_frames);
  fprintf(output, "\n");
#else
/*
  fprintf(output, "Mote-Nic Stats\n");
  fprintf(output, "-----------------\n\n");
  fprintf(output, "Node id : %d\n\n", node_id);

  fprintf(output, "Mote-Nic : Bytes sent  : %d - Bytes received  : %d\n",
	  rpc_tx_bytes, rpc_rx_bytes);
  fprintf(output, "Mote-Nic : Frames sent : %d - Frames received : %d\n",
	  rpc_tx_frames, rpc_rx_frames);
  fprintf(output, "\n");
*/
#endif   
}
