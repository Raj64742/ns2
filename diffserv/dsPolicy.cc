/*
 * Copyright (c) 2000, Nortel Networks.
 * All rights reserved.
 *
 * License is granted to copy, to use, to make and to use derivative
 * works for research and evaluation purposes.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 *
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 * 
 * dsPolicy.cc
 *
 */

/* 
 *  Integrated into ns main distribution and reorganized by 
 *  Xuan Chen (xuanc@isi.edu). The main changes are:
 *  1. Created a general supper class Policy so that new policy can be added
 *     by just deriving from the supper class.
 *  2. Created a certain format for people who is trying to add their own
 *     new policies.
 *
 */

#include "dsPolicy.h"
#include "packet.h"
#include "tcp.h"
#include "random.h"

// The definition about the methods in the supper class Policy.
//Constructor.
Policy::Policy() {
  policyTableSize = 0;
  policerTableSize = 0;
}

/*-----------------------------------------------------------------------------
void Policy::addPolicyEntry()
Adds an entry to policyTable according to the arguments in argv, it sets the 
common parameters for different policies:
1. source and destination pair
2. the initial codePoint.
Policy type and policy-specific parameters will be specified by corresponding 
method in each policy.
No error-checking is performed on the parameters.  
If the Policy Table is full, this method prints an error message.
-----------------------------------------------------------------------------*/
void Policy::addPolicyEntry(int argc, const char*const* argv) {
  if (policyTableSize == MAX_POLICIES) {
    printf("ERROR: Policy Table size limit exceeded.\n");
    exit(-1);
  } else {
    policyTable[policyTableSize].sourceNode = atoi(argv[2]);
    policyTable[policyTableSize].destNode = atoi(argv[3]);
    policyTable[policyTableSize].codePt = atoi(argv[5]);
    policyTable[policyTableSize].arrivalTime = 0;
    policyTable[policyTableSize].winLen = 1.0;
  };
}

/*-----------------------------------------------------------------------------
void Policy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.  
Set the common parameters: initial codepoint, downgrade1/2 codes.
No error-checking is done on the arguments.  
If policerTable is full, an error message is printed.
-----------------------------------------------------------------------------*/
void Policy::addPolicerEntry(int argc, const char*const* argv) {
  if (policerTableSize == MAX_CP) {
    printf("ERROR: Policer Table size limit exceeded.\n");
    exit(-1);
  } else {
    policerTable[policerTableSize].initialCodePt = atoi(argv[3]);
    policerTable[policerTableSize].downgrade1 = atoi(argv[4]);
    if (argc == 6)
      policerTable[policerTableSize].downgrade2 = atoi(argv[5]);
  }
}

/*-----------------------------------------------------------------------------
policyTableEntry* Policy::getPolicyTableEntry(long source, long dest)
Pre: policyTable holds exactly one entry for the specified source-dest pair.
Post: Finds the policyTable array that matches the specified source-dest pair.
Returns: On success, returns a pointer to the corresponding policyTableEntry;
  on failure, returns NULL.
Note: the source-destination pair could be one-any or any-any (xuanc)
-----------------------------------------------------------------------------*/
policyTableEntry* Policy::getPolicyTableEntry(nsaddr_t source, nsaddr_t dest) {
  for (int i = 0; i <= policyTableSize; i++) {
    if ((policyTable[i].sourceNode == source) || (policyTable[i].sourceNode == ANY_HOST)) {
      if ((policyTable[i].destNode == dest) || (policyTable[i].destNode == ANY_HOST))
	return(&policyTable[i]);
    }
  }
  
  // !!! Could make a default code point for undefined flows:
  printf("ERROR: No Policy Table entry found for Source %d-Destination %d.\n", source, dest);
  printPolicyTable();
  return(NULL);
}

