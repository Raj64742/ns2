
/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcpq.h,v 1.3 2004/09/28 18:12:44 haldar Exp $ (LBL)
 */


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
  //void Ts_timeout();                      // for measuring thruput
  void everyRTT();
  void setupTimer();                  // setup timers for xcp queue only
  
  void routerId(XCPWrapQ* queue, int i);
  int routerId(int id = -1); 
  
  int limit(int len = 0);
  void setBW(double bw);
  void setChannel(Tcl_Channel queue_trace_file);
  //void setEffectiveRtt(double rtt) {effective_rtt_ = rtt;}
  double totalDrops() { return total_drops_; }
  
  void config();
  XCPTimer*        rtt_timer_;
  
  // Modified functions
  void enque(Packet* pkt);
  Packet* deque();
  virtual void drop(Packet* p);
  
  // tracing var
  int num_mice;

 protected:

  // Utility Functions
  double now();
  double running_avg(double var_sample, double var_last_avg, double gain);
  double max(double d1, double d2);
  double min(double d1, double d2);
  double absolute(double d);

  virtual void trace_var(char * var_name, double var);
  
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
  //XCPTimer*        sample_timer_;
  double          link_capacity_Kbytes_;
  double          alpha_;   // constant
  double          beta_;    // constant 
  double          gamma_;   // constant

  //----- Estimates ---------
  double          Te_;       // Time over which we compute 
                             // estimator and control decisions
  double          Tq_;    
  double          Ts;
  double          avg_rtt_;
  double          high_rtt_;   // highest rtt seen from flows
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

  double   thruput_elep_;
  double   thruput_mice_;
  double   total_thruput_;

  double effective_rtt_;
  
  // drops
  int drops_;
  double total_drops_ ;

  // ----- For Tracing Vars --------------//
  Tcl_Channel xqueue_trace_file_;

};


#endif //NS_XCPQ_H
