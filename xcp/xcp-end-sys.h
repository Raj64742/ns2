/*  Based on: Dina Katabi, Jan 2002 */

#ifndef ns_xcp_end_sys_h
#define ns_xcp_end_sys_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ip.h"
#include "tcp.h"
#include "flags.h"

#include "agent.h"
#include "packet.h"

#include "flags.h"
#include "tcp-sink.h"

//-----------  The Congestion Header ------------//
struct hdr_xcp {
	double	throughput_;
	double	rtt_;
	enum {
		XCP_DISABLED = 0,
		XCP_ENABLED,
		XCP_ACK
	} 	xcp_enabled_;       // to indicate that the flow is CC enabled
	int	xcpId_;         // Sender's ID
	double	cwnd_;            // The current window 
	double	reverse_feedback_;

	// --- Initialized by source and Updated by Router 
	double delta_throughput_;
	unsigned int controlling_hop_;  // The AQM ID of the controlling router

	static int offset_;	    // offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_xcp* access(Packet* p) {
		return (hdr_xcp*) p->access(offset_);
	}

	/* per-field member functions */
	double& cwnd() { return (cwnd_); }
	double& rtt() { return (rtt_); }
};

/*--------------- Cwnd Shrinking Timer ---------------*
 *  If the cwnd becomes smaller than 1 then we keep it
 *  as one and reduce but delay sending the packet
 *  for s_rtt/cwnd
 * 
 * This code is to be written later!
 */

class XcpAgent;
  
class cwndShrinkingTimer : public TimerHandler {
public: 
        cwndShrinkingTimer(XcpAgent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        XcpAgent *a_;
};

class XcpAgent : public virtual RenoTcpAgent {
 public:
	XcpAgent();
 protected:
	double time_now()  { return  Scheduler::instance().clock(); };
	void trace_var(char * var_name, double var);
	
	void init_rtt_vars(){
		flag_first_ack_received_ = 0.0;
		srtt_estimate_           = 0.0;
	}

	virtual void output(int seqno, int reason = 0); // modified so that the packet says cwnd and rtt
	virtual void recv_newack_helper(Packet*); // updates congestion info based on info in the ack
	virtual void opencwnd(); // compute the next window based on info extracted from the last ack
	virtual void rtt_init(); // called in reset()
	virtual void rtt_update(double tao);

	/*--------- Variables --------------*/
	double current_positive_feedback_ ;
	int    tcpId_;
	double srtt_estimate_;
	double flag_first_ack_received_;

	cwndShrinkingTimer shrink_cwnd_timer_;
};

#endif /* ns_xcp_end_sys_h */
