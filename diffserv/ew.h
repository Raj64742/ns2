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
// ew.h (Early warning system)
//   by Xuan Chen (xuanc@isi.edu), USC/ISI

#ifndef EW_H
#define EW_H

#include "packet.h"
#include "dsPolicy.h"

#define EW_MAX_WIN 8
#define EW_SWIN_SIZE 4
// the max and min dropping probability
#define EW_DETECT_RANGE 0.1
#define EW_MIN_DROP_P 0.1
#define EW_MAX_DROP_P 0.9
#define EW_FLOW_TIME_OUT 10.0

#define EW_SAMPLE_INTERVAL 240
#define EW_DETECT_INTERVAL 60
#define EW_MIN_SAMPLE_INTERVAL 60
#define EW_MIN_DETECT_INTERVAL 15
#define EW_FADING_FACTOR 0.875

#define PKT_ARRIVAL 1001
#define PKT_DEPT 1002
#define PKT_DROP 1003

#define EW_DT_INV 240
#define EW_DB_INV EW_DT_INV

#define EW_UNCHANGE -1

// The default tocken-bucket size and tocken rate (in packet/byte)
#define DEFAULT_TB_SIZE 50
#define DEFAULT_TB_RATE_P 5
#define DEFAULT_TB_RATE_B 1000

// EW

// Record the number of packet arrived, departured, and dropped
struct PktRec {
  int arrival, dept, drop;
};

// Record the detection result
struct AListEntry {
  int src_id;
  int dst_id;
  int f_id;
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

// Data structure for High-low filter
// high(t) = alpha * high(t-1) + (1 - alpha) * o(t)
// low(t) = (1 - alpha) * low(t-1) + alpha * o(t)
class HLF {
 public:
  HLF();

  // configure the HLF
  void setAlpha(double);
  void reset(double value);
  void reset();

  // Get the output of HLF
  double getHigh();
  double getLow();

  // update high-low filter
  void update(double);

 private:
  double alpha;
  
  // the estimated value for the high-pass/low-pass filters
  double high, low;
};

// Definition for a tocken-bucket rate limitor
class TBrateLimitor {
 public:
  TBrateLimitor();
  TBrateLimitor(double);

  // set the rate
  void setRate(double);
  // adjust the rate
  void adjustRate();

  // parameters for a tocken bucket
  // the size of the token bucket (in bytes or packets)
  double bucket_size;             
  // number of tokens in the bucket (in bytes or packets)
  double tocken_num;
  // the rate to generate tockens
  double tocken_rate, ini_tocken_rate, last_tocken_rate;
  // last time when updating states
  double last_time;

  // in packet or byte mode (packet mode, by default)
  int pkt_mode;

  int run(double);
  int run(double, double); 

  // states to determine the direction of increasing/decreasing rate
  int n_score, p_score;

  // adjust the score for increasing or decreasing scores
  void resetScore();
  void adjustScore(int);
};

// Definition for the EW detector
class EWdetector {
 public:
  EWdetector();
  //virtual ~EWdetector() = 0;

  // Enable detecting and debugging rate (eg: pkt or bit rate)
  void setDb(int);
  void setDt(int);
  void setLink(int, int);

  // set and clean alarm
  void setAlarm();
  void resetAlarm();
  int testAlarm();

  void setChange();
  void resetChange();
  //private:
  // The nodes connected by EW
  int ew_src, ew_dst;

  // The current time
  double now;

  // The timer for detection and debugging
  int db_timer, dt_timer;
  // Detecting interval
  int dt_inv, db_inv;

  // Current rate and long term average
  double cur_rate, avg_rate;

  // The alarm flag and proposed dropping probability
  int alarm;

  int change;

  // High-low filter
  HLF hlf;

  // Detecting engine
  void run(Packet *);
  // Conduct measurement
  virtual void measure(Packet *) = 0;
  // Detect changes
  virtual void detect() = 0;
  // Update current measurement 
  virtual void updateCur() = 0;
  // Update long term average
  void updateAvg();

