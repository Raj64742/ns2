#include "ip.h"
#include "random.h"

#include "ew.h"

// Beginning of EW
//Constructor.  
EW::EW() {
  ew_src = ew_dst = -1;
  // default value: no EW coupled, no detection
  cew = NULL;
  detect_p = detect_b = 0;
  debug_p = debug_b = 0;
  last_debug_t = 0;

  // random sampling timer setup:
  // EW sleeps initially for EW_SAMPLE_INTERVAL 
  timer = EW_SAMPLE_INTERVAL;
  sleep = 1;

  init_s_inv = s_inv = EW_SAMPLE_INTERVAL;
  d_inv = EW_DETECT_INTERVAL;

  adjustor = 1.0;

  // Initialize ALIST
  alist.head = alist.tail = NULL;
  alist.count = 0;

  swin.head = swin.tail = NULL;
  swin.count = swin.ravg = 0;

  hlf.alpha = 0.875;
  hlf.high = hlf.low = 0;

  avg_rate = cur_rate = 0;
  alarm = alarm_count = 0;
  p_count = db_pc = avg_req = cur_req = 0;
  
  drop_p = 0;
}

//Deconstructor.
EW::~EW(){
  cew = NULL;

  resetAList();
  resetSWin();
}

// Initialize the EW parameters
void EW::init(int ew_adj, int ew_inv, int qsrc, int qdst) {
  // EW adjustor (g = resp rate / request rate)
  adjustor = ew_adj;

  // Detection and sample interval
  init_s_inv = s_inv = ew_inv;
  d_inv = (int) (ew_inv / 8);
  if (d_inv < EW_MIN_DETECT_INTERVAL)
    d_inv = EW_MIN_DETECT_INTERVAL;

  //printf("s_inv: %d, d_inv: %d\n", s_inv, d_inv);
  // EW id
  ew_src = qsrc;
  ew_dst = qdst;
  //printf("EW[%d:%d] \n", ew_src, ew_dst);
}

// Enter EW processing
void EW::runEW(Packet *pkt) {
  // test if the detector is on (when there is an EW coupled.)
  if (!detect_b && !detect_p)
    return;

  // can't count ACKs:
  //   with resp: may cause inaccurate calculation with TSW(??)
  //   with req:  may cause resp retransmission.
  hdr_cmn *th = hdr_cmn::access(pkt);
  if (th->ptype() == PT_ACK)
    return;

  // get the time
  now = Scheduler::instance().clock();
  
  //printf("EW[%d:%d] run ", ew_src, ew_dst);

  if (detect_p)
    p_count++;

  //printf(" before UA");
  // Conduct detection continously
  if (detect_b)
    updateAList(pkt, now);
  //printf(" after UA");

  // Debug information
  if (debug_p) {
    db_pc++;
    if (now - last_debug_t > debug_p) {
      //printf("debugP ");
      tracePr();
      last_debug_t = (int)now;
    }
  }
  //printf(" DP ");
  if (debug_b && now - last_debug_t > debug_b) {
    //printf("debugB ");
    traceBr();
    last_debug_t = (int)now;
  }
  //printf(" DB ");

  // There is a timeout!
  if (now >= timer) {
    // Start detection
    //printf("Detection timeout(%d)\n", (int)now);
    
    // 1. Update the current rate (req/resp) from measurement
    // update the request rate
    if (detect_p) {
      //printf("%d", s_inv);
      cur_req = (int)(p_count / s_inv);
    }
    //printf("after UUP ");
    // update the aggregated response rate
    if (detect_b) {
      //printAList();
      // Record current aggregated response rate
      cur_rate = computeARR();
    }
    //printf("after UUB ");
    // 2. Detect change and Trigger alarm if necessary
    // Compare the current rate with the long term average for
    //   both req and resp
    if (detect_p) {
      detectChangeP();
    }
    //printf("after DP ");
    if (detect_b) {
      detectChangeB();
    }
    //printf("after DB ");
    // 3. Update the long term averages
    // update the request rate
    if (detect_p) {
      avg_req = (int)(hlf.alpha * avg_req + (1 - hlf.alpha) * cur_req);
    }
    //printf("after UP ");
    // update the aggregated response rate
    if (detect_b) {
      // Update flip-flop filter
      updateHLF(cur_rate);
      
      // Update SWIN, not used any more.
      //updateSWin(cur_rate);
      //ravgSWin();
      //printSWin();
      
      // Update the long term average of aggregated response rate
      updateAvgRate();
    }
    //printf("after DB ");
    // setup the sleeping timer
    timer = (int)now + s_inv;
    //printf("%d\n", s_inv);
  }
}