/*-----------------------------------------------------------------------------
void Policy::updatePolicyRTT(int argc, const char*const* argv)
Pre: The command line specifies a source and destination node for which an
  RTT-Aware policy exists and a current RTT value for that policy.
Post: The aggRTT field of the appropriate policy is updated to a weighted
  average of the previous value and the new RTT value specified in the command
  line.  If no matching policy is found, an error message is printed.
-----------------------------------------------------------------------------*/
void Policy::updatePolicyRTT(int argc, const char*const* argv) {
  policyTableEntry *policy;
  
  policy = getPolicyTableEntry(atoi(argv[2]), atoi(argv[3]));
  if (policy == NULL)   
    printf("ERROR: cannot update RTT; no existing policy found for Source %d-Desination %d.\n",
	   atoi(argv[2]), atoi(argv[3]));
  else {
    policy->winLen = (double) atof(argv[4]);
  }
}

/*-----------------------------------------------------------------------------
double Policy::getCBucket(int argc, const char*const* argv) {
Pre: The command line specifies a source and destination node for which a
  policy exists that uses a cBucket value.  That policy's cBucket parameter is
  currently valid.
Post: The policy's cBucket value is found and returned.
Returns: The value cBucket on success; or -1 on an error.
-----------------------------------------------------------------------------*/
double Policy::getCBucket(const char*const* argv) {
  policyTableEntry *policy;
  
  policy = getPolicyTableEntry(atoi(argv[2]), atoi(argv[3]));
  if (policy == NULL) {
    printf("ERROR: cannot get bucket size; no existing policy found for Source %d-Desination %d.\n",
	   atoi(argv[2]), atoi(argv[3]));
    return(-1);
  }
  else {
    if ((policy->policer == tokenBucketPolicer) || (policy->policer == srTCMPolicer) || (policy->policer == trTCMPolicer))
      return(policy->cBucket);
    else {
      printf("ERROR: cannot get bucket size; the Source %d-Desination %d Policy does not include a Committed Bucket.\n", atoi(argv[2]), atoi(argv[3]));
      return(-1);
    }
  }
}

/*-----------------------------------------------------------------------------
  int Policy::downgradeOne(policerType policer, int oldCodePt)
  Pre: policer is a valid policer; and policerTable has an entry corresponding to
  policer and oldCodePoint.
  Post: The policerTable is searched for a corresponding downgraded code point.
  Returns: On success, this method returns the downgraded code point.  (This code
  point is the yellow code point for a three color marker.)  On failure, this
  method prints an error message and returns -1.
-----------------------------------------------------------------------------*/
int Policy::downgradeOne(policerType policer, int oldCodePt) {
  
  for (int i = 0; i < policerTableSize; i++)
    if ((policerTable[i].policer == policer) &&
	(policerTable[i].initialCodePt == oldCodePt))
      return(policerTable[i].downgrade1);
  
  printf("ERROR: No Policer Table entry found for initial code point %d.\n", oldCodePt);
  return(-1);
}

/*-----------------------------------------------------------------------------
  int Policy::downgradeTwo(policerType policer, int oldCodePt)
  Pre: policer is a valid policer that uses at least three drop precedences.
  policerTable has an entry corresponding to policer and oldCodePoint.
  Post: The policerTable is searched for a corresponding downgraded code point.
  Returns: On success, this method returns the downgraded code point.  (This code
  point is the red code point for a three color marker.)  On failure, this
  method prints an error message and returns -1.
-----------------------------------------------------------------------------*/
int Policy::downgradeTwo(policerType policer, int oldCodePt) {
  if ((policer == TSW2CM) || (policer == tokenBucketPolicer) || (policer == FW)) {
    printf("ERROR: Attempt to downgrade to a third code point with a two code point policer.\n");
    return(-1);
  } else {
    for (int i = 0; i < policerTableSize; i++)
      if ((policerTable[i].policer == policer) && (policerTable[i].initialCodePt == oldCodePt))
	return(policerTable[i].downgrade2);
    
    printf("ERROR: No Policer Table entry found for initial code point %d.\n", oldCodePt);
    return(-1);
  }	
}

/*----------------------------------------------------------------------------
int Policy::mark(Packet *pkt, double minRTT)
Pre: The source-destination pair taken from pkt matches a valid entry in
  policyTable.
Post: pkt is marked with an appropriate code point.
-----------------------------------------------------------------------------*/
int Policy::mark(Packet *pkt) {
  policyTableEntry *policy;
  int codePt;
  hdr_ip* iph;
  int fid;
  
  iph = hdr_ip::access(pkt);
  fid = iph->flowid();
  
  //  printf("flow %d, %d -> %d\n", fid, iph->saddr(), iph->daddr());
  
  policy = getPolicyTableEntry(iph->saddr(), iph->daddr());
  codePt = policy->codePt;
  
  applyMeter(policy, pkt);
  codePt = applyPolicer(policy, codePt, pkt);

  iph->prio_ = codePt;
  return(codePt);
};

