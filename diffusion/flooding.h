/****************************************************************/
/* flooding.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99    */
/****************************************************************/

// Share api with diffusion and omnicient multicast
// Using diffusion packet header


#ifndef ns_flooding_h
#define ns_flooding_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>

#include <tcl.h>

#include "diff_header.h"
#include "agent.h"
#include "tclcl.h"
#include "ip.h"
#include "config.h"
#include "packet.h"
#include "trace.h"
#include "random.h"
#include "classifier.h"
#include "node.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"


#define THIS_NODE             here_.addr_
#define JITTER                0.11       // (sec) to jitter broadcast

#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)


// Flooding Entry

class Flooding_Entry {
 public:
  Agent_List *source;
  Agent_List *sink;

  Flooding_Entry() { 
    source       = NULL;
    sink         = NULL;
  }

 void reset();
 void clear_agentlist(Agent_List *);
};


// Flooding Agent

class FloodingAgent : public Agent {
 public:
  FloodingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  Flooding_Entry routing_table[MAX_DATA_TYPE];

 protected:
  int pk_count;
  Pkt_Hash_Table PktTable;

  Node *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;

  inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void Terminate();
  void reset();
  void ConsiderNew(Packet *pkt);
 
  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  void DataForSink(Packet *pkt);
  void StopSource();
  void MACprepare(Packet *pkt);
  void MACsend(Packet *pkt, Time delay=0);

  void trace(char *fmt,...);

};



#endif




