
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
 * @(#) $Header: /home/smtatapudi/Thesis/nsnam/nsnam/ns-2/xcp/xcpq.cc,v 1.3 2004/09/28 18:12:44 haldar Exp $ (LBL)
 */

 /* Author : Dina Katabi, dina@ai.mit.edu
 * Created on 1/15/2002
 * Description: This is similar to Dina-pcp.cc, howover all quantities 
 * are measured in Kbytes rather than bytes. This is was done because 
 * the type "double" does not allow high capacities.
 *
 * Code Assumptions:  
 *
 * + packets size in a flow is constant
 * + capacity, BTA, BTF are measured in KB, times are measured in seconds
 * + cwnd and H_feedback are measured in packets (this inconsitency 
 * is not necessary but makes code easier since TCP code already measures 
 * cwnd in packets)
 * + The FC controller converges to a state in which all flows have same 
 * BW in packets/sec
 */


#include "xcpq.h"
#include "xcp.h"

static class XCPQClass : public TclClass {
public:
  XCPQClass() : TclClass("Queue/RED/XCPQ") {}
  TclObject* create(int, const char*const*) {
    return (new XCPQueue);
  }
} class_red_xcpq;


XCPQueue::XCPQueue(): rtt_timer_(NULL), queue_timer_(NULL), 
		      estimation_control_timer_(NULL), 
		      effective_rtt_(0.1)
{
  init_vars();
}

void XCPQueue::setupTimer()
{
  queue_timer_ = new XCPTimer(this, &XCPQueue::Tq_timeout);
  estimation_control_timer_ = new XCPTimer(this, &XCPQueue::Te_timeout);
  //sample_timer_ = new XCPTimer(this, &XCPQueue::Ts_timeout);
  rtt_timer_ = new XCPTimer(this, &XCPQueue::everyRTT);

  // Scheduling queue_timer randomly so routers are not synchronized
  double T;
  
  T = Random::normal(Tq_, 0.2*Tq_);
  if (T < 0.004) { T = 0.004; }
  //printf("Q%d: Queue Timer First TIMEOUT %g \n", routerId_, T);
  queue_timer_->sched(T);

  // Scheduling estimation_timer randomly so routers are not synchronized

  T = Random::normal(Te_, 0.2 * Te_);
  if (T < 0.004) { T = 0.004; }
  //printf("Q%d: Estimation Control Timer First TIMEOUT %g \n", routerId_, T);
  estimation_control_timer_->sched(T);
  
  // timer set to largest rtt seen in control interval
  // T = Random::normal(Te_, 0.2*Te_);
  //   if (T < 0.004) { T = 0.004; }
  //   //printf("Q%d: sample Timer First TIMEOUT %g \n", routerId_, T);
  //   sample_timer_->sched(T);
}

// change this to pass the variables for red parameter setting
void XCPQueue::config() {
  
  edp_.th_min = 0.4 * limit();                  // minthresh
  edp_.th_max = 0.8 * limit();                  // maxthresh
  edp_.q_w = 0.01 / limit();                    // for EWMA
  edp_.max_p_inv = 3;
  edp_.th_min_pkts = 0.6 * limit();
  edp_.th_max_pkts = 0.8 * limit();
  BTA_ = min(double(limit() * PACKET_SIZE_KB)/2, alpha_ * link_capacity_Kbytes_ * Te_);
}

void XCPQueue::routerId(XCPWrapQ* queue, int id)
{
  if (id < 0 && queue == 0)
    fprintf(stderr, "XCP:invalid routerId and queue\n");
  routerId_ = id;
  myQueue_ = queue;
}

int XCPQueue::routerId(int id)
{
  if (id > -1) routerId_ = id;
  return ((int)routerId_); 
}

int XCPQueue::limit(int qlim)
{
  if (qlim > 0) qlim_ = qlim;
  return (qlim_);
}

void XCPQueue::setBW(double bw)
{
  if (bw > 0) 
    link_capacity_Kbytes_ = bw;
}

void XCPQueue::setChannel(Tcl_Channel queue_trace_file)
{
  xqueue_trace_file_ = queue_trace_file;
}

Packet* XCPQueue::deque()
{
  int byteLen = byteLength()/MULTI_FAC;
  
  if (byteLen < running_min_queue_Kbytes_) 
    running_min_queue_Kbytes_= byteLen;
  
  Packet* p = REDQueue::deque();
  do_before_packet_departure(p);
  
  return (p);
}

void XCPQueue::enque(Packet* pkt)
{
  do_on_packet_arrival(pkt);
  return (REDQueue::enque(pkt));
}

void XCPQueue::do_on_packet_arrival(Packet* pkt){
  
  //trace_var("Qsize",curq_);
  //trace_var("Qavg", edv_.v_ave);
  //trace_var("prob",edv_.v_prob1);
    
  input_traffic_Kbytes_ +=  (double(hdr_cmn::access(pkt)->size())/MULTI_FAC);
  hdr_cctcp *cctcph = hdr_cctcp::access(pkt); 
  
  // Estimates depend only on Forward CC Traffic
  if (cctcph->ccenabled_ == 1) {
    
    num_cc_packets_in_Te_ += 1;
    
    if (cctcph->rtt_ != INITIAL_RTT_ESTIMATE){
      double inv_rate = (cctcph->rtt_/cctcph->cwnd_);
      sum_rtt_by_cwnd_ += inv_rate;
      sum_rtt_square_by_cwnd_ += (cctcph->rtt_*inv_rate);
    }
  } 
}


void XCPQueue::do_before_packet_departure(Packet* p)
{
  fill_in_feedback(p);
}


void XCPQueue::fill_in_feedback(Packet* p)
{
  int id;
  //char buf[25];
  double pos_feedback_Kbytes=0, neg_feedback_Kbytes=0, inv_rate=0, rtt=0, feedback_packets=0;

  if (p != 0) {
    hdr_cctcp *cctcph = hdr_cctcp::access(p);
    hdr_ip* iph = hdr_ip::access(p); 
    
    if (cctcph->ccenabled_ == 1) {
      inv_rate = cctcph->rtt_ / cctcph-> cwnd_;
      rtt      = cctcph->rtt_;

      pos_feedback_Kbytes = min(BTA_, xi_pos_ * inv_rate * rtt);   
      neg_feedback_Kbytes = min(BTF_, xi_neg_ * rtt);
      feedback_packets = (pos_feedback_Kbytes - neg_feedback_Kbytes)*MULTI_FAC /hdr_cmn::access(p)->size();
      
      if (cctcph->positive_feedback_ == -1 ){ // this field should be called feedback 
	// the first router to change the feedback info
	
	cctcph->positive_feedback_ = feedback_packets;
	cctcph->controlling_hop_ = routerId_;
	
	BTA_ = max(0, BTA_ - pos_feedback_Kbytes);
	BTF_ = max(0, BTF_ - neg_feedback_Kbytes);

      } else {
	if (cctcph->positive_feedback_ >= feedback_packets) {
	  // A Less congested upstream router
	
	  cctcph->positive_feedback_ = feedback_packets;
	  cctcph->controlling_hop_ = routerId_;
	  
	  BTA_ = max(0, BTA_ - pos_feedback_Kbytes);
	  BTF_ = max(0, BTF_ - neg_feedback_Kbytes);

	} else {
	  // a more congested upstream router has changed the congestion info 
	  // The code below might be changed to balance the spreading in all quatities
	  double feedback_Kbytes =  (cctcph->positive_feedback_ *(hdr_cmn::access(p)->size())/MULTI_FAC);
	  
	  if ( feedback_Kbytes > 0) { 
	    BTA_  -= min( feedback_Kbytes, BTA_);
	    BTF_  -= min(BTF_ , ((pos_feedback_Kbytes - neg_feedback_Kbytes) - feedback_Kbytes));
	    
	  } else {
	    BTF_  -= min(-1*feedback_Kbytes, BTF_);
	    if (feedback_packets > 0) 
	      BTF_  -= min((pos_feedback_Kbytes - neg_feedback_Kbytes), BTF_);
	  }
	}
      }
      if (TRACE && (xqueue_trace_file_ != 0 )){
	
	//printf("Q%d: %g, BTA %g, BTF %g, pos_fbk %g, neg_fbk %g, H_Feedback %g, cwnd %g, rtt %g, R %g flow %d\n", 
	//routerId_, now(), BTA_, BTF_, pos_feedback_Kbytes, neg_feedback_Kbytes, cctcph->positive_feedback_, cctcph->cwnd_, cctcph->rtt_, cctcph->cwnd_/cctcph->rtt_, iph->flowid()); 
	

	//trace_var("H_feedback", cctcph->positive_feedback_);
	//trace_var("pos_feedback", pos_feedback_Kbytes);
	//trace_var("neg_feedback", neg_feedback_Kbytes);
	
	id = iph->flowid();
	//sprintf(buf, "Thruput%d",id);
	//trace_var(buf, cctcph->cwnd_/cctcph->rtt_/link_capacity_Kbytes_);
	
	//sprintf(buf, "cwnd%d",id);
	//trace_var(buf, cctcph->cwnd_);
	
	// tracing thruput info
	if (id >= num_mice) 
	  thruput_elep_ += (double(hdr_cmn::access(p)->size())/MULTI_FAC);
	else 
	  thruput_mice_ += (double(hdr_cmn::access(p)->size())/MULTI_FAC);
	if (rtt > high_rtt_)
	  high_rtt_ = rtt;
	total_thruput_ += (double(hdr_cmn::access(p)->size())/MULTI_FAC);
      }
    }
  }
}
  

void XCPQueue::Tq_timeout()
{
  Queue_Kbytes_ = running_min_queue_Kbytes_;
  running_min_queue_Kbytes_ = byteLength()/MULTI_FAC;
  assert(link_capacity_Kbytes_ > 0);

  Tq_ = max(0.002,(avg_rtt_ - double(byteLength())/(MULTI_FAC*link_capacity_Kbytes_))/2); 
  queue_timer_->resched(Tq_);
 
  if (TRACE && (xqueue_trace_file_ != 0)){
    //trace_var("Tq_",Tq_);
    //trace_var("Queue_Kbytes_", Queue_Kbytes_);
  }
}

void XCPQueue::Te_timeout()
{
  double phi_Kbytes = 0.0;
  double shuffled_traffic_Kbytes = 0.0;
  assert(link_capacity_Kbytes_ > 0);
  
  if (TRACE && (xqueue_trace_file_ != 0)) {
    trace_var("BTA_not_allocated", BTA_);
    trace_var("BTF_not_reclaimed", BTF_);
  }

  if(sum_rtt_by_cwnd_ > 0){  // check we received packets
    
    avg_rtt_ = sum_rtt_square_by_cwnd_ / sum_rtt_by_cwnd_;
    
    phi_Kbytes = alpha_ * (link_capacity_Kbytes_- input_traffic_Kbytes_ /Te_) * avg_rtt_ - beta_ * Queue_Kbytes_;
    
    if( absolute(phi_Kbytes) > gamma_ * input_traffic_Kbytes_ ) {
      shuffled_traffic_Kbytes = 0.0;
    
    } else {
      shuffled_traffic_Kbytes = (gamma_ * input_traffic_Kbytes_) - absolute(phi_Kbytes);
    }

    BTA_ = max(0, phi_Kbytes) + shuffled_traffic_Kbytes;
    
    BTF_    =  max(0, -1*phi_Kbytes)+ shuffled_traffic_Kbytes;
    xi_pos_ =  1.1 * BTA_ / (sum_rtt_by_cwnd_ * avg_rtt_); // Multiplication by 1.1 is experimental
    xi_neg_ =  1.1 * BTF_ / (avg_rtt_ * double(num_cc_packets_in_Te_));  // Multiplication by 1.1 is experimental

  } 

  if (TRACE && (xqueue_trace_file_ != 0)) {
    trace_var("input_traffic_Kbytes_", input_traffic_Kbytes_/(Te_*link_capacity_Kbytes_));
    trace_var("avg_rtt_", avg_rtt_);
    trace_var("BTA_", BTA_);
    trace_var("BTF_", BTF_);
    
    //printf("Q%d: %g estimation_control_timer TIMEOUT Te_ = %g\n", routerId_, now(), Te_);
    //printf("Q%d: %g RTT %g\t N %g\t y %g\t Q %g\t phi %g\t H %g\t BTA %g\t BTF %g\t xi_p %g\t xi_n %g\n", 
    //routerId_, now(), avg_rtt_, sum_rtt_by_cwnd_/avg_rtt_, input_traffic_Kbytes_/Te_, Queue_Kbytes_, phi_Kbytes/avg_rtt_, shuffled_traffic_Kbytes/avg_rtt_, BTA_, BTF_, xi_pos_, xi_neg_);

  } 
  
  input_traffic_Kbytes_ = 0.0;
  num_cc_packets_in_Te_ = 0;
  sum_rtt_by_cwnd_ = 0.0;
  sum_rtt_square_by_cwnd_ = 0.0;
  Te_ = avg_rtt_;
  
  estimation_control_timer_->resched(Te_);
}

/*void XCPQueue::Ts_timeout() 
{
  
  Ts = high_rtt_;
    
  // calc. thruput for elephants and mice
  if (total_thruput_ > 0) {
    trace_var("total_thruput", total_thruput_/(Ts*link_capacity_Kbytes_));
    trace_var("thru_elep", thruput_elep_/(Ts*link_capacity_Kbytes_));
    trace_var("thru_mice", thruput_mice_/(Ts*link_capacity_Kbytes_));
  }
  thruput_elep_ = 0.0;
  thruput_mice_ = 0.0;
  total_thruput_ = 0.0;
  high_rtt_ = 0.004;
  
  sample_timer_->resched(Ts);
}*/

void XCPQueue::everyRTT ()
{
	
  trace_var("d", drops_);
  drops_=0;

  // Update sample the current queue size
  trace_var("q",length());
  
  // Update utilization 
  trace_var("u", total_thruput_/(effective_rtt_*link_capacity_Kbytes_));
  total_thruput_ = 0;
  
  rtt_timer_->resched(effective_rtt_);
}

void  XCPQueue::drop(Packet* p)
{
  drops_++;
  total_drops_ = total_drops_++;
  
  Connector::drop(p);
}

// Utility functions

double XCPQueue::now()
{
  return  Scheduler::instance().clock();
}


double XCPQueue::running_avg(double var_sample, double var_last_avg, double gain)
{
	double avg;
	if (gain < 0)
	  exit(3);	
	avg = gain * var_sample + ( 1 - gain) * var_last_avg;
	return avg;
}


double XCPQueue::max(double d1, double d2)
{
  if (d1 > d2)
    return d1; 
  else 
    return d2;
}


double XCPQueue::min(double d1, double d2)
{
  if (d1 < d2)
    return d1;
  else 
    return d2;
}


double XCPQueue::absolute(double d) 
{
  if (d >= 0) 
    return d; 
  else {
    d=-1*d; 

    if(d<0) {
      abort(); 
      return -1; // make gcc happy 
    } 
    else 
      return d;
  } 
}


// Estimation & Control Helpers

void XCPQueue::init_vars() 
{
  link_capacity_Kbytes_ = 0.0;
  alpha_                = 0.4;
  beta_                 = 0.226;
  gamma_                = 0.1;
  avg_rtt_              = INITIAL_Te_VALUE;
  
  Te_      = INITIAL_Te_VALUE;
  Tq_      = INITIAL_Te_VALUE; 
  xi_pos_  = 1.0;      // will be initialized after specifying the capacity
  xi_neg_  = 0;     
  BTA_     = double(limit() * PACKET_SIZE_KB)/2; //will be initialized after specifying the capacity
  BTF_     = 0;     
  Queue_Kbytes_ = 0.0; // our estimate of the fluid model queue
  
  input_traffic_Kbytes_    = 0.0; 
  sum_rtt_by_cwnd_        = 0.0; 
  sum_rtt_square_by_cwnd_ = 0.0;
  running_min_queue_Kbytes_= 0;
  num_cc_packets_in_Te_   = 0;

  // measuring drops
  drops_ = 0;
  total_drops_ = 0;
  
  // vars for calculating thruput
  high_rtt_ = 0.004;  // some arbitrary small rtt value
  thruput_elep_ = 0.0;
  thruput_mice_ = 0.0;
  total_thruput_ = 0.0;
  xqueue_trace_file_ = 0;
  myQueue_ = 0;
}


void XCPTimer::expire(Event *) { 
  (*a_.*call_back_)();
}


void XCPQueue::trace_var(char * var_name, double var)
{
  char wrk[500];
  double now = Scheduler::instance().clock();

  if (xqueue_trace_file_) {
    int n;
    sprintf(wrk, "%s %g %g",var_name, now, var);
    n = strlen(wrk);
    wrk[n] = '\n'; 
    wrk[n+1] = 0;
    (void)Tcl_Write(xqueue_trace_file_, wrk, n+1);
  }
  return; 
}