// Test if the alarm has been triggered.
float EW::testAlarm(Packet *pkt) {
  hdr_cmn *th = hdr_cmn::access(pkt);
  hdr_ip *iph = hdr_ip::access(pkt);
  int src_id = iph->saddr();
  int dst_id = iph->daddr();

  //return 0;
  // Can't touch acks for response.
  if (th->ptype() == PT_ACK) {
    //printf("an ack\n");
    return 0;
  }

  if (!cew)
    return 0;
  
  float drop_p;
  drop_p = cew->testAlarmCouple(src_id, dst_id);
  if (drop_p > 0 && alarm)
    return(drop_p);

  return 0;
}

// Test if the corrsponding alarm on reversed link has been triggered.
float EW::testAlarmCouple(int c_id, int s_id){
  // No alarm, no reaction
  if (!alarm) {
    if (drop_p == 0) {
      //printf(" N/A\n");	
      return 0;
    } else {
      //printf(" N/AD\n");	
      return (drop_p);
    }
  }

  // Alarm, clear the alarm count
  alarm_count = 0;    
  // Protect existing connection
  // Responses are from Server to Client
  if (searchAList(s_id, c_id)) {
    //printf(" A/E\n");	
    return(0);
  }

  // Reaction!
  //printf(" A/NE\n");	
  return(drop_p);
}

// Conduct the measurement
void EW::updateAList(Packet *pkt, double now) {
  hdr_cmn* hdr = hdr_cmn::access(pkt);
  hdr_ip* iph = hdr_ip::access(pkt);
  int dst_id = iph->daddr();
  int src_id = iph->saddr();
  int fid = iph->flowid(); 

  // Get the corresponding id.
  //printf("EW[%d:%d] in detector\n", ew_src, ew_dst);

  AListEntry *p;
  p = searchAList(src_id, dst_id);

  // Add new entry to AList
  // keep the bytes sent by each aggregate in AList
  if (!p) {
    p = newAListEntry(src_id, dst_id, fid);
  }

  // update the existing (or just created) entry in AList
  assert(p && p->src_id == src_id && p->dst_id == dst_id);

  // update the flow's arrival rate using TSW
  double bytesInTSW, newBytes;
  bytesInTSW = p->avg_rate * p->win_length;
  newBytes = bytesInTSW + (double) hdr->size();
  p->avg_rate = newBytes / (now - p->t_front + p->win_length);
  p->t_front = now;

  //printf("%f: %d %f\n", now, p->fid, p->avg_rate);
}

// Calculate the aggragated response rate for high-bandwidth flows
int EW::computeARR() {
  int i, agg_rate;
  struct AListEntry *max;
  int k;

  // Explicit garbage collection first 
  //  before both choosing HBFs and searching AList
  //printf("before timeout ");
  timeoutAList();
  //printf("after timeout ");

  // Pick the 10% highest bandwidth flows
  k = (int) (alist.count * 0.1 + 1);

  // Calculate the ARR for HBA
  i = 0;
  agg_rate = 0;

  do {
    max = maxAList();
    //printAListEntry(max, i);
    
    if (max) {
      agg_rate += (int)max->avg_rate;
      i++;
    }
  } while (i < k && max);

  // Have to clean the chosen bit in AList
  resetAListC();

  // Just use simple average!
  if (i)
    agg_rate = (int) (agg_rate / i);
  else {
    printf("No MAX returned from ALIST!!!\n");
  }
  //printf("%f %d %d\n", now, agg_rate, k);
  
  return(agg_rate);
}

// Find the matched AList entry
struct AListEntry * EW::searchAList(int src_id, int dst_id) {
  AListEntry *p;

