/*  Author: Dina Katabi */
/*  Date Jan 2002 */

#ifndef ns_cctcp_h
#define ns_cctcp_h

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

static double INITIAL_RTT_ESTIMATE = 0.001;

//-----------  The Congestion Header ------------//
struct hdr_cctcp {

	//---- Modified Only By Source
	double    cwnd_;            // The current window 
	double    rtt_;             // My rtt estimate
	int       ccenabled_;       // to indicate that the flow is CC enabled
	int       cctcpId_;         // Sender's ID
	double    price_;           // price the sender is willing to pay for a bit of traffic
	double    profile_;         // The rate that the provider has promised to the source (best effort 0)

	// --- Initialized by source and Updated by Router 
	double positive_feedback_;  // router's feedback to the source about how much to open its cwnd (initialized to -1)
	double negative_feedback_;  // router's feedback to source about how much to decrease its cwnd (initialized to -1)
	double shuffling_feedback_; // the router's feedback to the source asking for halving cwnd
	unsigned int controlling_hop_;  // The AQM ID of the controlling router
	unsigned int restricted_;       // a bit set by the routers when the is the source's 
	                                // cwnd/rtt > avg throughput at the router and SBW < 0
	unsigned int was_restricted_;   // set if restricted was set in last received ack.
	int          restricted_by_;    // initialized to -1. has the restricting router's id

	static int offset_;	    // offset for this header
	inline static int& offset() { return offset_; }
	inline static hdr_cctcp* access(Packet* p) {
		return (hdr_cctcp*) p->access(offset_);
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

class CC1TcpAgent;
  
class cwndShrinkingTimer : public TimerHandler {
public: 
        cwndShrinkingTimer(CC1TcpAgent *a) : TimerHandler() { a_ = a; }
protected:
        virtual void expire(Event *e);
        CC1TcpAgent *a_;
};



//---------- CC1 TCP : Derived from Reno -------------//
class CC1TcpAgent : public virtual RenoTcpAgent {
 public:
	CC1TcpAgent();
	int command(int argc, const char*const* argv);	
 protected:
	//----- Utilities Functions
	double time_now()  { return  Scheduler::instance().clock(); };
	void trace_var(char * var_name, double var);
	
	//------ Functions
	void init_rtt_vars(){
		flag_first_ack_received_ = 0.0;
		rtt_estimate_            = INITIAL_RTT_ESTIMATE; 
		srtt_estimate_           = INITIAL_RTT_ESTIMATE;
		I_was_restricted_        = 0;
	}

	virtual void output(int seqno, int reason = 0); // modified so that the packet says cwnd and rtt
	virtual void recv_newack_helper(Packet*); // updates congestion info based on info in the ack
	virtual void opencwnd(); // compute the next window based on info extracted from the last ack
	//---- Rtt Estimation
	virtual void rtt_init(); // called in reset()
	virtual void rtt_update(double tao);

	/*--------- Variables --------------*/
	double current_positive_feedback_ ;
	int    tcpId_;
	double rtt_estimate_;
	double srtt_estimate_;
	double flag_first_ack_received_;

	unsigned int I_was_restricted_; //set when the sender receives an ack with restricted_ set
	double    price_;           // price the sender is willing to pay for a bit of traffic
	double    profile_;         // rate the sender bought from ISP
	cwndShrinkingTimer shrink_cwnd_timer_;
};

#endif
