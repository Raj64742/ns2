// Copyrights go here

#ifndef ns_xcpq_h
#define ns_xcpq_h

#include "red.h"
#include "packet.h"
#include "xcp-end-sys.h"

#define  PACKET_SIZE_BYTES  1000
#define  PACKET_SIZE_KB     1
#define  MULTI_FAC	  1000
#define  INITIAL_Te_VALUE   0.05       // Was 0.3 Be conservative when 
				       // we don't kow the RTT

#define TRACE  1		       // when 0, we don't trace or write 
				       // var to disk

class XCPWrapQ;
class XCPQueue;

class XCPTimer : public TimerHandler { 
 public:
	XCPTimer(XCPQueue *a, void (XCPQueue::*call_back)() ) 
		: a_(a), call_back_(call_back) {};
 protected:
	virtual void expire (Event *e);
	XCPQueue *a_; 
	void (XCPQueue::*call_back_)();
}; 


class XCPQueue : public REDQueue {
	friend class XCPTimer;
 public:
	XCPQueue();
	void Tq_timeout ();			// timeout every propagation delay 
	void Te_timeout ();			// timeout every avg. rtt
	void setupTimers();			// setup timers for xcp queue only
	void routerId(XCPWrapQ* queue, int i);
	int routerId(int id = -1); 
	int limit(int len = 0);
	void setBW(double bw);
	void setChannel(Tcl_Channel queue_trace_file);
	void config();
	
	// Overloaded functions
	void enque(Packet* pkt);
	Packet* deque();
	
protected:
	double max(double d1, double d2) { return (d1 > d2) ? d1 : d2; }
	double min(double d1, double d2) { return (d1 < d2) ? d1 : d2; }
	double abs(double d) { return (d < 0) ? -d : d; }
	
	virtual void trace_var(char * var_name, double var);
	virtual void drop(Packet* pkt);
	
	// Estimation & Control Helpers
	void init_vars();
	virtual void do_on_packet_arrival(Packet* pkt); 
				// called in enque, but packet may be
				// dropped; used for updating the
				// estimation helping vars such as
				// counting the offered_load_,
				// sum_rtt_by_cwnd_

	virtual void do_before_packet_departure(Packet* p); 
				// called in deque, before packet
				// leaves used for writing the
				// feedback in the packet
  
	int 			routerId_;
	XCPWrapQ *		myQueue_;   //pointer to (top) wrapper queue
	XCPTimer *		queue_timer_;
	XCPTimer *		estimation_control_timer_;
	double			link_capacity_bps_;
	
	static const double	ALPHA_		= 0.4;
	static const double	BETA_		= 0.226;
	static const double	GAMMA_		= 0.1;
	static const double	XCP_MAX_INTERVAL= 1.0;
	static const double	XCP_MIN_INTERVAL= .001;
	
	double			Te_;       // Control Interval
	double			Tq_;    
	double			avg_rtt_;
	double			Cp_;
	double			Cn_;
	double			residue_pos_fbk_;
	double			residue_neg_fbk_;
	double			queue_bytes_;   // our estimate of the fluid model queue
	double			input_traffic_bytes_;       // traffic in Te 
	double			sum_rtt_by_throughput_;
	double			sum_inv_throughput_;
	double			running_min_queue_bytes_;
	unsigned int		num_cc_packets_in_Te_;
	int			drops_;

	Tcl_Channel queue_trace_file_;
};


#endif //ns_xcpq_h
