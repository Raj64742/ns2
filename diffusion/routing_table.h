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

class Routing_Entry {
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

  Routing_Entry();

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

