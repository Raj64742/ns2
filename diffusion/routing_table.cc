/********************************************************************/
/* routing_table.cc : Chalermek Intanagonwiwat (USC/ISI)  05/18/99  */
/********************************************************************/

// Important Note: Work still in progress !

extern "C" {
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>
}

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
#include "diffusion.h"
#include "diff_rate.h"
#include "iflist.h"
#include "hash_table.h"
#include "arp.h"
#include "mac.h"
#include "ll.h"
#include "dsr/path.h"
#include "god.h"
#include "routing_table.h"


void Routing_Entry::reset()
{
  counter = 0;
  num_active = 0;
  num_iif = 0;
  clear_outlist(active);
  clear_outlist(inactive);
  clear_inlist(iif);
  clear_inlist(down_iif);
  clear_agentlist(source);
  clear_agentlist(sink);
  active = NULL;
  inactive = NULL;
  iif = NULL;
  down_iif = NULL;
  source = NULL;
  sink = NULL;
  last_fwd_time = -2.0*INTEREST_PERIODIC;
  new_org_counter = 0;
}


void Routing_Entry::clear_outlist(Out_List *list)
{
  Out_List *cur=list;
  Out_List *temp = NULL;

  while (cur != NULL) {
    temp = OUT_NEXT(cur);
    delete cur;
    cur = temp;
  }
  
}


void Routing_Entry::clear_inlist(In_List *list)
{
  In_List *cur=list;
  In_List *temp = NULL;

  while (cur != NULL) {
    temp = IN_NEXT(cur);
    delete cur;
    cur = temp;
  }
}



void Routing_Entry::clear_agentlist(Agent_List *list)
{
  Agent_List *cur=list;
  Agent_List *temp = NULL;

  while (cur != NULL) {
    temp = AGENT_NEXT(cur);
    delete cur;
    cur = temp;
  }
}


int Routing_Entry::MostRecvOrg()
{
  In_List *cur;
  int     most = 0;

  for (cur=iif; cur!=NULL; cur = IN_NEXT(cur)) {
      most = max(most,NEW_ORG_RECV(cur));
  }
  return most;
}


bool Routing_Entry::ExistOriginalGradient()
{
  Out_List *cur_out;

  for (cur_out = active; cur_out!=NULL; cur_out=OUT_NEXT(cur_out)) {
    if (GRADIENT(cur_out) == ORIGINAL)
      return true;
  }

  return false;
}


void Routing_Entry::IncRecvCnt(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  counter++;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     TOTAL_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  TOTAL_RECV(AddInList(agent_addr))++;

}


void Routing_Entry::CntPosSend(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NUM_POS_SEND(RetVal.cur)++;
     return;
  } 

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NUM_POS_SEND(cur_in)++;
}


void Routing_Entry::CntNeg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(active, agent_addr);
  if (RetVal.cur != NULL) {
     NUM_NEG_RECV(RetVal.cur)++;
     return;
  } 
  /*
  perror("Hey man. How come you send me the negative reinforment?\n");
  exit(-1);
  */
}


void Routing_Entry::CntNewSub(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_SUB_RECV(RetVal.cur)++;
     TOTAL_NEW_SUB_RECV(RetVal.cur)++;
     LAST_TS_NEW_SUB(RetVal.cur) = NOW;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NEW_SUB_RECV(cur_in)++;
  TOTAL_NEW_SUB_RECV(cur_in)++;
  LAST_TS_NEW_SUB(cur_in) = NOW;
}


void Routing_Entry::ClrNewSub(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_SUB_RECV(RetVal.cur)= 0;
     LAST_TS_NEW_SUB(RetVal.cur) = -1.0;
     return;
  } 
}


void Routing_Entry::CntNewOrg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     NEW_ORG_RECV(RetVal.cur)++;
     TOTAL_NEW_ORG_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  NEW_ORG_RECV(cur_in)++;
  TOTAL_NEW_ORG_RECV(cur_in)++;
}


void Routing_Entry::CntOldOrg(ns_addr_t agent_addr)
{
  PrvCurPtr RetVal;

  RetVal=INTF_FIND(iif, agent_addr);
  if (RetVal.cur != NULL) {
     OLD_ORG_RECV(RetVal.cur)++;
     TOTAL_OLD_ORG_RECV(RetVal.cur)++;
     return;
  } 

  RetVal=INTF_FIND(source, agent_addr);
  if (RetVal.cur != NULL)
    return;

  // On-demand adding In_List 

  In_List *cur_in = AddInList(agent_addr);
  OLD_ORG_RECV(cur_in)++;
  TOTAL_OLD_ORG_RECV(cur_in)++;
}


void Routing_Entry::ClrAllNewOrg()
{
  In_List *cur;

  for (cur = iif; cur!= NULL; cur = IN_NEXT(cur)) {
     NEW_ORG_RECV(cur)= 0;
  }
}


void Routing_Entry::ClrAllOldOrg()
{
  In_List *cur;

  for (cur=iif; cur!=NULL; cur = IN_NEXT(cur) ) {
     OLD_ORG_RECV(cur)= 0;
  }
}


In_List *Routing_Entry::MostRecentIn()
{
  In_List *cur, *ret;
  double recent_time;

  ret = NULL;
  recent_time = -1.0;
  for (cur = iif; cur!=NULL; cur=IN_NEXT(cur)) {
    if (LAST_TS_NEW_SUB(cur) > recent_time) {
      ret = cur;
      recent_time = LAST_TS_NEW_SUB(cur);
    }
  }
  return ret;
}


In_List * Routing_Entry::AddInList(ns_addr_t addr)
{
  In_List *inPtr= new In_List;

  AGT_ADDR(inPtr) = addr;
  INTF_INSERT(iif, inPtr); 

  num_iif++;
  return inPtr;
}


Routing_Entry:: Routing_Entry() 
{ 
    last_fwd_time = -2.0*INTEREST_PERIODIC;  
    counter      = 0;
    new_org_counter = 0;
    num_active   = 0;
    num_iif      = 0;
    active       = NULL; 
    inactive     = NULL; 
    iif          = NULL;
    down_iif     = NULL;
    source       = NULL;
    sink         = NULL;
}