// The end of the definition of the methods in supper class Policy.

// The beginning of the definition of DumbPolicy
// DumbPolicy will do nothing, but is a good example to show how to add 
// new policy.
/*-----------------------------------------------------------------------------
void DumbPolicy::addPolicyEntry()
Adds an entry to policyTable according to the arguments in argv.  
The policy and requested parameter is DumbPolicer. 
-----------------------------------------------------------------------------*/
void DumbPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "Dumb") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);
    
    policyTable[policyTableSize].policer = dumbPolicer;
    policyTable[policyTableSize].meter = dumbMeter;
    
    policyTableSize++;
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void DumbPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.  
-----------------------------------------------------------------------------*/
void DumbPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "Dumb") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = dumbPolicer;

    policerTableSize++;
  }
}

/*-----------------------------------------------------------------------------
void DumbPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Do nothing
-----------------------------------------------------------------------------*/
void DumbPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  policy->arrivalTime = Scheduler::instance().clock();  
}

/*-----------------------------------------------------------------------------
int DumbPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Always return the initial codepoint.
-----------------------------------------------------------------------------*/
int DumbPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) {
  return(initialCodePt);
}

/*-----------------------------------------------------------------------------
void DumbPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void DumbPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): Dumbe policer\n",
	   policyTable[i].sourceNode,policyTable[i].destNode);
  }
  printf("\n");
}


/*-----------------------------------------------------------------------------
void DumbPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
-----------------------------------------------------------------------------*/
void DumbPolicy::printPolicerTable() {
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) 
    printf("Dumbe\n");
}
// The end of DumbPolicy

// The beginning of the definition of TSW2CM
/*-----------------------------------------------------------------------------
void TSW2CMPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  The policy and requested parameters are:

TSW2CM        InitialCodePoint  CIR

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.
------------------------------------------------------------------------------*/
void TSW2CMPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "TSW2CM") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);

    policyTable[policyTableSize].policer = TSW2CM;
    policyTable[policyTableSize].cir = 
      policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
    if (argc == 8) policyTable[policyTableSize].winLen = (double) atof(argv[7]);/* mb */
    policyTable[policyTableSize].meter = tswTagger;

    policyTableSize++;
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void TSW2CMPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.  
A policer type should be specified as TSW2CM followed by an initial code point.
Next should be an out-of-profile code point for policers with two-rate markers.
-----------------------------------------------------------------------------*/
void TSW2CMPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "TSW2CM") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = TSW2CM;
    
    policerTableSize++;
  }
}

/*-----------------------------------------------------------------------------
void TSW2CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
-----------------------------------------------------------------------------*/
void TSW2CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now, bytesInTSW, newBytes;
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  
  bytesInTSW = policy->avgRate * policy->winLen;
  newBytes = bytesInTSW + (double) hdr->size();
  now = Scheduler::instance().clock();
  policy->avgRate = newBytes / (now - policy->arrivalTime + policy->winLen);
  policy->arrivalTime = now;
  
}

/*-----------------------------------------------------------------------------
int TSW2CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Pre: policy points to a policytableEntry that is using the TSW2CM policer and
  whose state variables (avgRate and cir) are up to date.
Post: If policy's avgRate exceeds its CIR, this method returns an out-of-profile
  code point with a probability of ((rate - CIR) / rate).  If it does not
  downgrade the code point, this method simply returns the initial code point.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
-----------------------------------------------------------------------------*/
int TSW2CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) {
  if ((policy->avgRate > policy->cir)
      && (Random::uniform(0.0, 1.0) <= (1-(policy->cir/policy->avgRate)))) {
    return(downgradeOne(TSW2CM, initialCodePt));
  }
  else {
    return(initialCodePt);
  }
}

