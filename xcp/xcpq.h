// Copyrights go here

#ifndef NS_XCPQ_H
#define NS_XCPQ_H

#include <math.h>
#include <random.h>
#include "red.h"
#include "packet.h"
#include "xcp-end-sys.h"

#define  PACKET_SIZE_BYTES  1000
#define  PACKET_SIZE_KB     1
#define  MULTI_FAC          1000
#define  INITIAL_Te_VALUE   0.05       // Was 0.3 Be conservative when 
                                       // we don't kow the RTT

#define TRACE  1                       // when 0, we don't race or write 
                                       // var to disk

extern double INITIAL_RTT_ESTIMATE;    // if H_rtt is set to this value, 
                                       // the source has no estimate of its RTT
 
class XCPWrapQ;
class XCPQueue;

class XCPTimer : public TimerHandler { 
 public:
  XCPTimer(XCPQueue *a, void (XCPQueue::*call_back)() ) : a_(a), call_back_(call_back) {};
 protected:
  virtual void expire (Event *e);
  XCPQueue *a_; 
  void (XCPQueue::*call_back_)();
}; 


class XCPQueue : public REDQueue {
  friend class XCPTimer;
 public:
  XCPQueue();
  void Tq_timeout ();                     // timeout every propagation delay 
  void Te_timeout ();                     // timeout every avg. rtt
  void setupTimer();                  // setup timers for xcp queue only
  void routerId(XCPWrapQ* queue, int i);
  int routerId(int id = -1); 
  int limit(int len = 0);
  void setBW(double bw);
  void setChannel(Tcl_Channel queue_trace_file);
  void config();
  
  // Modified functions
  void enque(Packet* pkt);
  Packet* deque();

protected:

  // Utility Functions
  double now();
  double running_avg(double var_sample, double var_last_avg, double gain);
  double max(double d1, double d2);
  double min(double d1, double d2);
  double absolute(double d);

  virtual void trace_var(char * var_name, double var);
  virtual void drop(Packet* pkt);

  // Estimation & Control Helpers
  void init_vars();
  virtual void do_on_packet_arrival(Packet* pkt); // called in enque, but packet 
                                                  // may be dropped
                                                  // used for updating the 
                                                  // estimation helping vars 
                                                  // such as counting the 
                                                  // offered_load_, sum_rtt_by_cwnd_
  virtual void do_before_packet_departure(Packet* p); 
                                    // called in deque, before packet leaves
                                    // used for writing the feedback in the packet
  
  virtual void fill_in_feedback(Packet* p); 
                                    // called in do_before_packet_departure()
                                    // used for writing the feedback in the packet

  // ---- Variables --------
  unsigned int routerId_;
  XCPWrapQ* myQueue_;   //pointer to wrapper queue lying on top
  XCPTimer*        queue_timer_;
  XCPTimer*        estimation_control_timer_;
  double          link_capacity_Kbytes_;
  double          alpha_;   // constant
  double          beta_;    // constant 
  double          gamma_;   // constant

  //----- Estimates ---------
  double          Te_;       // Time over which we compute 
                             // estimator and control decisions
  double          Tq_;    
  double          avg_rtt_;
  //double          cum_avg_rtt_;   // for tracing control interval
  double          xi_pos_;
  double          xi_neg_;
  double          BTA_;
  double          BTF_;
  double          Queue_Kbytes_;   // our estimate of the fluid model queue

  //----- General Helping Vars -
  // An interval over which a fluid model queue is computed
  double          input_traffic_Kbytes_;       // traffic in Te 
  double          sum_rtt_by_cwnd_; 
  double          sum_rtt_square_by_cwnd_;
  double          running_min_queue_Kbytes_;
  unsigned int    num_cc_packets_in_Te_;
  int             drops_;

  // ----- For Tracing Vars --------------//
  Tcl_Channel queue_trace_file_;

};


#endif //NS_XCPQ_H
