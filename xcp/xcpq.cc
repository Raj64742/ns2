/*
 * Copy rights go here.
 *
 * Based on Dina Katabi <dina@ai.mit.edu>, 1/15/2002
 */

#include "xcpq.h"
#include "xcp.h"
#include "random.h"

static class XCPQClass : public TclClass {
public:
	XCPQClass() : TclClass("Queue/RED/XCPQ") {}
	TclObject* create(int, const char*const*) {
		return (new XCPQueue);
	}
} class_red_xcpq;


XCPQueue::XCPQueue(): queue_timer_(NULL), 
		      estimation_control_timer_(NULL)
{
	init_vars();
}

void XCPQueue::setupTimers()
{
	queue_timer_ = new XCPTimer(this, &XCPQueue::Tq_timeout);
	estimation_control_timer_ = new XCPTimer(this, &XCPQueue::Te_timeout);
	
	// Scheduling timers randomly so routers are not synchronized
	double T;
	
	T = max(0.004, Random::normal(Tq_, 0.2*Tq_));
	queue_timer_->sched(T);
	
	T = max(0.004, Random::normal(Te_, 0.2 * Te_));
	estimation_control_timer_->sched(T);
}

// change this to pass the variables for red parameter setting
void XCPQueue::config() {
	// The following RED setup attempts to disable RED
	edp_.th_min = limit();		// minthresh
	edp_.th_max = limit();		// maxthresh
	edp_.q_w = 0.01 / limit();	// for EWMA
	edp_.max_p_inv = 3;
	edp_.th_min_pkts = limit();
	edp_.th_max_pkts = limit();
}

void XCPQueue::routerId(XCPWrapQ *q, int id)
{
	if (id < 0 || q == 0)
		fprintf(stderr, "XCP:invalid routerId or queue\n");
	routerId_ = id;
	myQueue_ = q;
}

int XCPQueue::routerId(int id)
{
	if (id > -1) 
		routerId_ = id;
	return routerId_; 
}

int XCPQueue::limit(int qlim)
{
	if (qlim > 0) 
		qlim_ = qlim;
	return qlim_;
}

void XCPQueue::setBW(double bw)
{
	if (bw > 0) 
		link_capacity_bps_ = bw;
}

void XCPQueue::setChannel(Tcl_Channel queue_trace_file)
{
	queue_trace_file_ = queue_trace_file;
}

Packet* XCPQueue::deque()
{
	double inst_queue = byteLength();
/* L 36 */
	if (inst_queue < running_min_queue_bytes_) 
		running_min_queue_bytes_= inst_queue;
	
	Packet* p = REDQueue::deque();
	do_before_packet_departure(p);
	
	return (p);
}


void XCPQueue::drop(Packet* p) 
{
	myQueue_->drop(p);
}

void XCPQueue::enque(Packet* pkt)
{
	do_on_packet_arrival(pkt);
	REDQueue::enque(pkt);
}

void XCPQueue::do_on_packet_arrival(Packet* pkt)
{
	double pkt_size = double(hdr_cmn::access(pkt)->size());

/* L 1 */
	input_traffic_bytes_ += pkt_size;

	hdr_xcp *xh = hdr_xcp::access(pkt); 
	if (spread_bytes_) {
		int i = int(xh->rtt_/BWIDTH + .5);
		if (i > maxb_)
			maxb_ = i;
		b_[i] += pkt_size;
		
		if (xh->rtt_ != 0.0 && xh->throughput_ != 0.0)
			t_[i] += pkt_size/xh->throughput_;
	}
	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;	// Estimates depend only on Forward XCP Traffic

	++num_cc_packets_in_Te_;

	if (xh->rtt_ != 0.0 && xh->throughput_ != 0.0) {
/* L 2 */
		sum_inv_throughput_ += pkt_size / xh->throughput_;
/* L 3 */
		if (xh->rtt_ < XCP_MAX_INTERVAL) {
/* L 4 */
			sum_rtt_by_throughput_ += (xh->rtt_ 
						   * pkt_size 
						   / xh->throughput_);
/* L 5 */
		} else {
/* L 6 */
			sum_rtt_by_throughput_ += (XCP_MAX_INTERVAL 
						   * pkt_size 
						   / xh->throughput_);	
		}
	}
}