/*------------------------------------------------------------------------------
void TSW2CMPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
------------------------------------------------------------------------------*/
void TSW2CMPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): TSW2CM policer, ",
	   policyTable[i].sourceNode,policyTable[i].destNode);
    printf("initial code  point %d, CIR %.1f bps.\n",
	   policyTable[i].codePt, policyTable[i].cir * 8);
  }
  printf("\n");
}


/*-----------------------------------------------------------------------------
void TSW2CMPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
-----------------------------------------------------------------------------*/
void TSW2CMPolicy::printPolicerTable() {
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("TSW2CM\n");
    printf("policer code point %d is policed to code point %d.\n",
	   policerTable[i].initialCodePt,
	   policerTable[i].downgrade1);
  }
}
// The end of TSW2CM

// The Beginning of TSW3CM
/*-----------------------------------------------------------------------------
void  TSW3CMPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

TSW3CM        InitialCodePoint  CIR  PIR

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.
-----------------------------------------------------------------------------*/
void TSW3CMPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "TSW3CM") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);

    policyTable[policyTableSize].policer = TSW3CM;
    policyTable[policyTableSize].cir =
      policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
    policyTable[policyTableSize].pir = (double) atof(argv[7]) / 8.0;
    policyTable[policyTableSize].meter = tswTagger;
    
    policyTableSize++;
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void TSW3CMPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.  
A policer type should be specified as TSW3CM, followed by an initial code point.
Next should be an out-of-profile code point for policers with two-rate markers;
or a yellow and a red code point for policers with three drop precedences.
-----------------------------------------------------------------------------*/
void TSW3CMPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "TSW3CM") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = TSW3CM;

    policerTableSize++;
  }
}

/*------------------------------------------------------------------------------
void TSW3CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
------------------------------------------------------------------------------*/
void TSW3CMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
	double now, bytesInTSW, newBytes;
	hdr_cmn* hdr = hdr_cmn::access(pkt);

	bytesInTSW = policy->avgRate * policy->winLen;
	newBytes = bytesInTSW + (double) hdr->size();
	now = Scheduler::instance().clock();
	policy->avgRate = newBytes / (now - policy->arrivalTime + policy->winLen);
	policy->arrivalTime = now;
	
}

/*-----------------------------------------------------------------------------
int TSW3CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt)
Pre: policy points to a policytableEntry that is using the TSW3CM policer and
  whose state variables (avgRate, cir, and pir) are up to date.
Post: Sets code points with the following probabilities when rate > PIR:

          red:    (rate - PIR) / rate
          yellow: (PIR - CIR) / rate
          green:  CIR / rate

and with the following code points when CIR < rate <= PIR:

          red:    0
          yellow: (rate - CIR) / rate
          green:  CIR / rate

    When rate is under CIR, a packet is always marked green.
Returns: A code point to apply to the current packet.
Uses: Methods downgradeOne() and downgradeTwo().
-----------------------------------------------------------------------------*/
int TSW3CMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) {
  double rand = policy->avgRate * (1.0 - Random::uniform(0.0, 1.0));
  
  if (rand > policy->pir)
    return (downgradeTwo(TSW3CM, initialCodePt));
  else if (rand > policy->cir)
    return(downgradeOne(TSW3CM, initialCodePt));
  else
    return(initialCodePt);
}
 

/*-----------------------------------------------------------------------------
void TSW3CMPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void TSW3CMPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): TSW3CM policer, initial code ",
	   policyTable[i].sourceNode, policyTable[i].destNode);
    printf("point %d, CIR %.1f bps, PIR %.1f bytes.\n",
	   policyTable[i].codePt, policyTable[i].cir * 8,
	   policyTable[i].pir * 8);
  };
  printf("\n");
}


/*-----------------------------------------------------------------------------
void TSW3CMPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
-----------------------------------------------------------------------------*/
void TSW3CMPolicy::printPolicerTable() {
  bool threeColor;
  
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("TSW3CM ");
    printf("policer code point %d is policed to yellow ",
	   policerTable[i].initialCodePt);
    printf("code point %d and red code point %d.\n",
	   policerTable[i].downgrade1,
	   policerTable[i].downgrade2);
    printf("\n");
  };
}

// End of TSW3CM