  // Explicit garbage collection first.
  //printf("before timeout ");
  timeoutAList();
  //printf("after timeout ");
  // Use src and dest pair rather than flow id:
  //   aggregate flows within the same request-response exchange
  // Use timeout to seperate different exchanges.
  p = alist.head;
  while (p && (p->src_id != src_id || p->dst_id != dst_id)) {
    p = p->next;
  }
  
  return(p);
}

// Add new entry to AList
struct AListEntry * EW::newAListEntry(int src_id, int dst_id, int fid) {
  AListEntry *p;

  p = new AListEntry;
  p->chosen = 0;
  p->src_id = src_id;
  p->dst_id = dst_id;
  p->fid = fid;
  p->last_update = now;
  p->avg_rate = 0;
  // Since we are doing random sampling, 
  // the t_front should set to the beginning of this period instead of 0.
  p->t_front = now;
  p->win_length = 1;
  p->next = NULL;
  
  // Add new entry to AList
  if (alist.tail)
    alist.tail->next = p;
  alist.tail = p;
  
  if (!alist.head)
    alist.head = p;
  
  alist.count++;

  return(p);
}

// Find the max value in AList
struct AListEntry * EW::maxAList() {
  struct AListEntry *p, *max;
  double interval;
  int i;

  //printAList();
  max = alist.head;
  i = 0;
  while (max && max->chosen) {
    max = max->next;
  }

  //printf("%d ", i++);
  p = max;
  while(p) {
    //printf("%d ", i++);
    if (!(p->chosen) && p->avg_rate > max->avg_rate)
      max = p;
    p = p->next;
  }

  // set the chosen flag
  // Clean the chosen flag is in timeoutAList
  if (max) 
    max->chosen = 1;

  return(max);
}

// Timeout AList entries
void EW::timeoutAList() {
  AListEntry *p, *q;
  
  // Expire the old entries in AList
  p = q = alist.head;
  while (p) {
    // Garbage collection
    if (p->last_update + EW_FLOW_TIME_OUT < now){
      // The coresponding flow is expired.      
      if (p == alist.head){
	if (p == alist.tail) {
	  alist.head = alist.tail = NULL;
	  free(p);
	  p = q = NULL;
	} else {
	  alist.head = p->next;
	  free(p);
	  p = q = alist.head;
	}
      } else {
	q->next = p->next;
	if (p == alist.tail)
	  alist.tail = q;
	free(p);
	p = q->next;
      }
      alist.count--;
    } else {
      q = p;
      p = q->next;
    }
  }
}

// Reset the chosen bit in AList
void EW::resetAListC() {
  struct AListEntry *p;

  p = alist.head;
  while (p) {
    // clean the chosen flag.
    if (p->chosen)
      p->chosen = 0;
    p = p->next;
  }
}

// Reset AList
void EW::resetAList() {
  struct AListEntry *ap, *aq;

  ap = aq = alist.head;
  while (ap) {
    aq = ap;
    ap = ap->next;
    free(aq);
  }
  
  ap = aq = NULL;
  alist.head = alist.tail = NULL;  
  alist.count = 0;
}

// update the long term average aggregated response rate
void EW::updateAvgRate() {
  // Update the long term average value with the output from different filters
  if (!alarm && alarm_count == 0) {
    // Use low-gain filter for fast response
    avg_rate = (int)(hlf.low);
  } else {
    // Use high-gain filter to keep the long term average stable
    avg_rate = (int)(hlf.high);
  }
}

// update high-low filter
// high(t) = alpha * high(t-1) + (1 - alpha) * o(t)
// low(t) = (1 - alpha) * low(t-1) + alpha * o(t)
void EW::updateHLF(int rate) {
  hlf.high = hlf.alpha * hlf.high + (1 - hlf.alpha) * rate;
  hlf.low = (1 - hlf.alpha) * hlf.low + hlf.alpha * rate;
  //  printf("%f Flip-flop filter [%.2f, %.2f]\n", now, hlf.high, hlf.low);
}

// Reset SWin
void EW::resetSWin() {
  struct SWinEntry *p, *q;

  p = q = swin.head;
  while (p) {
    q = p;
    p = p->next;
    free(q);
  }
  
  p = q = NULL;
  swin.head = swin.tail = NULL;  
  swin.count = swin.ravg = 0;
}

