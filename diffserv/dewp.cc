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

#include "ip.h"
#include "tcp.h"
#include "tcp-full.h"
#include "random.h"

#include "dewp.h"
BPEntry *DEWPPolicy::bport_list = NULL;
// Initialize parameters
double DEWPPolicy::dt_inv_ = DT_INV;
double DEWPPolicy::beta = 0.5;
double DEWPPolicy::alpha = 0.125;
// num per second
int DEWPPolicy::anum_th = 50;

// EW Policy: deal with queueing stuffs.
//Constructor.  
DEWPPolicy::DEWPPolicy() : Policy() {
  // Initialize detectors
  cdewp = NULL;

  bport_list = new BPEntry[P_LEN];
  for (int i = 0; i < P_LEN; i++) {
    dport_list[i] = 0;
    bport_list[i].s = bport_list[i].b = 0;
    bport_list[i].anum = bport_list[i].last_anum = bport_list[i].avg_anum = 0;
    bport_list[i].last_time = 0;
    bport_list[i].aset = NULL;
  }
}

//Deconstructor.
DEWPPolicy::~DEWPPolicy(){
  if (cdewp)
    free(cdewp);

  for (int i = 0; i < P_LEN; i++) {
    if (bport_list[i].aset) {
      AddrEntry * p = bport_list[i].aset;
      AddrEntry * q;
      while (p) {
        q = p->next;
        free(p);
        p = q;
      }
    }
  }
}

// Initialize the DEWP parameters
void DEWPPolicy::init(double dt_inv) {
  dt_inv_ = dt_inv;
  //printf("%f\n", dt_inv_);
}

// DEWP meter: do nothing.
//  measurement is done in policer: we need to know whether the packet is
//    dropped or not.
void DEWPPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  hdr_ip* iph = hdr_ip::access(pkt);

  dport_list[iph->dport()] = Scheduler::instance().clock();

  return;
}

// DEWP Policer
//  1. do measurement: P: both arrival and departure; B: only departure
//  2. make packet drop decisions
int DEWPPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt) {
  //printf("enter applyPolicer ");

  // can't count/penalize ACKs:
  //   with resp: may cause inaccurate calculation with TSW(??)
  //   with req:  may cause resp retransmission.
  // just pass them through
  hdr_ip* iph = hdr_ip::access(pkt);
  detect(pkt);
  if (bport_list[iph->dport()].b) {
    //printf("downgrade!\n");	
    return(policer->downgrade1);
  } else {
    //printf("initial!\n");	
    return(policer->initialCodePt);
  }
}

// detect if there is alarm triggered
void DEWPPolicy::detect(Packet *pkt) {
  // it is not for outbound traffic
  if (!cdewp)
    return;

  // get the current time
  now = Scheduler::instance().clock();

  // get IP header
  hdr_ip* iph = hdr_ip::access(pkt);
  int dport = iph->dport();
  unsigned int daddr = iph->daddr();

  // use dport matching to find suspects
  if (dport_list[dport] > 0 && (cdewp->dport_list[dport]) > 0 &&
      now - dport_list[dport]< dt_inv_ &&
      now - cdewp->dport_list[dport] < dt_inv_) {
	if (bport_list[dport].s == 0) {
      printf("S %.2f %d\n", now, dport);
      bport_list[dport].s = 1;
    }
  }

  // count outbound traffic only
  if (bport_list[dport].s == 1 && daddr > 0) {
    AddrEntry * p = bport_list[dport].aset; 
    
    while (p) {
      if (p->addr == daddr) {
        break;
      }
      p = p->next;
    }
    
    if (!p) {
      AddrEntry * new_addr = new AddrEntry;
      
      new_addr->addr = daddr;
      new_addr->next = NULL;
      
      if (bport_list[dport].aset) {
        new_addr->next = bport_list[dport].aset;
      };
      bport_list[dport].aset = new_addr;
    }
    bport_list[dport].anum++;
   
    /* debug purpose only
    p = bport_list[dport].aset;
    printf("[%d] ", dport);
    while (p) {
      printf("%u ", p->addr);
      p = p->next;
    }
    printf("\n");
    */

    if (now - bport_list[dport].last_time > dt_inv_) {
          //printf("DT %f %d %d %d\n", now, dport, bport_list[dport].anum, bport_list[dport].avg_anum);
      if (bport_list[dport].anum > anum_th * dt_inv_ &&
          bport_list[dport].avg_anum > 0 &&
          bport_list[dport].anum > (1 + beta) * bport_list[dport].avg_anum) {
        if (bport_list[dport].b == 0) {
          bport_list[dport].b = 1;
          printf("B %.2f %d %d %d\n", now, dport, bport_list[dport].anum, bport_list[dport].avg_anum);
        }
      }
      
      bport_list[dport].avg_anum = (int) (alpha * bport_list[dport].avg_anum + 
                                       (1 - alpha) * bport_list[dport].anum);
      if (bport_list[dport].avg_anum == 0 && bport_list[dport].anum > 0)
      	bport_list[dport].avg_anum = bport_list[dport].anum;
      bport_list[dport].last_anum = bport_list[dport].anum;
      bport_list[dport].anum = 0;
      bport_list[dport].last_time = now;
    }
    
  }
  
}

//  make packet drop decisions
int DEWPPolicy::dropPacket(Packet *pkt) {
 
  return(0);
}

// couple DEWP detector
void DEWPPolicy::couple(DEWPPolicy *ewpc) {
  cdewp = ewpc;
}

// End of DEWP