// Begin of Token Bucket.
/*-----------------------------------------------------------------------------
void TBPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

TokenBucket   InitialCodePoint  CIR  CBS

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.
-----------------------------------------------------------------------------*/
void TBPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "TokenBucket") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);

    policyTable[policyTableSize].policer = tokenBucketPolicer;
    policyTable[policyTableSize].cir =
      policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
    policyTable[policyTableSize].cbs =
      policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
    policyTable[policyTableSize].meter = tokenBucketMeter;
    policyTableSize++;  
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void TBPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.  
A policer type should be specified as TokenBucket, followed by an initial code 
point.  Next should be an out-of-profile code point for policers with two-rate 
markers; or a yellow and a red code point for policers with three drop 
precedences.
------------------------------------------------------------------------------*/
void TBPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "TokenBucket") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = tokenBucketPolicer;
    
    policerTableSize++;
  } 
}

/*-----------------------------------------------------------------------------
void TBPolicy::applyTokenBucketMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables cBucket, cir, cbs, and arrivalTime hold valid values.
Post: Increments policy's Token Bucket state variable cBucket according to the
  elapsed time since the last packet arrival.  cBucket is filled at a rate equal  to CIR, capped at an upper bound of CBS.
  This method also sets arrivalTime equal to the current simulator time.
-----------------------------------------------------------------------------*/
void TBPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now = Scheduler::instance().clock();
  double tokenBytes;
  
  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
    policy->cBucket += tokenBytes;
  else
    policy->cBucket = policy->cbs;
  policy->arrivalTime = now;
}

/*----------------------------------------------------------------------------
int TBPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt,
        Packet* pkt)
Pre: policy points to a policytableEntry that is using the Token Bucket policer
and whose state variable (cBucket) is up to date.  pkt points to a
newly-arrived packet.
Post: If policy's cBucket is at least as large as pkt's size, cBucket is
decremented by that size and the initial code point is retained.  Otherwise,
the code point is downgraded.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
-----------------------------------------------------------------------------*/
int TBPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  double size = (double) hdr->size();
  
  if ((policy->cBucket - size) >= 0) {
    policy->cBucket -= size;
    return(initialCodePt);
  } else{
    return(downgradeOne(tokenBucketPolicer, initialCodePt));
  }
}

/*-----------------------------------------------------------------------------
void printPolicyTable()
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void TBPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): Token Bucket policer, ",
	   policyTable[i].sourceNode,policyTable[i].destNode);
    printf("initial code  point %d, CIR %.1f bps, CBS %.1f bytes.\n",
	   policyTable[i].codePt, policyTable[i].cir * 8,
	   policyTable[i].cbs);
  };
  printf("\n");
}


/*------------------------------------------------------------------------------
void TBPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
------------------------------------------------------------------------------*/
void TBPolicy::printPolicerTable() {
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("Token Bucket ");
    printf("policer code point %d is policed to code point %d.\n",
	   policerTable[i].initialCodePt,
	   policerTable[i].downgrade1);
  }
  printf("\n");
}

// End of Tocken Bucket.

// Begining of SRTCM

/*-----------------------------------------------------------------------------
void SRTCMPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

SRTCM         InitialCodePoint  CIR  CBS  EBS

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.
-----------------------------------------------------------------------------*/
void SRTCMPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "srTCM") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);
    
    policyTable[policyTableSize].policer = srTCMPolicer;
    policyTable[policyTableSize].cir =
      policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
    policyTable[policyTableSize].cbs =
      policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
    policyTable[policyTableSize].ebs =
      policyTable[policyTableSize].eBucket = (double) atof(argv[8]);
    policyTable[policyTableSize].meter = srTCMMeter;
    
    policyTableSize++;
  } else 
      printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void SRTCMPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv. 
A policer type should be specified as srTCM, followed by an initial code point.
Next should be an out-of-profile code point for policers with two-rate markers;
or a yellow and a red code point for policers with three drop precedences.
-----------------------------------------------------------------------------*/
void SRTCMPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "srTCM") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = srTCMPolicer;
    
    policerTableSize++;
  }
}

