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

/********************************************************************/
/* routing_table.h : Chalermek Intanagonwiwat (USC/ISI)  08/16/99   */
/********************************************************************/

// Important Note: Work still in progress ! 

#ifndef ns_routing_table_h
#define ns_routing_table_h

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


// Routing Entry

class Diff_Routing_Entry {
 public:
  int      counter;
  int      num_active;
  int      num_iif;
  Out_List *active;                    // active and up oif   	       
  Out_List *inactive;                  // inactive and down oif 
  In_List  *iif;                       // active and up iif
  In_List  *down_iif;                  // inactive and down iif
  Agent_List *source;
  Agent_List *sink;

  double   last_fwd_time;              // the last time forwarding interest
                                       // For Diffusion/RateGradient
  int new_org_counter;                 // Across all incoming gradients.

  Diff_Routing_Entry();

  void reset();
  void clear_outlist(Out_List *);
  void clear_inlist(In_List *);
  void clear_agentlist(Agent_List *);
  int MostRecvOrg();
  bool ExistOriginalGradient();

  void IncRecvCnt(ns_addr_t);

  void CntPosSend(ns_addr_t);
  void CntNeg(ns_addr_t);
  void CntNewSub(ns_addr_t);
  void ClrNewSub(ns_addr_t);
  void CntNewOrg(ns_addr_t);
  void CntOldOrg(ns_addr_t);
  void ClrAllNewOrg();
  void ClrAllOldOrg();

  In_List *MostRecentIn();
  In_List *AddInList(ns_addr_t);
};

#endif