// update swin with the latest measurement of aggregated response rate
void EW::updateSWin(int rate) {
  struct SWinEntry *p, *new_entry;

  new_entry = new SWinEntry;
  new_entry->rate = rate;
  new_entry->weight = 1;
  new_entry->next = NULL;
  
  if (swin.tail)
    swin.tail->next = new_entry;
  swin.tail = new_entry;
  
  if (!swin.head)
    swin.head = new_entry;

  // Reset current rate.
  if (swin.count < EW_SWIN_SIZE) {
    swin.count++;
  } else {
    p = swin.head;
    swin.head = p->next;
    free(p);
  }
}

// Calculate the running average over the sliding window
void EW::ravgSWin() {
  struct SWinEntry *p;
  float sum = 0;
  float t_weight = 0;

  //printf("Calculate running average over the sliding window:\n");
  p = swin.head;
  //printf("after p\n");

  while (p) {
    //printSWinEntry(p, i++);
    sum += p->rate * p->weight;
    t_weight += p->weight;
    p = p->next;
  }
  p = NULL;
  //printf("\n");  

  swin.ravg = (int)(sum / t_weight);

  //  printf("Ravg: %d\n", swin.ravg);
}

// Detect the change in packet arrival rate
void EW::detectChangeP() {
  if (!detect_p)
    return;

  if (cur_req > avg_req) {
    alarm = 1;
  }
}

// detect the traffic change by 
// comparing the current measurement with the long-term average
//   trigger alarm if necessary.
void EW::detectChangeB() {
  if (!detect_b)
    return;

  float p = 0;

  // When ALARM:
  //  detect if it is the time to release the alarm
  // When NO ALARM:
  //  detect if it is the time to trigger the alarm
  if (alarm) {
    // Determine if an alarm should be released
    if (cur_rate > avg_rate * (1 + EW_DETECT_RANGE)) {
      // reset alarm
      alarm = 0;
      
      if (alarm_count)
	alarm_count = 0;
      
      //incSInv();
    } 
  } else {
    // Determine if an alarm should be triggered
    if (cur_rate < avg_rate * (1 - EW_DETECT_RANGE)) {
      alarm_count++;
      if (alarm_count >= EW_A_TH) {
        alarm = 1;

        // Reset low and high gain filters' input values to the long-term avg
        // Actually, there is no change to high gain filter
        hlf.high = hlf.low = avg_rate;

	// Initial drop_p to the MAX value whenever alarm triggered
	if (drop_p < EW_MAX_DROP_P)
	  drop_p = EW_MAX_DROP_P;
      } else {
        //decSInv();
      }
    }
  }

  // Determine the dropping probability
  // Could be in reactor
  if (alarm) {
    // Compute the dropping probability as a linear function of current rate
    if (avg_rate)
      p = (avg_rate - cur_rate) * adjustor / avg_rate;
    else
      p = 1;
    // p could be negative here, but will be set to the MIN
    if (p < EW_MIN_DROP_P)
      p = EW_MIN_DROP_P;
    if (p > EW_MAX_DROP_P)
      p = EW_MAX_DROP_P;
    
    // Compute the actual drop probability
    drop_p = hlf.alpha * drop_p + (1 - hlf.alpha) * p;    
  } else {
    // Fade out the drop_p when no alarm
    if (drop_p > 0) {
      if (drop_p <= EW_MIN_DROP_P)
        drop_p = 0;
      else {
        drop_p = hlf.alpha * drop_p;
      }
    }
  }
}

// Decreas the sample interval
void EW::decSInv() {
  // Need some investigation for the min allowed detection interval
  if (s_inv / 2 > EW_MIN_SAMPLE_INTERVAL) {
    s_inv = s_inv / 2;
    
    //printf("SINV decreased by 2.\n");
  }
}

// Increase the sample interval
void EW::incSInv() {
  struct SWinEntry *p;
  
  if(s_inv * 2 <= init_s_inv) {
    s_inv = s_inv * 2;
    
    //printf("SINV increased by 2.\n");
  }
}

// Prints one entry in SWin
void EW::printSWin() {
  struct SWinEntry *p;
  printf("%f SWIN[%d, %d]", now, swin.ravg, swin.count);
  p = swin.head;
  int i = 0;
  while (p) {
    printSWinEntry(p, i++);
    p = p->next;
  }
  p = NULL;
  printf("\n");
}

