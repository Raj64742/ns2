// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//

/****************************************************************/
/* diffusion.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/****************************************************************/

// Important Note: Work still in progress ! 

#ifndef ns_diffusion_h
#define ns_diffusion_h

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include <tcl.h>
#include <stdlib.h>

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
#include "routing_table.h"


#define THIS_NODE             here_.addr_
#define MAC_RETRY_            0          // 0: NO RETRY
                                         // 1: RETRY
                                         // But, we only have no retry now!
#define JITTER                0.11       // (sec) to jitter broadcast
#define ARP_BUFFER_CHECK      0.03       // (sec) between buffer checks
#define ARP_MAX_ATTEMPT       3          // (times)
#define ARP_BUF_SIZE          64         // (slots)

#define SEND_BUFFER_CHECK     0.03       // (sec) between buffer checks
#define SEND_TIMEOUT          30.0       // (sec) a packet can live in sendbuf
#define SEND_BUF_SIZE         64         // (slots)


#define SEND_MESSAGE(x,y,z)  send_to_dmux(prepare_message(x,y,z), 0)
#define max(a,b) (((a)<(b))?(b):(a))

class DiffusionAgent;


class ArpBufEntry {
 public:
  Time   t;                            // Insertion Time
  int    attempt;                      // number of attempts
  Packet *p;

  ArpBufEntry() { t=0; attempt=0; p=NULL; }
};


class ArpBufferTimer : public TimerHandler {
 public:
  ArpBufferTimer(DiffusionAgent *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
 protected:
  DiffusionAgent *a_;
};


class SendBufferEntry {
 public:
  Time    t;                          // Insertion Time 
  Packet *p;

  SendBufferEntry() { t=0; p=NULL; }
};


class SendBufTimer : public TimerHandler {
 public:
  SendBufTimer(DiffusionAgent *a) : TimerHandler() { a_ = a; }
  void expire(Event *e);
 protected:
  DiffusionAgent *a_;
};


class DiffusionAgent : public Agent {
 public:
  DiffusionAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);

  Diff_Routing_Entry  routing_table[MAX_DATA_TYPE];


 protected:

  // Variables start here.

  bool POS_REINF_;
  bool NEG_REINF_;

  int pk_count;
  int overhead;

  Pkt_Hash_Table PktTable;

  Node *node;
  Trace *tracetarget;       // Trace Target
  NsObject *ll;  
  NsObject *port_dmux;
  ARPTable *arp_table;

  ArpBufferTimer     arp_buf_timer;
  ArpBufEntry        arp_buf[ARP_BUF_SIZE];
  SendBufTimer       send_buf_timer;
  SendBufferEntry    send_buf[SEND_BUF_SIZE];

  // Methods start here.

  inline void send_to_dmux(Packet *pkt, Handler *h) { 
    port_dmux->recv(pkt, h); 
  }

  void clear_arp_buf();
  void clear_send_buf();
  void reset();
  void consider_old(Packet *);
  void consider_new(Packet *);
  void Terminate();
  virtual void Start();

  Packet *create_packet();
  Packet *prepare_message(unsigned int dtype, ns_addr_t to_addr, int msg_type);

  virtual void Print_IOlist();
  void DataForSink(Packet *);
  void StopSource();

  void MACprepare(Packet *pkt, nsaddr_t next_hop, int type, 
		  bool lk_dtct);
  void MACsend(Packet *pkt, Time delay=0);
  void xmitFailed(Packet *pkt);

  void StickPacketInArpBuffer(Packet *pkt);
  void ArpBufferCheck();
  void SendBufferCheck();
  void StickPacketInSendBuffer(Packet *p);

  void trace(char *fmt,...);

  friend void XmitFailedCallback(Packet *pkt, void *data);
  friend class ArpBufferTimer;
  friend class SendBufTimer;
};

#endif


