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
/* diff_rate.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/****************************************************************/

// Important Note: Work still in progress !!!

#ifndef ns_diff_rate_h
#define ns_diff_rate_h

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


//#define DEBUG_RATE

typedef enum subsample_tx_type__ {
  BCAST_SUB,
  UNICAST_SUB
} sub_t;


typedef enum original_tx_type__ {
  BCAST_ORG,
  UNICAST_ORG
} org_t;


typedef enum pos_reinf_type__ {
  POS_HASH,
  POS_LAST,
  POS_ALL
} pos_t;


typedef enum pos_reinf_node_type__ {
  END_POS,
  INTM_POS
} pos_ndt;


typedef enum neg_window_type__ {
  NEG_COUNTER,
  NEG_TIMER
} neg_wint;


// For NEG_COUNTER Only, 
// this threshold defines which neighbors will be negatively reinfoced.

typedef enum neg_threshold_type__ {
  NEG_ABSOLUTE,
  NEG_RELATIVE
} neg_tht;


// For NEG_COUNTER Only,
// The max counter defines when it is time to negatively reinforce.

typedef enum neg_max_counter_type__ {
  NEG_FIXED_MAX,
  NEG_SCALE_MAX
} neg_mxt;


#define INTEREST_PERIODIC     5            // (sec) between interest 
#define INTEREST_TIMEOUT      15           // (sec)  

#define NEG_CHECK             2.0         // (sec) bw neg checks
#define MAX_NEG_COUNTER       20           // (new original pkts) bw neg reinf
#define PER_IIF               5            // new org data for scale max 
                                           // counter
#define NEG_MIN_RATIO         0.1          // min ratio compared to the max iif
#define MAX_DUP_DATA          0            // max dupplicate bw neg reinf


class DiffusionRate;

class GradientTimer : public TimerHandler {
public:
  GradientTimer(DiffusionRate *a) : TimerHandler() 
  { 
      a_ = a; 
  }
  virtual void expire(Event *e=NULL);
protected:
  DiffusionRate *a_;
};


class NegativeReinforceTimer : public TimerHandler {
public:
  NegativeReinforceTimer(DiffusionRate *a) : TimerHandler() 
  { 
      a_ = a; 
  }
  virtual void expire(Event *e=NULL);
protected:
  DiffusionRate *a_;
};


class DiffusionRate : public DiffusionAgent {
 public:
  DiffusionRate();
  void recv(Packet*, Handler*);
  int command(int argc, const char*const* argv);

 protected:

  bool DUP_SUP_;

  sub_t sub_type_;
  org_t org_type_;
  pos_t pos_type_;
  pos_ndt pos_node_type_;
  neg_wint neg_win_type_;
  neg_tht neg_thr_type_;
  neg_mxt neg_max_type_;

  Data_Hash_Table DataTable;

  GradientTimer *gradient_timer;
  NegativeReinforceTimer *neg_reinf_timer;

  int num_not_send_bcast_data;
  int num_data_bcast_send;
  int num_data_bcast_rcv;
  int num_neg_bcast_send;
  int num_neg_bcast_rcv;

  void reset();
  void consider_old(Packet *);
  void consider_new(Packet *);
  void Start();

  void DataReqAll(unsigned int dtype, int report_rate);
  void Print_IOlist();
  void FwdData(Packet *);
  void PosReinf(int dtype, nsaddr_t to_node, ns_addr_t info_sender, 
		      unsigned int info_seq);
  void ProcessPosReinf(Packet *pkt);
  void ProcessNegReinf(Packet *pkt);
  void UcastNeg(int dtype, ns_addr_t to);
  void BcastNeg(int dtype);
  void GenNeg(int dtype);
  void InterestHandle(Packet *pkt);

  void GradientTimeOut();
  void NegReinfTimeOut();

  void CheckNegCounter(int dtype);
  bool FwdSubsample(Packet *pkt);
  void FwdOriginal(Packet *pkt);
  void TriggerPosReinf(Packet *pkt, ns_addr_t forward_agent);

  friend class GradientTimer;
  friend class NegativeReinforceTimer;
};


sub_t ParseSubType(const char* str);
org_t ParseOrgType(const char* str);
pos_t ParsePosType(const char* str);
pos_ndt ParsePosNodeType(const char* str);
neg_wint ParseNegWinType(const char* str);
neg_tht ParseNegThrType(const char* str);
neg_mxt ParseNegMaxType(const char* str);

#endif


