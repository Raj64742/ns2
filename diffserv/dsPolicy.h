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

// dsPolicy.h 

#ifndef DS_POLICY_H
#define DS_POLICY_H
#include "dsred.h"

#define ANY_HOST -1		// Add to enable point to multipoint policy
#define MAX_FLOW 8            // MAX num of flows to keep state in hash table.
#define FLOW_TIME_OUT 5.0      // The flow does not exist already.

#define MAX_POLICIES 20		// Max. size of Policy Table.

enum policerType {dumbPolicer, TSW2CM, TSW3CM, tokenBucketPolicer, srTCMPolicer, trTCMPolicer, FW};
enum meterType {dumbMeter, tswTagger, tokenBucketMeter, srTCMMeter, trTCMMeter, fwTagger};

//struct policyTableEntry
struct policyTableEntry {
  nsaddr_t sourceNode, destNode;	// Source-destination pair
  policerType policer;
  meterType meter;
  int codePt;		     // In-profile code point
  double cir;		     // Committed information rate (bytes per s) 
  double cbs;		     // Committed burst size (bytes)			
  double cBucket;	     // Current size of committed bucket (bytes)     
  double ebs;		     // Excess burst size (bytes)			
  double eBucket;	     // Current size of excess bucket (bytes)	
  double pir;		     // Peak information rate (bytes per s)
  double pbs;		     // Peak burst size (bytes)			
  double pBucket;	     // Current size of peak bucket (bytes)	       
  double arrivalTime;	     // Arrival time of last packet in TSW metering
  double avgRate, winLen;    // Used for TSW metering
};
	

//    This struct specifies the elements of a policer table entry.
struct policerTableEntry {
	policerType policer;
	int initialCodePt;
	int downgrade1;
	int downgrade2;
};

class Policy;

// Class PolicyClassifier: keep the policy and polier tables.
class PolicyClassifier : public TclObject {
 public:
  PolicyClassifier();
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);
  void updatePolicyRTT(int argc, const char*const* argv);
  double getCBucket(const char*const* argv);
  int mark(Packet *pkt);

  // prints the policy tables
  void printPolicyTable();		
  void printPolicerTable();

 protected:
  Policy *policy;

  // policy table and its pointer
  //policyTableEntry policyTable[MAX_POLICIES];
  int policyTableSize;
  // policer table and its pointer
  //policerTableEntry policerTable[MAX_CP];  
  int policerTableSize;	

  //policyTableEntry* getPolicyTableEntry(nsaddr_t source, nsaddr_t dest);
};

// Below are actual policy classes.
class Policy : public TclObject {
 public:
  Policy();
  virtual void addPolicyEntry(int argc, const char*const* argv);
  virtual void addPolicerEntry(int argc, const char*const* argv);
  void updatePolicyRTT(int argc, const char*const* argv);
  double getCBucket(const char*const* argv);
  int mark(Packet *pkt);
  
  // prints the policy tables
  virtual void printPolicyTable() = 0;		
  virtual void printPolicerTable() = 0;

 protected:
  // policy table and its pointer
  policyTableEntry policyTable[MAX_POLICIES];
  int policyTableSize;
  // policer table and its pointer
  policerTableEntry policerTable[MAX_CP];  
  int policerTableSize;	

  policyTableEntry* getPolicyTableEntry(nsaddr_t source, nsaddr_t dest);
  
  // Metering and policing methods:
  virtual void applyMeter(policyTableEntry *policy, Packet *pkt) = 0;
  virtual int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) = 0;

  // Map a code point to a new code point:
  int downgradeOne(policerType policer, int oldCodePt);
  int downgradeTwo(policerType policer, int oldCodePt);
};

// DumbPolicy will do nothing, but is a good example to show how to add 
// new policy.
class DumbPolicy : public Policy {
 public:
  DumbPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

class TSW2CMPolicy : public Policy {
 public:
  TSW2CMPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

class TSW3CMPolicy : public Policy {
 public:
  TSW3CMPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

class TBPolicy : public Policy {
 public:
  TBPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

class SRTCMPolicy : public Policy {
 public:
  SRTCMPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

class TRTCMPolicy : public Policy {
 public:
  TRTCMPolicy() : Policy(){};
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();

 protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

struct flow_entry {
  int fid;
  double last_update;
  int bytes_sent;
  struct flow_entry *next;
};

struct flow_list {
  struct flow_entry *head;
  struct flow_entry *tail;
};

class FWPolicy : public Policy {
 public:
  FWPolicy();
  ~FWPolicy();
  void addPolicyEntry(int argc, const char*const* argv);
  void addPolicerEntry(int argc, const char*const* argv);

  void printPolicyTable();		
  void printPolicerTable();
  void printFlowTable();

 protected:
  // The table to keep the flow states.
  struct flow_list flow_table;

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt);
};

#endif