// Print the contents in SWin
void EW::printSWinEntry(struct SWinEntry *p, int i) {
  if (p)
    printf("[%d: %d, %.2f] ", i, p->rate, p->weight);
}

// Print one entry in AList
void EW::printAListEntry(struct AListEntry *p, int i) {
  if (!p)
    return;

  printf("[%d] %d (%d>%d) %f %f %d\n", i, p->fid, p->src_id, p->dst_id, 
	 p->avg_rate, p->last_update, p->chosen);
}


// Print the entries in AList
void EW::printAList() {
  struct AListEntry *p;
  printf("%f AList(%d):\n", now, alist.count);

  p = alist.head;
  int i = 0;
  while (p) {
    printAListEntry(p, i);
    i++;
    p = p->next;
  }
  p = NULL;
  printf("\n");
}

// Trace packet incoming rate (req rate)
void EW::tracePr() {
  printf("P %d %d\n", (int)now, db_pc);
  db_pc = 0;
}

// Trace bit rate (resp rate)
void EW::traceBr() {
  int db_rate = 0;
  
  //printf("B ");
  db_rate = computeARR();
  printf("B %d %d\n", (int)now, db_rate);
}

// Setup the coupled EW
void EW::coupleEW(EW *ew) {
  if (!ew) {
    printf("ERROR!!! NOT A VALID EW POINTER!!!\n");
    exit (-1);
  } else {
    cew = ew;
    //    printf("Coupled!\n");
  }
}

// Enable the detector on packet incoming rate (req rate)
void EW::detectPr() {
  detect_p = 1;
}

// Enable the detector on bit rate (resp rate)
void EW::detectBr() {
  detect_b = 1;
}

// Enable packet incoming rate (req rate) debugging
void EW::debugPr(int debug_inv) {
  debug_p = debug_inv;
  //printf("%d\n", debug_p);
}

// Enable bit rate (resp rate) debugging
void EW::debugBr(int debug_inv) {
  debug_b = debug_inv;
  //printf("%d\n", debug_b);
}

// EW Policy: deal with queueing stuffs.
//Constructor.  
EWPolicy::EWPolicy() : Policy() {
  ew = new EW;
  drop_count = pass_count = 0;
  drop_total = pass_total = 0;
}

//Deconstructor.
EWPolicy::~EWPolicy(){
  if (ew)
    free(ew);
}

// Initialize the EW parameters
void EWPolicy::init(int ew_adj, int ew_inv, int qsrc, int qdst) {
  ew->init(ew_adj, ew_inv, qsrc, qdst);
}

/*-----------------------------------------------------------------------------
 void EWPolicy::applyMeter(policyTableEntry *policy, Packet *pkt)
 Flow states are kept in a linked list.
 Record how many bytes has been sent per flow and check if there is any flow
 timeout.
-----------------------------------------------------------------------------*/
void EWPolicy::applyMeter(policyTableEntry *policy, Packet *pkt) {
  // Invoke the change detector in EW directly. 
  //printf("enter applyMeter ");
  ew->runEW(pkt);
  //printf("leave applyMeter\n");
  return;
}

/*-----------------------------------------------------------------------------
void EWPolicy::applyPolicer(policyTableEntry *policy, int initialCodePt, Packet *pkt) 
    Prints the policyTable, one entry per line.
-----------------------------------------------------------------------------*/
int EWPolicy::applyPolicer(policyTableEntry *policy, policerTableEntry *policer, Packet *pkt) {
  //printf("enter applyPolicer ");
  float p = ew->testAlarm(pkt);
  if (p) {
    //printf("Alarm (%.2f), ", p);
    // Drop packets with probability of p
    if (Random::uniform(0.0, 1.0) > p) {
      pass_total++;
      //printf("initial (%d)!\n", pass_total);	
      return(policer->initialCodePt);
    } else {
      drop_total++;
      //printf("downgrade (%d)!\n", drop_total);	
      return(policer->downgrade1);
    }
  } else { 
    pass_total++;
    //printf("OK!\n");	
    return(policer->initialCodePt);
  }
}
// End of EWP
