/*
 * File: Header File for a new 'Ping' Agent Class for the ns
 *       network simulator
 * Author: Marc Greis (greis@cs.uni-bonn.de), May 1998
 *
 * IMPORTANT: Incase of any changes made to this file ,
 * tutorial/examples/ping.h  (used in Greis' tutorial) should
 * be updated as well.
 */


#ifndef ns_ping_h
#define ns_ping_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"


struct hdr_ping {
  char ret;
  double send_time;
};


class PingAgent : public Agent {
 public:
  PingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);
 protected:
  int off_ping_;
};


#endif
