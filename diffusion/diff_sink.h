/*****************************************************************/
/*  diff_sink.h : Header file for sink agent                     */
/*  By     : Chalermek Intanagonwiwat (USC/ISI)                  */
/*  Last Modified : 8/21/99                                      */
/*****************************************************************/


#ifndef ns_diff_sink_h
#define ns_diff_sink_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "ip.h"
#include "diff_header.h"
#include "diffusion.h"


class SinkAgent;

// Timer for packet rate

class Sink_Timer : public TimerHandler {
 public:
	Sink_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// For periodic report of avg and var pkt received.

class Report_Timer : public TimerHandler {
 public:
	Report_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// Timer for periodic interest

class Periodic_Timer : public TimerHandler {
 public:
	Periodic_Timer(SinkAgent *a) : TimerHandler() { a_ = a; }
 protected:
	virtual void expire(Event *e);
	SinkAgent *a_;
};


// Class SinkAgent as source and sink for directed diffusion

class SinkAgent : public Agent {

 public:
  SinkAgent();
  int command(int argc, const char*const* argv);
  virtual void timeout(int);

  void report();
  void recv(Packet*, Handler*);
  void reset();
  void set_addr(ns_addr_t);
  int get_pk_count();
  void incr_pk_count();
  Packet *create_packet();

 protected:
  bool APP_DUP_;

  bool periodic_;
  bool always_max_rate_;
  int pk_count;
  int data_type_;
  int num_recv;
  int num_send;
  int RecvPerSec;

  double cum_delay;
  double last_arrival_time;

  Data_Hash_Table DataTable;

  void Terminate();
  void bcast_interest();
  void data_ready();
  void start();
  void stop();
  virtual void sendpkt();

  int running_;
  int random_;
  int maxpkts_;
  double interval_;

  int simple_report_rate;
  int data_counter;
 
  Sink_Timer sink_timer_;
  Periodic_Timer periodic_timer_;
  Report_Timer report_timer_;

  friend class Periodic_Timer;
};


#endif