void XCPQueue::do_before_packet_departure(Packet* p)
{
	if (!p) return;

	hdr_xcp *xh = hdr_xcp::access(p);
    
	if (xh->xcp_enabled_ != hdr_xcp::XCP_ENABLED)
		return;
	if (xh->rtt_ == 0.0) {
		xh->delta_throughput_ = 0;
		return;
	}
	if (xh->throughput_ == 0.0)
		xh->throughput_ = .1; // XXX 1bps is small enough

	double inv = 1.0/xh->throughput_;
	double pkt_size = double(hdr_cmn::access(p)->size());
	double frac = 1.0;
	if (spread_bytes_) {
		// rtt-scaling
		frac = (xh->rtt_ > Te_) ? Te_ / xh->rtt_ : xh->rtt_ / Te_;
	}
/* L 19 */
	double pos_fbk = min(residue_pos_fbk_, Cp_ * inv * pkt_size);
	pos_fbk *= frac;
/* L 20 */
	double neg_fbk = min(residue_neg_fbk_, Cn_ * pkt_size);
	neg_fbk *= frac;
/* L 21 */
	double feedback = pos_fbk - neg_fbk;

/* L 22 */	
	if (xh->delta_throughput_ >= feedback) {
/* L 23 */
		xh->delta_throughput_ = feedback;
		xh->controlling_hop_ = routerId_;
/* L 26 */
	} else {
/* L 27-33 corrected */
		neg_fbk = min(residue_neg_fbk_, neg_fbk + (feedback - xh->delta_throughput_));
		pos_fbk = xh->delta_throughput_ + neg_fbk;
	}
/* L 24-25 */
	residue_pos_fbk_ = max(0, residue_pos_fbk_ - pos_fbk);
	residue_neg_fbk_ = max(0, residue_neg_fbk_ - neg_fbk);
/* L 34 */
	if (residue_pos_fbk_ == 0.0)
		Cp_ = 0.0;
/* L 35 */
	if (residue_neg_fbk_ == 0.0)
		Cn_ = 0.0;
		
	if (TRACE && (queue_trace_file_ != 0 )) {
		trace_var("pos_fbk", pos_fbk);
		trace_var("neg_fbk", neg_fbk);
		trace_var("delta_throughput", xh->delta_throughput_);
		int id = hdr_ip::access(p)->flowid();
		char buf[25];
		sprintf(buf, "Thruput%d", id);
		trace_var(buf, xh->throughput_);
	}
}


void XCPQueue::Tq_timeout()
{
	double inst_queue = byteLength();
/* L 37 */
	queue_bytes_ = running_min_queue_bytes_;
/* L 37.5 corrected: min_queue = inst_queue */
	running_min_queue_bytes_ = inst_queue;
/* L 38 */
	Tq_ = max(0.002, (avg_rtt_ - inst_queue/link_capacity_bps_)/2.0); 
/* L 39 */
	queue_timer_->resched(Tq_);
 
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("Tq_", Tq_);
		trace_var("queue_bytes_", queue_bytes_);
		trace_var("routerId_", routerId_);
	}
}

void XCPQueue::Te_timeout()
{
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("residue_pos_fbk_not_allocated", residue_pos_fbk_);
		trace_var("residue_neg_fbk_not_allocated", residue_neg_fbk_);
	}
	if (spread_bytes_) {
		double spreaded_bytes = b_[0];
		double tp = t_[0];
		for (int i = 1; i <= maxb_; ++i) {
			double spill = b_[i]/(i+1) + .5;
			spreaded_bytes += spill;
			b_[i-1] = b_[i] - spill;

			spill = t_[i]/(i+1);
		        tp += spill;
			t_[i-1] = t_[i] - spill;
		}
		b_[maxb_] = t_[maxb_] = 0;
		if (maxb_ > 0)
			--maxb_;
		input_traffic_bytes_ = spreaded_bytes;
		sum_inv_throughput_ = tp;
	}
		
	double input_bw = input_traffic_bytes_ / Te_;
	double phi_bps = 0.0;
	double shuffled_traffic_bps = 0.0;

	if (spread_bytes_) {
		avg_rtt_ = (maxb_ + 1)*BWIDTH/2; // XXX fix me
	} else {
		if (sum_inv_throughput_ != 0.0) {
/* L 7 */
			avg_rtt_ = sum_rtt_by_throughput_ / sum_inv_throughput_;
		} else
			avg_rtt_ = INITIAL_Te_VALUE;
	}

	if (input_traffic_bytes_ > 0) {
/* L 8 */
		phi_bps = ALPHA_ * (link_capacity_bps_- input_bw) 
			- BETA_ * queue_bytes_ / avg_rtt_;
/* L 9 corrected */
		shuffled_traffic_bps = GAMMA_ * input_bw;

		if (shuffled_traffic_bps > abs(phi_bps))
			shuffled_traffic_bps -= abs(phi_bps);
		else
			shuffled_traffic_bps = 0.0;
/* L 9 ends here */
	}
