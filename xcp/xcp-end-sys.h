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

#define		MAX(a,b)	((a) > (b) ? (a) : (b))
#define		TP_TO_TICKS	MAX(1, (t_srtt_ >> T_SRTT_BITS))

#define TP_AVG_EXP		4	// used for xcp_metered_output_ == true

class XcpAgent;
  
class cwndShrinkingTimer : public TimerHandler {
public: 
        cwndShrinkingTimer(XcpAgent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        XcpAgent *a_;
};

class XcpAgent : public RenoTcpAgent {
 public:
	XcpAgent();
 protected:
	double time_now()  { return  Scheduler::instance().clock(); };
	void trace_var(char * var_name, double var);
	
	void init_rtt_vars(){
		flag_first_ack_received_ = 0.0;
		srtt_estimate_           = 0.0;
	}
        virtual void delay_bind_init_all();
        virtual int delay_bind_dispatch(const char *varName, 
					const char *localName, 
					TclObject *tracer);

	virtual void output(int seqno, int reason = 0);
	virtual void recv_newack_helper(Packet *); 
	virtual void opencwnd(); 
	virtual void rtt_init(); // called in reset()
	virtual void rtt_update(double tao);

	/*--------- Variables --------------*/
	double current_positive_feedback_ ;
	int    tcpId_;
	double srtt_estimate_;
	double flag_first_ack_received_;

	// these are used if xcp_metered_output_ is true
	int    xcp_metered_output_;
	double xcp_feedback_;
	long   sent_bytes_;
	long   s_sent_bytes_;
	long   tp_to_start_ticks_;
	long   tp_to_ticks_;
	double estimated_throughput_;
	cwndShrinkingTimer shrink_cwnd_timer_;
};

#endif /* ns_xcp_end_sys_h */
