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
 */

/*
 * dsPolicy.cc
 *
 */


#include "dsPolicy.h"
#include "packet.h"
#include "tcp.h"
#include "random.h"


/*------------------------------------------------------------------------------
Policy() Constructor.
------------------------------------------------------------------------------*/
Policy::Policy() {
	policyTableSize = 0;
	policerTableSize = 0;
}


/*------------------------------------------------------------------------------
void addPolicyEntry()
    Adds an entry to policyTable according to the arguments in argv.  A source
and destination node ID must be specified in argv, followed by a policy type
and policy-specific parameters.  Supported policies and their parameters
are:

TSW2CM        InitialCodePoint  CIR
TSW3CM        InitialCodePoint  CIR  PIR
TokenBucket   InitialCodePoint  CIR  CBS
srTCM         InitialCodePoint  CIR  CBS  EBS
trTCM         InitialCodePoint  CIR  CBS  PIR  PBS

    No error-checking is performed on the parameters.  CIR and PIR should be
specified in bits per second; CBS, EBS, and PBS should be specified in bytes.

    If the Policy Table is full, this method prints an error message.
------------------------------------------------------------------------------*/
void Policy::addPolicyEntry(int argc, const char*const* argv) {

	if (policyTableSize == MAX_POLICIES)
		printf("ERROR: Policy Table size limit exceeded.\n");
	else {
		policyTable[policyTableSize].sourceNode = atoi(argv[2]);
		policyTable[policyTableSize].destNode = atoi(argv[3]);
		policyTable[policyTableSize].codePt = atoi(argv[5]);
		policyTable[policyTableSize].arrivalTime = 0;
		policyTable[policyTableSize].winLen = 1.0;

		if (strcmp(argv[4], "TSW2CM") == 0) {
			policyTable[policyTableSize].policer = TSW2CM;
		   policyTable[policyTableSize].cir =
			   policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
			if (argc == 8) policyTable[policyTableSize].winLen = (double) atof(argv[7]);/* mb */
			policyTable[policyTableSize].meter = tswTagger;
		}
		else if (strcmp(argv[4], "TSW3CM") == 0) {
			policyTable[policyTableSize].policer = TSW3CM;
   		policyTable[policyTableSize].cir =
	   		policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
			policyTable[policyTableSize].pir = (double) atof(argv[7]) / 8.0;
			policyTable[policyTableSize].meter = tswTagger;
		}
		else if (strcmp(argv[4], "DumbPolicer") == 0) {
			policyTable[policyTableSize].policer = dumbPolicer;
			policyTable[policyTableSize].meter = dumbMeter;
		}
		else if (strcmp(argv[4], "TokenBucket") == 0) {
			policyTable[policyTableSize].policer = tokenBucketPolicer;
   		policyTable[policyTableSize].cir =
	   		policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
			policyTable[policyTableSize].cbs =
		     policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
			policyTable[policyTableSize].meter = tokenBucketMeter;
		}
		else if (strcmp(argv[4], "srTCM") == 0) {
			policyTable[policyTableSize].policer = srTCMPolicer;
   		policyTable[policyTableSize].cir =
	   		policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
			policyTable[policyTableSize].cbs =
			   policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
         policyTable[policyTableSize].ebs =
			   policyTable[policyTableSize].eBucket = (double) atof(argv[8]);
			policyTable[policyTableSize].meter = srTCMMeter;

		}
		else if (strcmp(argv[4], "trTCM") == 0) {
			policyTable[policyTableSize].policer = trTCMPolicer;
   		policyTable[policyTableSize].cir =
	   		policyTable[policyTableSize].avgRate = (double) atof(argv[6]) / 8.0;
			policyTable[policyTableSize].cbs =
			   policyTable[policyTableSize].cBucket = (double) atof(argv[7]);
			policyTable[policyTableSize].pir = (double) atof(argv[8]) / 8.0;
			policyTable[policyTableSize].pbs =
			   policyTable[policyTableSize].pBucket = (double) atof(argv[9]);
			policyTable[policyTableSize].meter = trTCMMeter;
		}
		policyTableSize++;
	}
}


