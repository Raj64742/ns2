#ifndef EW_H
#define EW_H

#include "packet.h"
#include "dsPolicy.h"

#define EW_MAX_WIN 8
#define EW_SWIN_SIZE 4
#define EW_A_TH 1
// the max and min dropping probability
#define EW_DETECT_RANGE 0.1
#define EW_MIN_DROP_P 0.1
#define EW_MAX_DROP_P 0.9
#define EW_FLOW_TIME_OUT 10.0
#define EW_SAMPLE_INTERVAL 240
#define EW_DETECT_INTERVAL 60
#define EW_MIN_SAMPLE_INTERVAL 60
#define EW_MIN_DETECT_INTERVAL 15

// EW

// Record the detection result
struct AListEntry {
  int chosen;
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

// Data structure for High-low filter
// high(t) = alpha * high(t-1) + (1 - alpha) * o(t)
// low(t) = (1 - alpha) * low(t-1) + alpha * o(t)
struct HLF {
  double alpha;
  
  // the estimated value for the high-pass/low-pass filters
  double high, low;
};

class EW {
 public:
  EW();
  ~EW();

  // Initialize EW
  void init(int, int, int, int);

  // Process EW monitoring and detection
  void runEW(Packet *);

  // Test if the alarm has been triggered.
  float testAlarm(Packet *);
  // Test if the corrsponding alarm on reversed link has been triggered.
  float testAlarmCouple(int, int);

  // Setup the coupled EW
  void coupleEW(EW *);
  // Enable the detector on packet incoming rate (req rate)
  void detectPr();
  // Enable the detector on bit rate (resp rate)
  void detectBr();

  // Enable packet incoming rate (req rate) debugging
  void debugPr(int);
  // Enable bit rate (resp rate) debugging
  void debugBr(int);
  
  // output contents in SWin
  void printSWin();
  // Print one entry in SWin
  void printSWinEntry(struct SWinEntry *, int);

  // output contents in AList
  void printAList();
  // Print one entry in AList
  void printAListEntry(struct AListEntry *, int);

  // packet dropping probability
  float drop_p;

 private:
  // The nodes connected by EW
  int ew_src, ew_dst;

  // The coupled EW
  EW *cew;
  // EW can choose to detect packet arrival rate (Pr)
  //   or aggregated response rate (Br)
  //   or not to detect traffic change on one direction of the link
  int detect_p, detect_b;
  int debug_p, debug_b;
  int last_debug_t;

  // Current time for detection
  double now;
  
  // Current aggregated response rate
  int cur_rate;
  // Long term average aggregated response rate
  int avg_rate;

  // Random sampling timer
  int timer;
  // flag indicating if EW agent is sleeping or not.
  int sleep;

  // Sample interval
  int s_inv;
  // Initial interval
  int init_s_inv;  

  // Detection interval (may use if not continous detection)
  int d_inv;

  // Adjustor
  float adjustor;

  // Alarm flag
  int alarm, alarm_count;

  // List to keep the detection results
  struct AList alist;
  // The sliding window for running average on the aggregated response rate
  struct SWin swin;
  // High-low filter
  struct HLF hlf;

  // keep the packet number;
  int p_count;
  // for debug purpose only
  int db_pc;
  // Keep the current request rate (pkt/min)
  int cur_req;
  // the long term avergae of request rate
  int avg_req;
  
  // Measurement:
  // Conduct the measurement and update AList
  void updateAList(Packet *, double);
  // Find the max value in AList
  struct AListEntry *maxAList();
  // Timeout AList entries
  void timeoutAList();
  // Find the matched AList entry
  struct AListEntry * searchAList(int, int);
  // Add new entry to AList
  struct AListEntry * newAListEntry(int, int, int);
  // Calculate the aggragated response rate for high-bandwidth flows
  int computeARR(); 
  // Reset AList
  void resetAList();
  // Reset the chosen bit in AList
  void resetAListC();

  // update the long term average aggregated response rate
  void updateAvgRate();

  // update high-low filter
  void updateHLF(int);

  // update swin with the latest measurement for one HTab entry.
  void updateSWin(int);
  // compute the running average on the sliding window
  void ravgSWin();
  // Reset SWin
  void resetSWin();

  // Change detection:
  // detect the traffic change in aggregated response rate
  void detectChangeB();
  // Detect the change in packet arrival rate
  void detectChangeP();
  
  // Increase/decrease the sample interval to adjust the detection latency.
  void decSInv();
  void incSInv();

  // Trace packet incoming rate (req rate)
  void tracePr();
  // Trace bit rate (resp rate)
  void traceBr();
};

// Eearly warning policy: 
//  basically queueing stuffs
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
  int drop_count, pass_count;
  int drop_total, pass_total;
};
#endif