/* L 12, 13 */	
	residue_pos_fbk_ = max(0,  phi_bps) + shuffled_traffic_bps;
	residue_neg_fbk_ = max(0, -phi_bps) + shuffled_traffic_bps;

	if (sum_inv_throughput_ == 0.0)
		sum_inv_throughput_ = 1.0;
	if (input_traffic_bytes_ > 0) {
/* L 10 */
		Cp_ =  residue_pos_fbk_ / sum_inv_throughput_;
/* L 11 */
		Cn_ =  residue_neg_fbk_ / input_traffic_bytes_;
	} else 
		Cp_ = Cn_ = 0.0;
		
	if (TRACE && (queue_trace_file_ != 0)) {
		trace_var("input_traffic_bytes_", input_traffic_bytes_);
		trace_var("avg_rtt_", avg_rtt_);
		trace_var("residue_pos_fbk_", residue_pos_fbk_);
		trace_var("residue_neg_fbk_", residue_neg_fbk_);
		trace_var("Qsize", curq_);
		trace_var("Qavg", edv_.v_ave);
		trace_var("routerId", routerId_);
	} 
	num_cc_packets_in_Te_ = 0;

/* L 14 */  
	input_traffic_bytes_ = 0.0;
/* L 15 */
	sum_inv_throughput_ = 0.0;
/* L 16 */
	sum_rtt_by_throughput_ = 0.0;
/* L 17 */
	if (spread_bytes_)
		Te_ = BWIDTH;
	else
		Te_ = max(avg_rtt_, XCP_MIN_INTERVAL);

/* L 18 */
	estimation_control_timer_->resched(Te_);
}

// Estimation & Control Helpers
void XCPQueue::init_vars() 
{
	link_capacity_bps_	= 0.0;
	avg_rtt_		= INITIAL_Te_VALUE;
	if (spread_bytes_)
		Te_		= BWIDTH;
	else
		Te_		= INITIAL_Te_VALUE;

	Tq_			= INITIAL_Te_VALUE; 
	Cp_			= 0.0;
	Cn_			= 0.0;     
	residue_pos_fbk_	= 0.0;
	residue_neg_fbk_	= 0.0;     
	queue_bytes_		= 0.0; // our estimate of the fluid model queue
  
	input_traffic_bytes_	= 0.0; 
	sum_rtt_by_throughput_	= 0.0;
	sum_inv_throughput_	= 0.0;
	running_min_queue_bytes_= 0;
	num_cc_packets_in_Te_   = 0;
  
	queue_trace_file_ = 0;
	myQueue_ = 0;

	spread_bytes_ 		= 0;
	for (int i = 0; i<BSIZE; ++i)
		b_[i] = t_[i] = 0;
	maxb_ = 0;
}


void XCPTimer::expire(Event *) { 
	(*a_.*call_back_)();
}


void XCPQueue::trace_var(char * var_name, double var)
{
	char wrk[500];
	double now = Scheduler::instance().clock();

	if (queue_trace_file_) {
		int n;
		sprintf(wrk, "%s %g %g", var_name, now, var);
		n = strlen(wrk);
		wrk[n] = '\n'; 
		wrk[n+1] = 0;
		(void)Tcl_Write(queue_trace_file_, wrk, n+1);
	}
	return; 
}


