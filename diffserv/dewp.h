// Copyright (c) 1999 by the University of Southern California
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
// dewp.h (Early worm propagation detection)
//   by Xuan Chen (xuanc@isi.edu), USC/ISI

#ifndef DEWP_H
#define DEWP_H

#include "packet.h"
#include "dsPolicy.h"

#define DT_INV 2
#define DB_INV DT_INV
#define P_LEN 65536 

// list to store addresses
struct AddrEntry {
  unsigned int addr;
  struct AddrEntry *next;
};

// List for blocked port
struct BPEntry {
  int s, b;
  int anum, last_anum, avg_anum;
  double last_time;
  struct AddrEntry *aset;
};

// Eearly worm propagation detection policy
class DEWPPolicy : public Policy {
 public:
  DEWPPolicy();
  ~DEWPPolicy();

  void init(double);
  
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

  //  make packet drop decisions
  int dropPacket(Packet *);
  // detect if there is an alarm triggered
  void detect(Packet *pkt);
  
  // couple in- and out-bound queues
  void couple(DEWPPolicy *);

  //protected:
  DEWPPolicy *cdewp;

  double dport_list[P_LEN];
  static BPEntry *bport_list;

 private:
  // The current time
  double now;

  // commom paramters
  // detection interval
  static double dt_inv_;
  // change tolerance
  static double beta;
  // gain
  static double alpha;
  // absolute threshold to filter out some small values (num/s)
  static int anum_th;
};

//int *DEWPPolicy::bport_list = NULL;

#endif