/*-----------------------------------------------------------------------------
policyTableEntry* getPolicyTableEntry(long source, long dest)
Pre: policyTable holds exactly one entry for the specified source-dest pair.
Post: Finds the policyTable array that matches the specified source-dest pair.
Returns: On success, returns a pointer to the corresponding policyTableEntry;
  on failure, returns NULL.
------------------------------------------------------------------------------*/
policyTableEntry* Policy::getPolicyTableEntry(nsaddr_t source, nsaddr_t dest) {
	for (int i = 0; i <= policyTableSize; i++)
		if ((policyTable[i].sourceNode == source) && (policyTable[i].destNode == dest))
			return(&policyTable[i]);

	// !!! Could make a default code point for undefined flows:
	printf("ERROR: No Policy Table entry found for Source %d-Destination %d.\n", source, dest);
   printPolicyTable();
	return(NULL);
}


/*------------------------------------------------------------------------------
void updatePolicyRTT(int argc, const char*const* argv)
Pre: The command line specifies a source and destination node for which an
  RTT-Aware policy exists and a current RTT value for that policy.
Post: The aggRTT field of the appropriate policy is updated to a weighted
  average of the previous value and the new RTT value specified in the command
  line.  If no matching policy is found, an error message is printed.
------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------
void addPolicerEntry(int argc, const char*const* argv)
Pre: argv contains a valid command line for adding a policer entry.
Post: Adds an entry to policerTable according to the arguments in argv.  No
  error-checking is done on the arguments.  A policer type should be specified,
  consisting of one of the names {TSW2CM, TSW3CM, TokenBucket,
  srTCM, trTCM}, followed by an initial code point.  Next should be an
  out-of-profile code point for policers with two-rate markers; or a yellow and
  a red code point for policers with three drop precedences.
      If policerTable is full, an error message is printed.
------------------------------------------------------------------------------*/
void Policy::addPolicerEntry(int argc, const char*const* argv) {
	if (policerTableSize == MAX_CP)
		printf("ERROR: Policer Table size limit exceeded.\n");
	else {
		if (strcmp(argv[2], "TSW2CM") == 0)
			policerTable[policerTableSize].policer = TSW2CM;
		else if (strcmp(argv[2], "TSW3CM") == 0)
			policerTable[policerTableSize].policer = TSW3CM;
		else if (strcmp(argv[2], "TokenBucket") == 0)
			policerTable[policerTableSize].policer = tokenBucketPolicer;
		else if (strcmp(argv[2], "srTCM") == 0)
			policerTable[policerTableSize].policer = srTCMPolicer;
		else if (strcmp(argv[2], "trTCM") == 0)
			policerTable[policerTableSize].policer = trTCMPolicer;

		policerTable[policerTableSize].initialCodePt = atoi(argv[3]);
		policerTable[policerTableSize].downgrade1 = atoi(argv[4]);
		if (argc == 6)
			policerTable[policerTableSize].downgrade2 = atoi(argv[5]);
		policerTableSize++;
	}
}


/*------------------------------------------------------------------------------
double Policy::getCBucket(int argc, const char*const* argv) {
Pre: The command line specifies a source and destination node for which a
  policy exists that uses a cBucket value.  That policy's cBucket parameter is
  currently valid.
Post: The policy's cBucket value is found and returned.
Returns: The value cBucket on success; or -1 on an error.
------------------------------------------------------------------------------*/
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


/*------------------------------------------------------------------------------
void applyTSWTaggerMeter(policyTableEntry *policy, Packet *pkt)
Pre: policy's variables avgRate, arrivalTime, and winLen hold valid values; and
  pkt points to a newly-arrived packet.
Post: Adjusts policy's TSW state variables avgRate and arrivalTime (also called
  tFront) according to the specified packet.
Note: See the paper "Explicit Allocation of Best effor Delivery Service" (David
  Clark and Wenjia Fang), Section 3.3, for a description of the TSW Tagger.
------------------------------------------------------------------------------*/
void Policy::applyTSWTaggerMeter(policyTableEntry *policy, Packet *pkt) {
	double now, bytesInTSW, newBytes;
	hdr_cmn* hdr = hdr_cmn::access(pkt);

	bytesInTSW = policy->avgRate * policy->winLen;
	newBytes = bytesInTSW + (double) hdr->size();
	now = Scheduler::instance().clock();
	policy->avgRate = newBytes / (now - policy->arrivalTime + policy->winLen);
	policy->arrivalTime = now;
	
}


