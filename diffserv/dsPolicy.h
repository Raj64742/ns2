/*
 * Copyright (c) 2000 Nortel Networks
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Nortel Networks.
 * 4. The name of the Nortel Networks may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORTEL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NORTEL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Developed by: Farhan Shallwani, Jeremy Ethridge
 *               Peter Pieda, and Mandeep Baines
 * Maintainer: Peter Pieda <ppieda@nortelnetworks.com>
 */

#ifndef DS_POLICY_H
#define DS_POLICY_H
#include "dsred.h"

#define ANY_HOST -1		// Add to enable point to multipoint policy
#define FLOW_TIME_OUT 5.0      // The flow does not exist already.
#define MAX_POLICIES 20		// Max. size of Policy Table.

#define DUMB 0
#define TSW2CM 1
#define TSW3CM 2
#define TB 3
#define SRTCM 4
#define TRTCM 5
#define SFD 6
#define EWP 7

#define EW_MAX_WIN 8
#define EW_SWIN_SIZE 4
#define EW_A_TH 1
// cut 3/4 of the incoming requests
#define EW_DROP_RATIO 10
#define EW_FLOW_TIME_OUT 30.0
#define EW_SLEEP_INTERVAL 300
#define EW_DETECT_INTERVAL 60
#define EW_MIN_DETECT_INTERVAL 15

enum policerType {dumbPolicer, TSW2CMPolicer, TSW3CMPolicer, tokenBucketPolicer, srTCMPolicer, trTCMPolicer, SFDPolicer, EWPolicer};

enum meterType {dumbMeter, tswTagger, tokenBucketMeter, srTCMMeter, trTCMMeter, sfdTagger, ewTagger};

class Policy;
class TBPolicy;

//struct policyTableEntry
struct policyTableEntry {
  nsaddr_t sourceNode, destNode;	// Source-destination pair
  int policy_index;                     // Index to the policy table.
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
	

// This struct specifies the elements of a policer table entry.
struct policerTableEntry {
  policerType policer;
  int initialCodePt;
  int downgrade1;
  int downgrade2;
  int policy_index;
};

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
  
  // The table keeps pointers to the real policy
  // Added to support multiple policy per interface.
  Policy *policy_pool[MAX_POLICIES];

protected:
  // policy table and its pointer
  policyTableEntry policyTable[MAX_POLICIES];
  int policyTableSize;
  // policer table and its pointer
  policerTableEntry policerTable[MAX_CP];
  int policerTableSize;	

  policyTableEntry* getPolicyTableEntry(nsaddr_t source, nsaddr_t dest);
  policerTableEntry* getPolicerTableEntry(int policy_index, int oldCodePt);
};

// Below are actual policy classes.
// Supper class Policy can't do anything useful.
class Policy : public TclObject {
 public:
  Policy(){};

  // Metering and policing methods:
  // Have to initialize all the pointers before actually do anything with them!
  // If not, ok with gcc but not cc!!! Nov 29, xuanc
  virtual void applyMeter(policyTableEntry *policy, Packet *pkt) = 0;
  virtual int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt) = 0;
};