  // Trace function
  virtual void trace() = 0;

};

class EWdetectorP : public EWdetector {
 public:
  EWdetectorP();
  virtual ~EWdetectorP();

  // update packet stats
  void updateStats(int);
  // get the current request rate
  double getRate();

 private:
  // keep the packet rate stats
  PktRec cur_p, last_p, last_p_db;

  // Conduct measurement
  void measure(Packet *);
  // Detect changes
  void detect();
  // Update current measurement 
  void updateCur();
  // Update long term average
  void updateAvg();

  // Trace function
  void trace();
};

// EW
class EWdetectorB : public EWdetector {
 public:
  EWdetectorB();
  virtual ~EWdetectorB();

  // Initialize EW
  void init(int);

  // Check if the packet belongs to existing flow
  int exFlow(Packet *);

 private:
  // Adjustor
  float adjustor;
  float drop_p;
  // keep the count of flows for ARR
  int arr_count;

  // List to keep the detection results
  struct AList alist;
  // The sliding window for running average on the aggregated response rate
  struct SWin swin;
  
  // Conduct measurement
  void measure(Packet *);
  // Update current measurement 
  void updateCur();
  // Update long term average
  void updateAvg();
  // Change detection:
  // detect the traffic change in aggregated response rate
  void detect();
  
  // Measurement:
  // Conduct the measurement and update AList
  void updateAList(Packet *);
  // Find the max value in AList
  struct AListEntry *getMaxAList();
  // Sort AList based on the avg_rate
  void sortAList();
  // Timeout AList entries
  void timeoutAList();
  // Find the matched AList entry
  struct AListEntry * searchAList(int, int, int);
  // Add new entry to AList
  struct AListEntry * newAListEntry(int, int, int);
  // Get the median of AList
  int getMedianAList(int, int);
  // Get the rate given the index in the list
  int getRateAList(int);
  // Reset AList
  void resetAList();
  // Print one entry in AList
  void printAListEntry(struct AListEntry *, int);
  // output contents in AList
  void printAList();

  // Calculate the aggragated response rate for high-bandwidth flows
  int computeARR();

  // Determine the dropping probability based on current measurement
  void computeDropP();

  // Increase/decrease the sample interval to adjust the detection latency.
  void decSInv();
  void incSInv();

  // Trace bit rate (resp rate)
  void trace();

  // update swin with the latest measurement for one HTab entry.
  void updateSWin(int);
  // compute the running average on the sliding window
  void ravgSWin();
  // Reset SWin
  void resetSWin();
  // output contents in SWin
  void printSWin();
  // Print one entry in SWin
  void printSWinEntry(struct SWinEntry *, int);
};

// Eearly warning policy: 
//  basically queueing stuffs
class EWPolicy : public Policy {
 public:
  EWPolicy();
  ~EWPolicy();

  void init(int, int, int);
  
  // Metering and policing methods:
  void applyMeter(policyTableEntry *policy, Packet *pkt);
  int applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt);

  //  make packet drop decisions
  int dropPacket(Packet *);
  // detect if there is an alarm triggered
  void detect(Packet *pkt);
  
  // Enable detecting and debugging packet rate (eg: req rate)
  void detectPr();
  void detectPr(int);
  void detectPr(int, int);

  // Enable detecting and debugging bit rate (eg: resp rate)
  void detectBr();
  void detectBr(int);
  void detectBr(int, int);
 
  // Rate limitor: packet rate
  void limitPr();
  void limitPr(double);
  // Rate limitor: bit rate
  void limitBr();
  void limitBr(double);

  // couple EW detector
  void coupleEW(EWPolicy *);
  void coupleEW(EWPolicy *, double);

  //protected:
  EWdetectorB *ewB, *cewB;
  EWdetectorP *ewP, *cewP;

  TBrateLimitor *rlP, *rlB;
  
 private:
  // The current time
  double now;
  
  // indicator for detecting and debugging
  int ew_adj, qsrc, qdst;

  // rate limits
  int max_p, max_b;

  // ew alarm
  int alarm, pre_alarm;
  int change;
};
#endif