/*------------------------------------------------------------------------------
void applyTokenBucketMeter(policyTableEntry *policy)
Pre: policy's variables cBucket, cir, cbs, and arrivalTime hold valid values.
Post: Increments policy's Token Bucket state variable cBucket according to the
  elapsed time since the last packet arrival.  cBucket is filled at a rate equal
  to CIR, capped at an upper bound of CBS.
      This method also sets arrivalTime equal to the current simulator time.
------------------------------------------------------------------------------*/
void Policy::applyTokenBucketMeter(policyTableEntry *policy) {
	double now = Scheduler::instance().clock();
	double tokenBytes;
	
	tokenBytes = (double) policy->cir * (now - policy->arrivalTime);
	if (policy->cBucket + tokenBytes <= policy->cbs)
		policy->cBucket += tokenBytes;
	else
		policy->cBucket = policy->cbs;
	policy->arrivalTime = now;
}


/*------------------------------------------------------------------------------
void applySrTCMMeter(policyTableEntry *policy)
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
------------------------------------------------------------------------------*/
void Policy::applySrTCMMeter(policyTableEntry *policy) {
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


/*------------------------------------------------------------------------------
void applyTrTCMMeter(policyTableEntry *policy)
Pre: policy's variables cBucket, pBucket, cir, pir, cbs, pbs, and arrivalTime
  hold valid values.
Post: Increments policy's trTCM state variables cBucket and pBucket according
  to the elapsed time since the last packet arrival.  cBucket is filled at a
  rate equal to CIR, capped at an upper bound of CBS.  pBucket is filled at a
  rate equal to PIR, capped at an upper bound of PBS.
      This method also sets arrivalTime equal to the current simulator time.
Note: See the Internet Draft, "A Two Rate Three Color Marker" (Heinanen et al;
  May, 1999) for a description of the srTCM.
------------------------------------------------------------------------------*/
void Policy::applyTrTCMMeter(policyTableEntry *policy) {
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


/*------------------------------------------------------------------------------
int downgradeOne(policerType policer, int oldCodePt)
Pre: policer is a valid policer; and policerTable has an entry corresponding to
  policer and oldCodePoint.
Post: The policerTable is searched for a corresponding downgraded code point.
Returns: On success, this method returns the downgraded code point.  (This code
  point is the yellow code point for a three color marker.)  On failure, this
  method prints an error message and returns -1.
------------------------------------------------------------------------------*/
int Policy::downgradeOne(policerType policer, int oldCodePt) {

	for (int i = 0; i < policerTableSize; i++)
	   if ((policerTable[i].policer == policer) &&
               (policerTable[i].initialCodePt == oldCodePt))
			return(policerTable[i].downgrade1);
		
	printf("ERROR: No Policer Table entry found for initial code point %d.\n", oldCodePt);
	return(-1);
}


/*------------------------------------------------------------------------------
int downgradeTwo(policerType policer, int oldCodePt)
Pre: policer is a valid policer that uses at least three drop precedences.
  policerTable has an entry corresponding to policer and oldCodePoint.
Post: The policerTable is searched for a corresponding downgraded code point.
Returns: On success, this method returns the downgraded code point.  (This code
  point is the red code point for a three color marker.)  On failure, this
  method prints an error message and returns -1.
------------------------------------------------------------------------------*/
int Policy::downgradeTwo(policerType policer, int oldCodePt) {
	if ((policer == TSW2CM) || (policer == tokenBucketPolicer)) {
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

/*------------------------------------------------------------------------------
int applyTSW2CMPolicer(policyTableEntry *policy, int initialCodePt)
Pre: policy points to a policytableEntry that is using the TSW2CM policer and
  whose state variables (avgRate and cir) are up to date.
Post: If policy's avgRate exceeds its CIR, this method returns an out-of-profile
  code point with a probability of ((rate - CIR) / rate).  If it does not
  downgrade the code point, this method simply returns the initial code point.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
------------------------------------------------------------------------------*/
int Policy::applyTSW2CMPolicer(policyTableEntry *policy, int initialCodePt) {
	if ((policy->avgRate > policy->cir)
		&& (Random::uniform(0.0, 1.0) <= (1-(policy->cir/policy->avgRate))))
	{
		return(downgradeOne(TSW2CM, initialCodePt));
	}
	else
	{
		return(initialCodePt);
	}
}

#if 0
/*------------------------------------------------------------------------------
int applyTSW3CMPolicer(policyTableEntry *policy, int initialCodePt)
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
------------------------------------------------------------------------------*/
int Policy::applyTSW3CMPolicer(policyTableEntry *policy, int initialCodePt) {
	double rand = Random::uniform(0.0, 1.0);

	if (rand <= ((policy->avgRate - policy->pir) / (1.0 * policy->avgRate)))
		return (downgradeTwo(TSW3CM, initialCodePt));
	else if (rand <= ((policy->avgRate - policy->cir) / (1.0 * policy->avgRate)))
		return(downgradeOne(TSW3CM, initialCodePt));
	else
		return(initialCodePt);
}
#endif

/*------------------------------------------------------------------------------
int applyTSW3CMPolicer(policyTableEntry *policy, int initialCodePt)
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
------------------------------------------------------------------------------*/
int Policy::applyTSW3CMPolicer(policyTableEntry *policy, int initialCodePt) {
	double rand = policy->avgRate * (1.0 - Random::uniform(0.0, 1.0));

	if (rand > policy->pir)
		return (downgradeTwo(TSW3CM, initialCodePt));
	else if (rand > policy->cir)
		return(downgradeOne(TSW3CM, initialCodePt));
	else
		return(initialCodePt);
}

/*------------------------------------------------------------------------------
int applyTokenBucketPolicer(policyTableEntry *policy, int initialCodePt,
        Packet* pkt)
Pre: policy points to a policytableEntry that is using the Token Bucket policer
  and whose state variable (cBucket) is up to date.  pkt points to a
  newly-arrived packet.
Post: If policy's cBucket is at least as large as pkt's size, cBucket is
  decremented by that size and the initial code point is retained.  Otherwise,
  the code point is downgraded.
Returns: A code point to apply to the current packet.
Uses: Method downgradeOne().
------------------------------------------------------------------------------*/
int Policy::applyTokenBucketPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {
	hdr_cmn* hdr = hdr_cmn::access(pkt);
	double size = (double) hdr->size();

	if ((policy->cBucket - size) >= 0) {
		policy->cBucket -= size;
		return(initialCodePt);
	} else{
		return(downgradeOne(tokenBucketPolicer, initialCodePt));
	}
}

/*------------------------------------------------------------------------------
int applySrTCMPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
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
------------------------------------------------------------------------------*/
int Policy::applySrTCMPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {
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

/*------------------------------------------------------------------------------
int applyTrTCMPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt)
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
------------------------------------------------------------------------------*/
int Policy::applyTrTCMPolicer(policyTableEntry *policy, int initialCodePt, Packet* pkt) {
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

/*------------------------------------------------------------------------------
int mark(Packet *pkt, double minRTT)
Pre: The source-destination pair taken from pkt matches a valid entry in
  policyTable.
Post: pkt is marked with an appropriate code point.
------------------------------------------------------------------------------*/
int Policy::mark(Packet *pkt) {
	policyTableEntry *policy;
	int codePt;
	hdr_ip* iph;
   int fid;

   iph = hdr_ip::access(pkt);
   fid = iph->flowid();


	policy = getPolicyTableEntry(iph->saddr(), iph->daddr());
	codePt = policy->codePt;

	switch (policy->meter) {
		case dumbMeter: break;
		case tswTagger: applyTSWTaggerMeter(policy, pkt);  break;
		case tokenBucketMeter: applyTokenBucketMeter(policy);  break;
		case srTCMMeter: applySrTCMMeter(policy);  break;
		case trTCMMeter: applyTrTCMMeter(policy);  break;
		default: printf("ERROR: Unknown meter type in Policy Table.\n");
	}

	switch (policy->policer) {
	   case TSW2CM: codePt = applyTSW2CMPolicer(policy, codePt);  break;
	   case TSW3CM: codePt = applyTSW3CMPolicer(policy, codePt);  break;
      case dumbPolicer: break;
	   case tokenBucketPolicer: codePt = applyTokenBucketPolicer(policy, codePt, pkt);  break;
	   case srTCMPolicer: codePt = applySrTCMPolicer(policy, codePt, pkt);  break;
	   case trTCMPolicer: codePt = applyTrTCMPolicer(policy, codePt, pkt);  break;
	   default: printf("ERROR: Unknown policer type in Policy Table.\n");

	}

	iph->prio_ = codePt;
	return(codePt);
};


/*------------------------------------------------------------------------------
void printPolicyTable()
    Prints the policyTable, one entry per line.
------------------------------------------------------------------------------*/
void Policy::printPolicyTable() {
	printf("Policy Table(%d):\n",policyTableSize);
	for (int i = 0; i < policyTableSize; i++)
	{
		switch (policyTable[i].policer) {
			case TSW2CM:
			   printf("Flow (%d to %d): TSW2CM policer, ",
               policyTable[i].sourceNode,policyTable[i].destNode);
            printf("initial code  point %d, CIR %.1f bps.\n",
               policyTable[i].codePt, policyTable[i].cir * 8);
			   break;
			case TSW3CM:
			   printf("Flow (%d to %d): TSW3CM policer, initial code ",
               policyTable[i].sourceNode, policyTable[i].destNode);
				printf("point %d, CIR %.1f bps, PIR %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].pir * 8);
			   break;
			case tokenBucketPolicer:
			   printf("Flow (%d to %d): Token Bucket policer, ",
               policyTable[i].sourceNode,policyTable[i].destNode);
            printf("initial code  point %d, CIR %.1f bps, CBS %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs);
			   break;
			case srTCMPolicer:
			   printf("Flow (%d to %d): srTCM policer, initial code ",
               policyTable[i].sourceNode, policyTable[i].destNode);
            printf("point %d, CIR %.1f bps, CBS %.1f bytes, EBS %.1f bytes.\n",
               policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs, policyTable[i].ebs);
			   break;
			case trTCMPolicer:
			   printf("Flow (%d to %d): trTCM policer, initial code ",
               policyTable[i].destNode, policyTable[i].sourceNode);
            printf("point %d, CIR %.1f bps, CBS %.1f bytes, PIR %.1f bps, ",
					policyTable[i].codePt, policyTable[i].cir * 8,
               policyTable[i].cbs, policyTable[i].pir * 8);
            printf("PBS %.1f bytes.\n", policyTable[i].pbs);
			   break;
			default:
			   printf("ERROR: Unknown policer type in Policy Table.\n");
		}
	}
	printf("\n");
}


/*------------------------------------------------------------------------------
void printPolicerTable()
    Prints the policerTable, one entry per line.
------------------------------------------------------------------------------*/
void Policy::printPolicerTable() {
	bool threeColor;

	printf("Policer Table:\n");
	for (int i = 0; i < policerTableSize; i++) {
		threeColor = false;
		switch (policerTable[i].policer) {
			case TSW2CM:
				printf("TSW2CM ");
				break;
			case TSW3CM:
				printf("TSW3CM ");
				threeColor = true;
				break;
			case tokenBucketPolicer:
				printf("Token Bucket ");
				break;
			case srTCMPolicer:
				printf("srTCM ");
				threeColor = true;
				break;
			case trTCMPolicer:
				printf("trTCM ");
				threeColor = true;
				break;
			default:
				printf("ERROR: Unknown policer type in Policer Table.");
		}

		if (threeColor) {
			printf("policer code point %d is policed to yellow ",
            policerTable[i].initialCodePt);
         printf("code point %d and red code point %d.\n",
            policerTable[i].downgrade1,
            policerTable[i].downgrade2);
		} else
			printf("policer code point %d is policed to code point %d.\n",
									policerTable[i].initialCodePt,
                           policerTable[i].downgrade1);
	}
	printf("\n");
}