/*-----------------------------------------------------------------------------
void SRTCMPolicy::applyMeter(policyTableEntry *policy)
Pre: policy's variables cBucket, eBucket, cir, cbs, ebs, and arrivalTime hold
  valid values.
Post: Increments policy's srTCM state variables cBucket and eBucket according
  to the elapsed time since the last packet arrival.  cBucket is filled at a
  rate equal to CIR, capped at an upper bound of CBS.  When cBucket is full
  (equal to CBS), eBucket is filled at a rate equal to CIR, capped at an upper
  bound of EBS.
      This method also sets arrivalTime equal to the current
  simulator time.
Note: See the Internet Draft, "A Single Rate Three Color Marker" (Heinanen et
  al; May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
void SRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now = Scheduler::instance().clock();
  double tokenBytes;
  
  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
    policy->cBucket += tokenBytes;
  else {
    tokenBytes = tokenBytes - (policy->cbs - policy->cBucket);
    
    policy->cBucket = policy->cbs;
    if (policy->eBucket + tokenBytes <= policy->ebs)
      policy->eBucket += tokenBytes;
    else
      policy->eBucket = policy->ebs;
  }
  policy->arrivalTime = now;
}

/*-----------------------------------------------------------------------------
int SRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
Pre: policy points to a policyTableEntry that is using the srTCM policer and
  whose state variables (cBucket and eBucket) are up to date.  pkt points to a
  newly-arrived packet.
Post: If policy's cBucket is at least as large as pkt's size, cBucket is
  decremented by that size and the initial code point is retained.  Otherwise,
  if eBucket is at least as large as the packet, eBucket is decremented and the
  yellow code point is returned.  Otherwise, the red code point is returned.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne() and downgradeTwo().
Note: See the Internet Draft, "A Single Rate Three Color Marker" (Heinanen et
  al; May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
int SRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {

  hdr_cmn* hdr = hdr_cmn::access(pkt);
  double size = (double) hdr->size();
  
  if ((policy->cBucket - size) >= 0) {
    policy->cBucket -= size;
    return(initialCodePt);
  } else {
    if ((policy->eBucket - size) >= 0) {
      policy->eBucket -= size;
      return(downgradeOne(srTCMPolicer, initialCodePt));
    } else
      return(downgradeTwo(srTCMPolicer, initialCodePt));
  }
}


/*-----------------------------------------------------------------------------
void SRTCMPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void SRTCMPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): srTCM policer, initial code ",
	   policyTable[i].sourceNode, policyTable[i].destNode);
    printf("point %d, CIR %.1f bps, CBS %.1f bytes, EBS %.1f bytes.\n",
	   policyTable[i].codePt, policyTable[i].cir * 8,
	   policyTable[i].cbs, policyTable[i].ebs);
  }
  printf("\n");
}


/*------------------------------------------------------------------------------
void SRTCMPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
------------------------------------------------------------------------------*/
void SRTCMPolicy::printPolicerTable() {
  bool threeColor;
  
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("srTCM ");
    printf("policer code point %d is policed to yellow ",
	   policerTable[i].initialCodePt);
    printf("code point %d and red code point %d.\n",
	   policerTable[i].downgrade1,
	   policerTable[i].downgrade2);
  }
  printf("\n");
}

// End of SRTCM

// Beginning of TRTCM
/*-----------------------------------------------------------------------------
void TRTCMPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

trTCM         InitialCodePoint  CIR  CBS  PIR  PBS

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.
-----------------------------------------------------------------------------*/
void TRTCMPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "trTCM") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);
    
    policyTable[policyTableSize].policer = trTCMPolicer;
    policyTable[policyTableSize].cir =
      policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
    policyTable[policyTableSize].cbs =
      policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
    policyTable[policyTableSize].pir = (double) atof(argv[8]) / 8.0;
    policyTable[policyTableSize].pbs =
      policyTable[policyTableSize].pBucket = (double) atof(argv[9]);
    policyTable[policyTableSize].meter = trTCMMeter;
  
    policyTableSize++;  
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*-----------------------------------------------------------------------------
void TRTCMPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.
A policer type should be specified as trTCM, followed by an initial code point.
Next should be an out-of-profile code point for policers with two-rate markers;
or a yellow and a red code point for policers with three drop precedences.
-----------------------------------------------------------------------------*/
void TRTCMPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "trTCM") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = trTCMPolicer;
    
    policerTableSize++;
  }
}

