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
/* diff_prob.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/****************************************************************/

// Important Note: Work still in progress !!! Major improvement is needed.

#ifndef ns_diff_prob_h
#define ns_diff_prob_h

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
#include "routing_table.h"
#include "diffusion.h"


//#define DEBUG_PROB

#define BACKTRACK_            false
#define ENERGY_CHECK          0.05         // (sec) between energy checks
#define INTEREST_DELAY        0.05         // (sec) bw receive and forward
#define MAX_REINFORCE_COUNTER 10           // (pkts) bw pos reinf

class DiffusionProb;

class EnergyTimer : public TimerHandler {
public:
	EnergyTimer(DiffusionProb *a, Node *b) : TimerHandler() { 
		a_ = a; 
		node_ = b;
		init_eng_ = node_->energy_model()->energy();
		threshold_ = init_eng_ / 2;
	}
	virtual void expire(Event *e);
protected:
	DiffusionProb *a_;
	Node *node_;
	double init_eng_;
	double threshold_;
};


class InterestTimer : public TimerHandler {
public:
  InterestTimer(DiffusionProb *a, Pkt_Hash_Entry *hashPtr, Packet *pkt) : 
    TimerHandler() 
  { 
      a_ = a; 
      hashPtr_ = hashPtr;
      pkt_ = pkt;
  }

  virtual ~InterestTimer() {
    if (pkt_ != NULL) 
      Packet::free(pkt_);
  }

  virtual void expire(Event *e);
protected:
  DiffusionProb *a_;
  Pkt_Hash_Entry *hashPtr_;
  Packet *pkt_;
};



class DiffusionProb : public DiffusionAgent {
 public:
  DiffusionProb();
  void recv(Packet*, Handler*);

 protected:

  int num_neg_bcast_send;
  int num_neg_bcast_rcv;

  EnergyTimer *energy_timer;
  bool is_low_power;

  void Start();
  void consider_old(Packet *);
  void consider_new(Packet *);
  void add_outlist(unsigned int, From_List *);
  void data_request_all(unsigned int dtype);

  void CreateIOList(Pkt_Hash_Entry *, unsigned int);
  void UpdateIOList(From_List *, unsigned int);
  void Print_IOlist();

  void CalGradient(unsigned int);
  void IncGradient(unsigned int, ns_addr_t);
  void DecGradient(unsigned int, ns_addr_t);

  void ForwardData(Packet *);
  void ForwardTxFailed(Packet *);
  void ReTxData(Packet *);

  void GenPosReinf(unsigned int);
  void FwdPosReinf(unsigned int, Packet *);
  void InterfaceDown(int, ns_addr_t);
  void SendInhibit(int);
  void SendNegReinf();

  void InterestPropagate(Packet *pkt, Pkt_Hash_Entry *hashPtr);
  void xmitFailed(Packet *pkt);

  friend class InterestTimer;
  friend class EnergyTimer;
};

#endif
