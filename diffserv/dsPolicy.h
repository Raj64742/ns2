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
 * dsPolicy.h
 *
 */

#ifndef dsPolicy_h
#define dsPolicy_h
#include "dsred.h"


#define MAX_POLICIES 20		// Max. size of Policy Table.
enum policerType {TSW2CM, TSW3CM, dumbPolicer, tokenBucketPolicer, srTCMPolicer, trTCMPolicer};
enum meterType {dumbMeter, tswTagger, tokenBucketMeter, srTCMMeter, trTCMMeter};


/*------------------------------------------------------------------------------
struct policyTableEntry
------------------------------------------------------------------------------*/
struct policyTableEntry {
	nsaddr_t sourceNode, destNode;	// Source-destination pair
	policerType policer;
	meterType meter;
	int codePt;			// In-profile code point
	double cir;			// Committed information rate (bytes per s)			
	double cbs;			// Committed burst size (bytes)			
	double cBucket;			// Current size of committed bucket (bytes)			
	double ebs;			// Excess burst size (bytes)			
	double eBucket;			// Current size of excess bucket (bytes)			
	double pir;			// Peak information rate (bytes per s)			
	double pbs;			// Peak burst size (bytes)			
	double pBucket;			// Current size of peak bucket (bytes)			
	double arrivalTime;		// Arrival time of last packet in TSW metering
	double avgRate, winLen;		// Used for TSW metering
};
	

/*------------------------------------------------------------------------------
struct policerTableEntry
    This struct specifies the elements of a policer table entry.
------------------------------------------------------------------------------*/
struct policerTableEntry {
	policerType policer;
	int initialCodePt;
	int downgrade1;
	int downgrade2;
};


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
class Policy : public TclObject {
public:
	Policy();
	void addPolicyEntry(int argc, const char*const* argv);
	void addPolicerEntry(int argc, const char*const* argv);
	void updatePolicyRTT(int argc, const char*const* argv);
	double getCBucket(const char*const* argv);
	int mark(Packet *pkt);
	void printPolicyTable();		// prints the policy table
	void printPolicerTable();		// prints the policer table

protected:
	policyTableEntry policyTable[MAX_POLICIES];
	int policyTableSize;
	policerTableEntry policerTable[MAX_CP]; 	// policer table
	int policerTableSize;	// current number of entries in the policer table

	policyTableEntry* getPolicyTableEntry(nsaddr_t source, nsaddr_t dest);

	// Metering methods:
	void applyTSWTaggerMeter(policyTableEntry *policy, Packet *pkt);
	void applyTokenBucketMeter(policyTableEntry *policy);
	void applySrTCMMeter(policyTableEntry *policy);
	void applyTrTCMMeter(policyTableEntry *policy);

	// Map a code point to a new code point:
	int downgradeOne(policerType policer, int oldCodePt);
	int downgradeTwo(policerType policer, int oldCodePt);

	// Policing methods:
	int applyTSW2CMPolicer(policyTableEntry *policy, int initialCodePt);
	int applyTSW3CMPolicer(policyTableEntry *policy, int initialCodePt);
	int applyTokenBucketPolicer(policyTableEntry *policy, int initialCodePt,
      Packet *pkt);
	int applySrTCMPolicer(policyTableEntry *policy, int initialCodePt,
      Packet* pkt);
	int applyTrTCMPolicer(policyTableEntry *policy, int initialCodePt,
      Packet* pkt);

};

#endif