/*----------------------------------------------------------------------------
void TRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables cBucket, pBucket, cir, pir, cbs, pbs, and arrivalTime
  hold valid values.
Post: Increments policy's trTCM state variables cBucket and pBucket according
  to the elapsed time since the last packet arrival.  cBucket is filled at a
  rate equal to CIR, capped at an upper bound of CBS.  pBucket is filled at a
  rate equal to PIR, capped at an upper bound of PBS.
      This method also sets arrivalTime equal to the current simulator time.
Note: See the Internet Draft, "A Two Rate Three Color Marker" (Heinanen et al;
  May, 1999) for a description of the srTCM.
---------------------------------------------------------------------------*/
void TRTCMPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  double now = Scheduler::instance().clock();
  double tokenBytes;
  tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
  if (policy->cBucket + tokenBytes <= policy->cbs)
    policy->cBucket += tokenBytes;
  else
    policy->cBucket = policy->cbs;
  
  tokenBytes = (double) policy->pir * (now - policy->arrivalTime);
  if (policy->pBucket + tokenBytes <= policy->pbs)
    policy->pBucket += tokenBytes;
  else
    policy->pBucket = policy->pbs;
  
  policy->arrivalTime = now;
}

/*----------------------------------------------------------------------------
int TRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
Pre: policy points to a policyTableEntry that is using the trTCM policer and
  whose state variables (cBucket and pBucket) are up to date.  pkt points to a
  newly-arrived packet.
Post: If policy's pBucket is smaller than pkt's size, the red code point is
  retained.  Otherwise, if cBucket is smaller than the packet size, the yellow
  code point is returned and pBucket is decremented.  Otherwise, the packet
  remains green and both buckets are decremented.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne() and downgradeTwo().
Note: See the Internet Draft, "A Two Rate Three Color Marker" (Heinanen et al;
  May, 1999) for a description of the srTCM.
-----------------------------------------------------------------------------*/
int TRTCMPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  double size = (double) hdr->size();
  
  if ((policy->pBucket - size) < 0)
    return(downgradeTwo(trTCMPolicer, initialCodePt));
  else {
    if ((policy->cBucket - size) < 0) {
      policy->pBucket -= size;
      return(downgradeOne(trTCMPolicer, initialCodePt));
    } else {
      policy->cBucket -= size;
      policy->pBucket -= size;
      return(initialCodePt);
    }
  }
}


/*-----------------------------------------------------------------------------
void TRTCMPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void TRTCMPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): trTCM policer, initial code ",
	   policyTable[i].destNode, policyTable[i].sourceNode);
    printf("point %d, CIR %.1f bps, CBS %.1f bytes, PIR %.1f bps, ",
	   policyTable[i].codePt, policyTable[i].cir * 8,
	   policyTable[i].cbs, policyTable[i].pir * 8);
    printf("PBS %.1f bytes.\n", policyTable[i].pbs);
  }
  printf("\n");
}


/*-----------------------------------------------------------------------------
void TRTCMPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
-----------------------------------------------------------------------------*/
void TRTCMPolicy::printPolicerTable() {
  bool threeColor;
  
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("trTCM ");
    printf("policer code point %d is policed to yellow ",
	   policerTable[i].initialCodePt);
    printf("code point %d and red code point %d.\n",
	   policerTable[i].downgrade1,
	   policerTable[i].downgrade2);
  }
  printf("\n");
}
// End of TRTCM

// Beginning of FW
//Constructor.
FWPolicy::FWPolicy() : Policy() {
  printf("hello\n!");
}

/*-----------------------------------------------------------------------------
void FWPolicy::addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:
FW         InitialCodePoint  CIR
-----------------------------------------------------------------------------*/
void FWPolicy::addPolicyEntry(int argc, const char*const* argv) {
  if (strcmp(argv[4], "FW") == 0) {
    // Specify the common parameters.
    Policy::addPolicyEntry(argc, argv);

    policyTable[policyTableSize].policer = FW;
    // Use cir as the transmission size threshold for the moment.
    policyTable[policyTableSize].cir = atoi(argv[6]);
    policyTable[policyTableSize].meter = fwTagger;

    policyTableSize++;
  } else 
    printf("ERROR: wrong policy name, check your simulation script.\n");
}