// DumbPolicy will do nothing, but is a good example to show how to add 
// new policy.
class DumbPolicy : public Policy {
 public:
  DumbPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TSW2CMPolicy : public Policy {
 public:
  TSW2CMPolicy() : Policy(){};

  // protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TSW3CMPolicy : public Policy {
 public:
  TSW3CMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TBPolicy : public Policy {
 public:
  TBPolicy() : Policy(){};

  // protected:
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class SRTCMPolicy : public Policy {
 public:
  SRTCMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

class TRTCMPolicy : public Policy {
 public:
  TRTCMPolicy() : Policy(){};

  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);
};

struct flow_entry {
  int fid;
  double last_update;
  int bytes_sent;
  int count;
  struct flow_entry *next;
};

struct flow_list {
  struct flow_entry *head;
  struct flow_entry *tail;
};

class SFDPolicy : public Policy {
public:
SFDPolicy();
~SFDPolicy();

// Metering and policing methods:
void applyMeter(policyTableEntry *policy, Packet *pkt);
int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

void printFlowTable();

 protected:
  // The table to keep the flow states.
  struct flow_list flow_table;
};

// EW

// Record the detection result
struct AListEntry {
  int src_id;
  int dst_id;
  int fid;
  double last_update;
  double avg_rate;
  double t_front;
  double win_length;
  struct AListEntry *next;
};

// List of detection result for aggregates
struct AList {
  struct AListEntry *head, *tail;
  int count;
};

// Record the request ratio
struct SWinEntry {
  float weight;
  int rate;
  struct SWinEntry *next;
};

// Data structure for sliding window
struct SWin {
  // counter of entries in the sliding window
  int count;
  // current running average
  int ravg;

  // List of SWin Entries
  struct SWinEntry *head;
  struct SWinEntry *tail;
};

// Data structure for Flip-flop filter
// high(t) = alpha * high(t-1) + (1 - alpha) * o(t)
// low(t) = (1 - alpha) * low(t-1) + alpha * o(t)
struct FF {
  double alpha;
  
  // the estimated value for the high-pass/low-pass filters
  double high, low;
};

// Data structure to keep the HOTTEST aggragates states
struct HTableEntry {
  //The node to be monitored
  int node_id;   
  // current measurement
  int cur_rate;

  // Sliding window:
  struct SWin swin;
  
  // last sample time
  int last_t; 
  // The alarm to trigger
  int alarm_count; 
  int alarm;    
};

class EW {
 public:
  EW();
  ~EW();

  // Initialize EW
  void init(int, int, int, int);

  // Process EW stuffs
  void runEW(Packet *);

  // Conduct the measurement and change detection
  void applyDetector(Packet *, double);
  // Test if the alarm has been triggered.
  int testAlarm(Packet *);
  // Test if the corrsponding alarm on reversed link has been triggered.
  int testAlarmCouple();

  // Setup the coupled EW
  void coupleEW(EW *);

  // output contents in SWin
  void printSWin();
  // Print one entry in SWin
  void printSWinEntry(struct SWinEntry *, int);

  // output contents in AList
  void printAList();
  // Print one entry in AList
  void printAListEntry(struct AListEntry *, int);

  int drop_ratio;
 private:
  // The nodes connected by EW
  int ew_src, ew_dst;

  // The coupled EW
  EW *cew;
  // EW can choose not to detect traffic change on one direction of the link
  int detector_on;

  // Current time for detection
  double now;
  // Current aggregated response rate
  int cur_rate;

  // Random sampling timer
  int timer;
  // Bit indicating if EW agent is sleeping or not.
  int sleep;

  // Detection Threshold
  int th;
  // initial threshold
  int init_th;        

  // Sample interval
  int inv;
  // Initial interval
  int init_inv;  

  // Detection interval
  int d_inv;
  // Sample interval
  int s_inv;

  int alarm, alarm_count;

  // List to keep the detection results
  struct AList alist;
  // The sliding window for running average on the aggregated response rate
  struct SWin swin;
  // Flip-flop filter
  struct FF ff;

  // Measurement:
  // Find the max value in AList
  struct AListEntry *maxAList();
  // Choose the high-bandwidth aggregates
  void choseHBA();
  // Reset AList
  void resetAList();

  // update flip-flop filter
  void updateFF(int);

  // update swin with the latest measurement for one HTab entry.
  void updateSWin(int);
  // compute the running average on the sliding window
  void ravgSWin();
  // Reset SWin
  void resetSWin();

  // Change detection:
  // detect the traffic change by 
  // comparing the running average calculated on the sliding window with 
  // a threshold and trigger alarm if necessary.
  void detectChange();

  // Increase/decrease SWin to adjust the detection latency.
  void decSWin();
  void incSWin();
};

class EWPolicy : public Policy {
 public:
  EWPolicy();
  ~EWPolicy();

  void init(int, int, int, int);
  
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

  //protected:
  EW *ew;
 private:
  int drop_count;
  int drop_total;
};
#endif