/*------------------------------------------------------------------------------
void FWPolicy::addPolicerEntry(int argc, const char*const* argv)
Adds an entry to policerTable according to the arguments in argv.
A policer type should be specified as FW, followed by an initial code point.
------------------------------------------------------------------------------*/
void FWPolicy::addPolicerEntry(int argc, const char*const* argv) {
  if (strcmp(argv[2], "FW") == 0) {
    // Specify the common parameters.
    Policy::addPolicerEntry(argc, argv);
    policerTable[policerTableSize].policer = FW;
    
    policerTableSize++;
  }
}

/*-----------------------------------------------------------------------------
void FWPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
void FWPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  int entry;
  
  double now = Scheduler::instance().clock();
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  hdr_ip* iph = hdr_ip::access(pkt);
  
  //  printf("enter applyMeter\n");
  // The flow states are kept in a hash table.
  entry = iph->flowid() % MAX_FLOW;
  
  //  printf("entry: %d, time_stamp: %f, fid: %d, byteSent: %d\n", entry,  fw_counter[entry].time_stamp, fw_counter[entry].fid, fw_counter[entry].bytesSent);

  // Test if this is the first flow.
  //  if (fw_counter[entry].time_stamp == 0) {
  //   printf ("FIRST set flow %d\n", iph->flowid());
  // fw_counter[entry].bytesSent = 0;
  // fw_counter[entry].fid = iph->flowid();
  //}
  // Test if the entry has kept the states for dead flow.
  /*
  if (fw_counter[entry].time_stamp + FLOW_TIME_OUT < now) {
    printf ("TIMEOUT flow %d change to flow %d\n", fw_counter[entry].fid, iph->flowid());
    fw_counter[entry].bytesSent = 0;
    fw_counter[entry].fid = iph->flowid();
  }

  fw_counter[entry].time_stamp = now;
  fw_counter[entry].bytesSent += hdr->size();
  */
  //printf("leave applyMeter\n");
}

/*-----------------------------------------------------------------------------
void FWPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) 
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
int FWPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) {
  int entry;
  hdr_ip* iph = hdr_ip::access(pkt);
  
  //  printf("enter applyPolicer\n");
  // The flow states are kept in a hash table.
  entry = iph->flowid() % MAX_FLOW;
  /*
  if (fw_counter[entry].fid != iph->flowid())
    printf ("MISS: flow %d in table, looking for flow %d\n", fw_counter[entry].fid, iph->flowid());
  */
  // Apply policy
  //if (fw_counter[entry].bytesSent > policy->cir) {
  //printf("leave applyPolicer, downgrade\n");
  // return(downgradeOne(FW, initialCodePt));
  //}
  // else {
    //    printf("leave applyPolicer, initial\n");
  return(initialCodePt);
    //}
}

/*------------------------------------------------------------------------------
void FWPolicy::printPolicyTable()
    Prints the policyTable, one entry per line.
------------------------------------------------------------------------------*/
void FWPolicy::printPolicyTable() {
  printf("Policy Table(%d):\n",policyTableSize);
  for (int i = 0; i < policyTableSize; i++) {
    printf("Flow (%d to %d): FW policer, ",
	   policyTable[i].sourceNode,policyTable[i].destNode);
      printf("initial code point %d, TH %d bytes.\n",
	     policyTable[i].codePt, policyTable[i].cir);
  }
  printf("\n");
}


/*-----------------------------------------------------------------------------
void FWPolicy::printPolicerTable()
    Prints the policerTable, one entry per line.
-----------------------------------------------------------------------------*/
void FWPolicy::printPolicerTable() {
  bool threeColor;
  
  printf("Policer Table:\n");
  for (int i = 0; i < policerTableSize; i++) {
    printf("FW ");
    printf("policer code point %d is policed to code point %d.\n",
	   policerTable[i].initialCodePt,
	   policerTable[i].downgrade1);
  }
  printf("\n");
}
// End of FW

